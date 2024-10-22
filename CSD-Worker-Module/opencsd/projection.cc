#include "projection.h"

void Projection::projection_worker(){
    MergeBuffer* merge_buffer = new MergeBuffer;
    merge_buffer_list_.push_back(merge_buffer);

    while (1){
        CsdResult result = projection_queue_->wait_and_pop();

        projectioning(merge_buffer, result);
    }

    delete merge_buffer;
}

void Projection::projectioning(MergeBuffer* merge_buffer, CsdResult& result){
    string key = make_map_key(result.snippet->query_id, result.snippet->work_id);

    //Key에 해당하는 블록버퍼가 없다면 생성
    make_block_count_map(key, result.snippet->result_info.csd_block_count);
    merge_buffer->check_id_buffer(key, result.snippet);

    //data가 있는 경우만 수행
    if(result.data.row_count != 0){
        vector<int> column_offset_list = result.snippet->schema_info.column_offset;
        
        for(int i=0; i<result.data.row_count; i++){
            int origin_row_length = result.data.row_offset[i+1] - result.data.row_offset[i];
            char origin_row_data[origin_row_length];
            memcpy(origin_row_data, result.data.raw_data+result.data.row_offset[i], origin_row_length);

            if(result.snippet->schema_info.has_var_char){
                calcul_column_offset(origin_row_data, result.snippet->schema_info, column_offset_list);
                column_offset_list.push_back(origin_row_length);
            }

            int projection_row_length = 0;
            char projection_row_data[origin_row_length] = {0};

            for(int j=0; j<result.snippet->query_info.projection.size(); j++){
                switch (result.snippet->query_info.projection[j].select_type){
                    case KETI_SELECT_TYPE::COL_NAME:{
                        int column_length = projection_raw_data(origin_row_data,result.snippet->schema_info, 
                            result.snippet->query_info.projection[j].expression, column_offset_list, projection_row_data, projection_row_length);
                        projection_row_length += column_length;
                        // /*debugg*/cout<<"buffer ";for(int t=0; t<projection_row_length; t++){printf("%02X ",(u_char)projection_row_data[t]);}cout << endl;
                        break;
                    }case KETI_SELECT_TYPE::SUM:
                    case KETI_SELECT_TYPE::COUNT:
                    case KETI_SELECT_TYPE::COUNTSTAR:
                    case KETI_SELECT_TYPE::TOP:
                    case KETI_SELECT_TYPE::MIN:
                    case KETI_SELECT_TYPE::MAX:
                    default:{
                        // AVG, COUNTDISTINCT cannot offload
                        KETILOG::ERRORLOG(LOGTAG, "@error undefined merge type");
                        break;
                    }
                }
            }

            //row 추가 시 버퍼 크기 넘는지 확인 
            if(merge_buffer->id_buffer_map[key].data.data_length + projection_row_length > BUFFER_SIZE){
                return_layer_->enqueue_return(merge_buffer->id_buffer_map[key]);
                merge_buffer->id_buffer_map[key].data.clear();
            }

            //버퍼에 column_projection 한 row 넣기
            merge_buffer->id_buffer_map[key].data.row_offset.push_back(merge_buffer->id_buffer_map[key].data.data_length);
            merge_buffer->id_buffer_map[key].data.row_count += 1;
            int current_offset = merge_buffer->id_buffer_map[key].data.data_length;
            memcpy(merge_buffer->id_buffer_map[key].data.raw_data+current_offset, projection_row_data, projection_row_length);
            merge_buffer->id_buffer_map[key].data.data_length += projection_row_length;
        }
    }

    merge_buffer->id_buffer_map[key].data.current_block_count += result.data.current_block_count;
    merge_buffer->id_buffer_map[key].data.scanned_row_count += result.data.scanned_row_count;
    merge_buffer->id_buffer_map[key].data.filtered_row_count += result.data.filtered_row_count;

    // block_count 내리고 작업 완료 시 release_buffer
    block_count_down_and_release_buffer(key, result.data.current_block_count);
}

void Projection::make_block_count_map(string key, int csd_block_count){
    unique_lock<mutex> lock(mu);

    if(id_block_count_map_.find(key)==id_block_count_map_.end()){
        id_block_count_map_[key] = csd_block_count;
    }
}

void Projection::block_count_down_and_release_buffer(string key, int block_count){
    unique_lock<mutex> lock(mu);

    if(id_block_count_map_.find(key)!=id_block_count_map_.end()){
        id_block_count_map_[key] = id_block_count_map_[key] - block_count;

        if(id_block_count_map_[key] == 0){
            cout << "block merge done!! " << endl;
            for(const auto& merge_buffer: merge_buffer_list_){
                if(merge_buffer->id_buffer_map.find(key) != merge_buffer->id_buffer_map.end()){
                    return_layer_->enqueue_return(merge_buffer->id_buffer_map[key]);
                }
                merge_buffer->release_buffer(key);
            }
            id_block_count_map_.erase(key);
        }
    }
}