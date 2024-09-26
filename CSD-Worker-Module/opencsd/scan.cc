#include "scan.h"

const char *file_path  = "/root/workspace/keti/new-csd-worker-module/data_file/tpch1_no_index/tpch_origin_sst/lineitem/B/001384.sst";

void Scan::scan_worker(){
    while (1){
        shared_ptr<Snippet> snippet = scan_queue_->wait_and_pop();

        if(snippet->type == SNIPPET_TYPE::FULL_SCAN){
            data_block_full_scan(snippet);  
        }else if(snippet->type == SNIPPET_TYPE::INDEX_SCAN){
            data_block_index_scan(snippet);
        }else if(snippet->type == SNIPPET_TYPE::INDEX_TABLE_SCAN){
            index_block_scan(snippet);
        }else{
            KETILOG::ERRORLOG(LOGTAG, "@ERROR snippet type is not defined!");
        }       
    }
}

void Scan::data_block_full_scan(shared_ptr<Snippet> snippet){
    Result scan_result(snippet);

    rocksdb::Options options;
    rocksdb::SstFileReader sst_file_reader(options);
    sst_file_reader.Open(file_path);
    rocksdb::Iterator* datablock_iter = sst_file_reader.NewIterator(rocksdb::ReadOptions());
    
    char table_index_number[4];
    generate_seek_key(snippet->schema_info, table_index_number);
    rocksdb::Slice seek_key(table_index_number,4);

    // for(int i=0; i<snippet->block_info.block_list.size(); i++){
    //     int offset = snippet->block_info.block_list[i].offset;
    //     int length = snippet->block_info.block_list[i].length;

    //     Options options;
    //     SstBlockReader sst_block_reader(
    //         options, table_rep.blocks_maybe_compressed, table_rep.blocks_definitely_zstd_compressed, 
    //         table_rep.immortal_table, table_rep.read_amp_bytes_per_bit, table_rep.dev_name);
    //     Iterator* datablock_iter = sst_block_reader.NewIterator(ReadOptions());

        for (datablock_iter->Seek(seek_key); datablock_iter->Valid(); datablock_iter->Next()) { // seek same table index number block
            if (!datablock_iter->status().ok()) {
                KETILOG::ERRORLOG(LOGTAG, "Error reading the block - Skipped");
                break;
            }               

            const rocksdb::Slice& key = datablock_iter->key();
            const rocksdb::Slice& value = datablock_iter->value();

            if(key.keti_get_table_index_number() != snippet->schema_info.table_index_number){ // check table index number
                //*스캔 동작 종료, 지금까지 스캔한거 보내기
                return;
            }

            std::cout << key.ToString(true) << ": " << value.ToString(true) << std::endl;

            if(snippet->schema_info.pk_column.size() != 0){ // append key to front value
                string converted_key = convert_key_to_value(key, snippet->schema_info);
                scan_result.data.row_offset.push_back(scan_result.data.data_length);
                memcpy(scan_result.data.raw_data + scan_result.data.data_length, converted_key.c_str(), converted_key.size());
                memcpy(scan_result.data.raw_data + scan_result.data.data_length + converted_key.size(), value.data(), value.size());
                scan_result.data.data_length += converted_key.size() + value.size();
                scan_result.data.row_count++;
            }else{ // save value only
                scan_result.data.row_offset.push_back(scan_result.data.data_length);
                memcpy(scan_result.data.raw_data + scan_result.data.data_length, value.data(), value.size());
                scan_result.data.data_length += value.size();
                scan_result.data.row_count++;
            }

            if(scan_result.data.data_length >= 4096){ // *보낼지 체크
                scan_result.data.row_offset.push_back(scan_result.data.data_length);
                enqueue_scan_result(scan_result);
                scan_result.data.clear();
                break;//임시 테스트용 나중에 그냥 지우기
            }
        }

        if(scan_result.data.data_length > 0){ //임시 테스트용 나중에 그냥 지우기
            scan_result.data.row_offset.push_back(scan_result.data.data_length);
            enqueue_scan_result(scan_result);
            scan_result.data.clear();
        }

    // }

    return;
}

void Scan::data_block_index_scan(shared_ptr<Snippet> snippet){
    rocksdb::Options options;
    rocksdb::SstFileReader sst_file_reader(options);
    sst_file_reader.Open(file_path);
    rocksdb::Iterator* iter = sst_file_reader.NewIterator(rocksdb::ReadOptions());

    Result scan_result(snippet);

    for(int i=0; i<snippet->query_info.seek_key.size(); i++){
        string seek_key = snippet->query_info.seek_key[i];
        // table_index_number + seek_key
        iter->Seek(seek_key);

    }
}

void Scan::index_block_scan(shared_ptr<Snippet> snippet){ //*인덱스 테이블 스캔 구현

}

void Scan::scan_wal(shared_ptr<Snippet> snippet){ //*wal 스캔 동작 구현
    
}

void Scan::generate_seek_key(SchemaInfo& schema_info, char* table_index_number){
    char temp[4];
    memcpy(temp, &schema_info.table_index_number, sizeof(int));

    table_index_number[0] = temp[3];
    table_index_number[1] = temp[2];
    table_index_number[2] = temp[1];
    table_index_number[3] = temp[0];

    return;
}

string Scan::convert_key_to_value(const rocksdb::Slice& key, SchemaInfo& schema_info){
    string converted_key = "";
    int offset = 4;

    for(int p=0; p<schema_info.pk_column.size(); p++){
        string pk_column = schema_info.pk_column[p];
        int index = schema_info.column_index_map[pk_column];
        int column_length = schema_info.column_length[index];
        int column_type = schema_info.column_type[index];

        switch(column_type){ // *다른 타입 추가, tpch는 key에 int만 있음
            case MySQL_INT32:{
                string origin_key = key.keti_get_sliced_string(offset,column_length);
                origin_key[0] ^= 0x80;
                for (int i = origin_key.length() - 1; i >= 0; --i) {
                    converted_key += origin_key[i];
                }
                offset += column_length;
                break;
            }default: {
                KETILOG::FATALLOG(LOGTAG, "error, convert_key_to_value another data type ("+to_string(column_type)+ ")");
            }
        }
    }

    return converted_key;
}

void Scan::enqueue_scan_result(Result scan_result){
    if(scan_result.data.row_count == 0 || scan_result.snippet->query_info.filtering.size() == 0){
        projection_layer_->enqueue_projection(scan_result);
    }else {
        filter_layer_->enqueue_filter(scan_result);
    }
}