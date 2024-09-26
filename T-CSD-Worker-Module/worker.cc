#include "worker.h"

int Worker::working(){
    while (1){
        t_snippet t_snippet = work_queue_.wait_and_pop();
        string block_dir = t_snippet.block_dir_;

        int fd = open(block_dir.c_str(), O_RDONLY); 
        if (fd < 0){
            printf("cannot open block file %s\n", block_dir.c_str());
            return -1;
        }

        for(int i=0; i<t_snippet.chunks_.size(); i++){
            uint64_t block_dba = t_snippet.chunks_[i].block_dba_;
            uint64_t block_size = t_snippet.chunks_[i].block_size_;

            char buf[block_size];

            if (read(fd, buf, block_size) != block_size){ 
                printf("cannot read from block file %s\n", block_dir.c_str());
                close(fd);
                return -1;
            }

            close(fd);

            bool pass = true;
            int tcnum = 0, row_num = 0,col_cnt = 0, out_rows = 0; 
            
            _blk_t *blk = (_blk_t *)buf;  // get block pointer

            if (table_block_verify(blk, block_dba, block_size) == 0){ // block verify
                pass = false;
            }

            res_chunk_t *res_chunk;

            res_chunk = csd_filter_and_eval_out(blk, &t_snippet.filter_info_); // scan, filter

            ///*debugg*/for(int t=0; t<res_chunk->res_len; t++){cout<<"key ";printf("%02X ",(u_char)res_chunk->res_buf[t]);}cout << endl;

            return_queue_.push_work(*res_chunk);
        }
    }
}

