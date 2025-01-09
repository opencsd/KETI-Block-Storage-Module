#include "scan.h"

void Scan::scan_worker(){
    while (1){
        shared_ptr<Snippet> snippet = scan_queue_->wait_and_pop();
        
        if(snippet->type == SNIPPET_TYPE::FULL_SCAN){
            data_block_full_scan(snippet);  
        }else if(snippet->type == SNIPPET_TYPE::INDEX_SCAN){
            data_block_index_scan(snippet);
        }else if(snippet->type == SNIPPET_TYPE::INDEX_TABLE_SCAN){
            index_block_scan(snippet);
        }else if(snippet->type == SNIPPET_TYPE::SST_FILE_SCAN){
            sst_file_full_scan(snippet);
        }else{
            KETILOG::ERRORLOG(LOGTAG, "@ERROR snippet type is not defined!");
        }       
    }
}

void Scan::read_data_block(CsdResult &scan_result, shared_ptr<Snippet> snippet, vector<size_t> offset, vector<uint64_t> length, rocksdb::Slice& seek_key, int& left_block_count, int& key_index){
    rocksdb::Options options;
    rocksdb::SstBlockReader sst_block_reader(
        options, false/*blocks_maybe_compressed*/, false/*blocks_definitely_zstd_compressed*/, 
        false/*immortal_table*/, 0/*read_amp_bytes_per_bit*/, snippet->block_info.partition);
    rocksdb::BlockInfo blockInfo(0/*_blockid*/,offset, length);
    rocksdb::Status s  = sst_block_reader.Open(&blockInfo);
    if (!s.ok()) {
        std::cerr << "Error in sst_block_reader.Open(): " << s.ToString() << std::endl;
    }
    rocksdb::Iterator* datablock_iter = sst_block_reader.NewIterator(rocksdb::ReadOptions());

    if(key_index != -1){
        while(key_index < snippet->query_info.seek_key.size()){
            // seek_key 설정 필요!!
            /*
                char table_index_number[4];
                generate_seek_key(snippet->schema_info, table_index_number);
                snippet->query_info.seek_key[key_index]
                seek_key = table_index_number 4byte + pk nbyte
                rocksdb::Slice seek_key("",4);
            */

            datablock_iter->Seek(seek_key);
            if (!datablock_iter->status().ok() || !datablock_iter->Valid()) {
                KETILOG::ERRORLOG(LOGTAG, "Error reading the block - Skipped");
                break;
            }    

            const rocksdb::Slice& key = datablock_iter->key();
            const rocksdb::Slice& value = datablock_iter->value();

            //조건 확인 필요!!
            /*
            if(seek success){
                save data
                key_index++
            }else{
                break
            }
            */

            if(key.keti_get_table_index_number() != snippet->schema_info.table_index_number){ // check table index number
                scan_result.data.current_block_count = left_block_count;
                left_block_count = 0;
                scan_result.data.row_offset.push_back(scan_result.data.data_length);
                enqueue_scan_result(scan_result);
                scan_result.data.clear();
                return;
            }

            // std::cout << key.ToString(true) << ": " << value.ToString(true) << std::endl;

            string converted_key = convert_key_to_value(key, snippet->schema_info);
            scan_result.data.row_offset.push_back(scan_result.data.data_length);
            memcpy(scan_result.data.raw_data + scan_result.data.data_length, converted_key.c_str(), converted_key.size());
            memcpy(scan_result.data.raw_data + scan_result.data.data_length + converted_key.size(), value.data(), value.size());
            scan_result.data.data_length += converted_key.size() + value.size();
            scan_result.data.row_count++;
            scan_result.data.scanned_row_count++;
        }

    }else{
        for (datablock_iter->Seek(seek_key); datablock_iter->Valid(); datablock_iter->Next()) { // seek same table index number block
            if (!datablock_iter->status().ok()) {
                KETILOG::ERRORLOG(LOGTAG, "Error reading the block - Skipped");
                break;
            }               

            const rocksdb::Slice& key = datablock_iter->key();
            const rocksdb::Slice& value = datablock_iter->value();

            if(key.keti_get_table_index_number() != snippet->schema_info.table_index_number){ // check table index number
                scan_result.data.current_block_count += left_block_count;
                left_block_count = 0;
                scan_result.data.row_offset.push_back(scan_result.data.data_length);
                enqueue_scan_result(scan_result);
                scan_result.data.clear();
                return;
            }

            // std::cout << key.ToString(true) << ": " << value.ToString(true) << std::endl;

            if(snippet->schema_info.pk_column.size() != 0){ // append key to front value
                string converted_key = convert_key_to_value(key, snippet->schema_info);
                scan_result.data.row_offset.push_back(scan_result.data.data_length);
                memcpy(scan_result.data.raw_data + scan_result.data.data_length, converted_key.c_str(), converted_key.size());
                memcpy(scan_result.data.raw_data + scan_result.data.data_length + converted_key.size(), value.data(), value.size());
                scan_result.data.data_length += converted_key.size() + value.size();
                scan_result.data.row_count++;
                scan_result.data.scanned_row_count++;
            }else{ // save value only
                scan_result.data.row_offset.push_back(scan_result.data.data_length);
                memcpy(scan_result.data.raw_data + scan_result.data.data_length, value.data(), value.size());
                scan_result.data.data_length += value.size();
                scan_result.data.row_count++;
                scan_result.data.scanned_row_count++;
            }
        }
    }

    scan_result.data.current_block_count += 1;
    left_block_count -= 1;

    if(scan_result.data.current_block_count == NUM_OF_BLOCKS || left_block_count == 0){
        if(left_block_count == 0){
            cout << "scanned row count : " << scan_result.data.scanned_row_count << "(" << snippet->work_id << ")" << endl;
        }
        scan_result.data.row_offset.push_back(scan_result.data.data_length);
        enqueue_scan_result(scan_result);
        scan_result.data.clear();
    }
}

void Scan::data_block_full_scan(shared_ptr<Snippet> snippet){
    CsdResult scan_result(snippet);
    
    char table_index_number[4];
    generate_seek_key(snippet->schema_info.table_index_number, table_index_number);
    rocksdb::Slice seek_key(table_index_number,4);

    int left_block_count = snippet->result_info.csd_block_count;
    int key_index = -1;

    for(int i=0; i<snippet->block_info.block_list.size(); i++){
        vector<uint64_t> offset = snippet->block_info.block_list[i].offset;

        if(offset.size() == 1){
            for(int j=0; j<snippet->block_info.block_list[i].length.size(); j++){
                vector<uint64_t> length;
                length.push_back(snippet->block_info.block_list[i].length[j]);

                read_data_block(scan_result, snippet, offset, length, seek_key, left_block_count, key_index);

                offset[0] += length[0];
            }   
        }else{
            vector<uint64_t> length;
            for(int j=0; j<snippet->block_info.block_list[i].length.size(); j++){
                length.push_back(snippet->block_info.block_list[i].length[j]);
            }

            read_data_block(scan_result, snippet, offset, length, seek_key, left_block_count, key_index);
        }
    }

    return;
}

void Scan::data_block_index_scan(shared_ptr<Snippet> snippet){
    // 파일 read 테스트 코드
    rocksdb::Options options;
    rocksdb::SstFileReader sst_file_reader(options);
    sst_file_reader.Open("/root/workspace/keti/data_file/tpch1_no_index/tpch_origin_sst/lineitem/A/001400.sst");
    rocksdb::Iterator* datafile_iter = sst_file_reader.NewIterator(rocksdb::ReadOptions());

    CsdResult scan_result(snippet);
    int key_index = 0;

    for(int i=0; i<snippet->query_info.seek_key.size(); i++){
        for(int current_key_index = key_index; current_key_index < snippet->query_info.seek_key.size(); current_key_index++){
            // seek_key 설정 필요!!
            // char table_index_number[4];
            // generate_seek_key(snippet->schema_info, table_index_number);
            // snippet->query_info.seek_key[current_key_index]
            // seek_key = table_index_number 4byte + pk nbyte
            
           rocksdb::Slice seek_key("",4);

            datafile_iter->Seek(seek_key);
            if (!datafile_iter->status().ok() || !datafile_iter->Valid()) {
                KETILOG::ERRORLOG(LOGTAG, "Error reading the block - Skipped");
                break;
            }    

            const rocksdb::Slice& key = datafile_iter->key();
            const rocksdb::Slice& value = datafile_iter->value();

            //조건 확인 필요!!
            /*
            if(seek success){
                save data
                current_key_index++
            }else{
                break
            }
            */

            if(key.keti_get_table_index_number() != snippet->schema_info.table_index_number){ // check table index number
                // scan_result.data.current_block_count += left_block_count;
                // left_block_count = 0;
                scan_result.data.row_offset.push_back(scan_result.data.data_length);
                enqueue_scan_result(scan_result);
                scan_result.data.clear();
                return;
            }

            // std::cout << key.ToString(true) << ": " << value.ToString(true) << std::endl;

            string converted_key = convert_key_to_value(key, snippet->schema_info);
            scan_result.data.row_offset.push_back(scan_result.data.data_length);
            memcpy(scan_result.data.raw_data + scan_result.data.data_length, converted_key.c_str(), converted_key.size());
            memcpy(scan_result.data.raw_data + scan_result.data.data_length + converted_key.size(), value.data(), value.size());
            scan_result.data.data_length += converted_key.size() + value.size();
            scan_result.data.row_count++;
            scan_result.data.scanned_row_count++;
        }

    }

    // 실제 블록 read 코드
    // CsdResult scan_result(snippet);

    // int left_block_count = snippet->result_info.csd_block_count;
    // int key_index = 0;
    // rocksdb::Slice seek_key("",4);

    // for(int i=0; i<snippet->block_info.block_list.size(); i++){
    //     vector<uint64_t> offset = snippet->block_info.block_list[i].offset;

    //     if(offset.size() == 1){
    //         for(int j=0; j<snippet->block_info.block_list[i].length.size(); j++){
    //             vector<uint64_t> length;
    //             length.push_back(snippet->block_info.block_list[i].length[j]);

    //             read_data_block(scan_result, snippet, offset, length, seek_key, left_block_count, key_index);

    //             offset[0] += length[0];
    //         }   
    //     }else{
    //         vector<uint64_t> length;
    //         for(int j=0; j<snippet->block_info.block_list[i].length.size(); j++){
    //             length.push_back(snippet->block_info.block_list[i].length[j]);
    //         }

    //         read_data_block(scan_result, snippet, offset, length, seek_key, left_block_count, key_index);
    //     }
    // }
}

void Scan::read_index_block(CsdResult &scan_result, shared_ptr<Snippet> snippet, vector<size_t> offset, vector<uint64_t> length, rocksdb::Slice& seek_key, int& left_block_count, int& key_index){
    rocksdb::Options options;
    rocksdb::SstBlockReader sst_block_reader(
        options, false/*blocks_maybe_compressed*/, false/*blocks_definitely_zstd_compressed*/, 
        false/*immortal_table*/, 0/*read_amp_bytes_per_bit*/, snippet->block_info.partition);
    rocksdb::BlockInfo blockInfo(0/*_blockid*/,offset, length);
    rocksdb::Status s  = sst_block_reader.Open(&blockInfo);
    if (!s.ok()) {
        std::cerr << "Error in sst_block_reader.Open(): " << s.ToString() << std::endl;
    }
    rocksdb::Iterator* datablock_iter = sst_block_reader.NewIterator(rocksdb::ReadOptions());

    if(key_index != -1){
        for(int current_key_index = key_index; current_key_index < snippet->query_info.seek_key.size(); current_key_index++){
            // seek_key 설정 필요!!
            /*
                char table_index_number[4];
                generate_seek_key(snippet->schema_info, table_index_number);
                snippet->query_info.seek_key[current_key_index]
                seek_key = table_index_number 4byte + pk nbyte
                rocksdb::Slice seek_key("",4);
            */

            datablock_iter->Seek(seek_key);
            if (!datablock_iter->status().ok() || !datablock_iter->Valid()) {
                KETILOG::ERRORLOG(LOGTAG, "Error reading the block - Skipped");
                break;
            }    

            const rocksdb::Slice& key = datablock_iter->key();
            const rocksdb::Slice& value = datablock_iter->value();

            //조건 확인 필요!!
            /*
            if(seek success){
                save data
                current_key_index++
            }else{
                break
            }
            */

            if(key.keti_get_table_index_number() != snippet->schema_info.table_index_number){ // check table index number
                scan_result.data.current_block_count = left_block_count;
                left_block_count = 0;
                scan_result.data.row_offset.push_back(scan_result.data.data_length);
                enqueue_scan_result(scan_result);
                scan_result.data.clear();
                return;
            }

            // std::cout << key.ToString(true) << ": " << value.ToString(true) << std::endl;

            string converted_key = convert_key_to_value(key, snippet->schema_info);
            scan_result.data.row_offset.push_back(scan_result.data.data_length);
            memcpy(scan_result.data.raw_data + scan_result.data.data_length, converted_key.c_str(), converted_key.size());
            memcpy(scan_result.data.raw_data + scan_result.data.data_length + converted_key.size(), value.data(), value.size());
            scan_result.data.data_length += converted_key.size() + value.size();
            scan_result.data.row_count++;
            scan_result.data.scanned_row_count++;
        }

    }else{
        for (datablock_iter->SeekToFirst(); datablock_iter->Valid(); datablock_iter->Next()) { // seek same table index number block
            if (!datablock_iter->status().ok()) {
                KETILOG::ERRORLOG(LOGTAG, "Error reading the block - Skipped");
                break;
            }               

            const rocksdb::Slice& key = datablock_iter->key();
            const rocksdb::Slice& value = datablock_iter->value();

            if(key.keti_get_table_index_number() != snippet->schema_info.table_index_number){ // check table index number
                scan_result.data.current_block_count = left_block_count;
                left_block_count = 0;
                scan_result.data.row_offset.push_back(scan_result.data.data_length);
                enqueue_scan_result(scan_result);
                scan_result.data.clear();
                return;
            }

            std::cout << key.ToString(true) << ": " << value.ToString(true) << std::endl;

            string converted_key = convert_key_to_value(key, snippet->schema_info);
            scan_result.data.row_offset.push_back(scan_result.data.data_length);
            memcpy(scan_result.data.raw_data + scan_result.data.data_length, converted_key.c_str(), converted_key.size());
            memcpy(scan_result.data.raw_data + scan_result.data.data_length + converted_key.size(), value.data(), value.size());
            scan_result.data.data_length += converted_key.size() + value.size();
            scan_result.data.row_count++;
            scan_result.data.scanned_row_count++;
        }
    }

    scan_result.data.current_block_count += 1;
    left_block_count -= 1;

    if(scan_result.data.current_block_count == NUM_OF_BLOCKS || left_block_count == 0){
        if(left_block_count == 0){
            cout << "scanned row count : " << scan_result.data.scanned_row_count << "(" << snippet->work_id << ")" << endl;
        }
        scan_result.data.row_offset.push_back(scan_result.data.data_length);
        enqueue_scan_result(scan_result);
        scan_result.data.clear();
    }
}

void Scan::index_block_scan(shared_ptr<Snippet> snippet){ //*인덱스 테이블 스캔 구현
    CsdResult scan_result(snippet);

    int left_block_count = snippet->result_info.csd_block_count;
    
    if(snippet->query_info.seek_key.size() == 0){//index table full scan
        char table_index_number[4];
        generate_seek_key(snippet->schema_info.table_index_number, table_index_number);
        rocksdb::Slice seek_key(table_index_number,4);

        int key_index = -1;

        for(int i=0; i<snippet->block_info.block_list.size(); i++){
            vector<uint64_t> offset = snippet->block_info.block_list[i].offset;

            if(offset.size() == 1){
                for(int j=0; j<snippet->block_info.block_list[i].length.size(); j++){
                    vector<uint64_t> length;
                    length.push_back(snippet->block_info.block_list[i].length[j]);

                    read_index_block(scan_result, snippet, offset, length, seek_key, left_block_count, key_index);

                    offset[0] += length[0];
                }   
            }else{
                vector<uint64_t> length;
                for(int j=0; j<snippet->block_info.block_list[i].length.size(); j++){
                    length.push_back(snippet->block_info.block_list[i].length[j]);
                }

                read_index_block(scan_result, snippet, offset, length, seek_key, left_block_count, key_index);
            }
        }

    }else{//index table index scan
        int key_index = 0;
        rocksdb::Slice seek_key("",4);

        for(int i=0; i<snippet->block_info.block_list.size(); i++){
            vector<uint64_t> offset = snippet->block_info.block_list[i].offset;

            if(offset.size() == 1){
                for(int j=0; j<snippet->block_info.block_list[i].length.size(); j++){
                    vector<uint64_t> length;
                    length.push_back(snippet->block_info.block_list[i].length[j]);

                    read_index_block(scan_result, snippet, offset, length, seek_key, left_block_count, key_index);

                    offset[0] += length[0];
                }   
            }else{
                vector<uint64_t> length;
                for(int j=0; j<snippet->block_info.block_list[i].length.size(); j++){
                    length.push_back(snippet->block_info.block_list[i].length[j]);
                }

                read_index_block(scan_result, snippet, offset, length, seek_key, left_block_count, key_index);
            }
        }
    }
}

void Scan::sst_file_full_scan(shared_ptr<Snippet> snippet){
    CsdResult scan_result(snippet);

    int left_block_count = snippet->result_info.csd_block_count;
    int total_scanned_row_count = 0;

    char table_index_number[4];
    generate_seek_key(snippet->schema_info.table_index_number, table_index_number);
    rocksdb::Slice lower_bound_key(table_index_number,4);

    char upper_bound[4];
    generate_seek_key(snippet->schema_info.table_index_number + 1, upper_bound);
    rocksdb::Slice upper_bound_key(upper_bound,4);

    for(int i=0; i<snippet->block_info.sst_list.size(); i++){
        rocksdb::Options options;
        rocksdb::SstFileReader sst_file_reader(options);
        string file_path = "/home/ngd/storage/keti-opencsd-origin/" + snippet->block_info.sst_list[i];
        sst_file_reader.Open(file_path);
        rocksdb::ReadOptions read_option;
        read_option.iterate_lower_bound = &lower_bound_key;
        read_option.iterate_upper_bound= &upper_bound_key;
        rocksdb::Iterator* datafile_iter = sst_file_reader.NewIterator(read_option);

        for (datafile_iter->SeekToFirst(); datafile_iter->Valid(); datafile_iter->Next()) {
            if (!datafile_iter->status().ok()) {
                KETILOG::ERRORLOG(LOGTAG, "Error reading the block - Skipped");
                break;
            }    

            const rocksdb::Slice& key = datafile_iter->key();
            const rocksdb::Slice& value = datafile_iter->value();

            if(snippet->schema_info.pk_column.size() != 0){ // append key at front of the value
                string converted_key = convert_key_to_value(key, snippet->schema_info);
                int row_length = converted_key.size() + value.size();

                if(scan_result.data.data_length + row_length > BUFFER_SIZE){
                    scan_result.data.current_block_count += 100;
                    left_block_count -= 100;
                    scan_result.data.row_offset.push_back(scan_result.data.data_length);
                    enqueue_scan_result(scan_result);
                    scan_result.data.clear();
                }

                scan_result.data.row_offset.push_back(scan_result.data.data_length);
                memcpy(scan_result.data.raw_data + scan_result.data.data_length, converted_key.c_str(), converted_key.size());
                memcpy(scan_result.data.raw_data + scan_result.data.data_length + converted_key.size(), value.data(), value.size());
                scan_result.data.data_length += row_length;
                scan_result.data.row_count++;
                scan_result.data.scanned_row_count++;
                total_scanned_row_count++;
            }else{ // save value only
                int row_length = value.size();

                if(scan_result.data.data_length + row_length > BUFFER_SIZE){
                    scan_result.data.current_block_count += 100;
                    left_block_count -= 100;
                    scan_result.data.row_offset.push_back(scan_result.data.data_length);
                    enqueue_scan_result(scan_result);
                    scan_result.data.clear();
                }

                scan_result.data.row_offset.push_back(scan_result.data.data_length);
                memcpy(scan_result.data.raw_data + scan_result.data.data_length, value.data(), value.size());
                scan_result.data.data_length += row_length;
                scan_result.data.row_count++;
                scan_result.data.scanned_row_count++;
                total_scanned_row_count++;
            }
        }
    }    

    cout << "total scanned row count : " << total_scanned_row_count << "(" << snippet->work_id << ")" << endl;

    scan_result.data.current_block_count += left_block_count;
    left_block_count = 0;
    scan_result.data.row_offset.push_back(scan_result.data.data_length);
    enqueue_scan_result(scan_result);
    scan_result.data.clear();
}

void Scan::scan_wal(shared_ptr<Snippet> snippet){ //*wal 스캔 동작 구현
    
}

void Scan::generate_seek_key(int number, char* table_index_number){
    char temp[4];
    memcpy(temp, &number, sizeof(int));

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

void Scan::enqueue_scan_result(CsdResult scan_result){
    if(scan_result.data.row_count == 0 || scan_result.snippet->query_info.filtering.size() == 0){
        scan_result.data.filtered_row_count = scan_result.data.scanned_row_count;
        projection_layer_->enqueue_projection(scan_result);
    }else {
        filter_layer_->enqueue_filter(scan_result);
    }
}