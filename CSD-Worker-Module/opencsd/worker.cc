#include "worker.h"

void Worker::tmax_working(){
    while (1){
        shared_ptr<tSnippet> t_snippet = work_queue_->wait_and_pop();
        context_prepare(&t_snippet->filter_info_); // 임의 init

        string block_dir = t_snippet->block_dir_;

        int fd = open(block_dir.c_str(), O_RDONLY); 
        if (fd < 0){
            printf("cannot open block file %s\n", block_dir.c_str());
            return;
        }

        for(int i=0; i<t_snippet->chunks_.size(); i++){
            uint64_t block_dba = t_snippet->chunks_[i].block_dba_;
            uint64_t block_size = t_snippet->chunks_[i].block_size_;

            char buf[block_size];

            if (read(fd, buf, block_size) != block_size){ 
                printf("cannot read from block file %s\n", block_dir.c_str());
                close(fd);
                return;
            }

            close(fd);

            bool pass = true;
            int tcnum = 0, row_num = 0,col_cnt = 0, out_rows = 0; 
            
            _blk_t *blk = (_blk_t *)buf;  // get block pointer

            if (table_block_verify(blk, block_dba, block_size) == 0){ // block verify
                pass = false;
            }

            res_chunk_t *res_chunk;

            res_chunk = csd_filter_and_eval_out(blk, &t_snippet->filter_info_); // scan, filter

            ///*debugg*/for(int t=0; t<res_chunk->res_len; t++){cout<<"key ";printf("%02X ",(u_char)res_chunk->res_buf[t]);}cout << endl;

            return_queue_->push_work(res_chunk);
        }
    }
}

void Worker::tmax_return(){
    while (1){
        res_chunk_t* res_chunk_t = return_queue_->wait_and_pop();
        int sockfd;
        struct sockaddr_in serv_addr;
        sockfd = socket(PF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("ERROR opening socket");
        }
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(T_SE_MERGING_TCP_PORT);

        if (inet_pton(AF_INET, "10.0.4.80", &serv_addr.sin_addr) <= 0) {
            perror("ERROR invalid IP address");
            close(sockfd);
            continue; // Skip to the next iteration of the loop
        }

        int response = connect(sockfd,(const sockaddr*)&serv_addr,sizeof(serv_addr));
        if( 0 != response ) {
            cout << "ERROR return interface connection failed" << endl;
        } 

        send(sockfd, &res_chunk_t->res_len, sizeof(int), 0);
        send(sockfd, &res_chunk_t->res_buf, res_chunk_t->res_len, 0);
        
        close(sockfd);
    }    
}

static void context_prepare(/*exp_ctx_t *exp_ctx, */filter_info_t *filter_info) // [K] exp_ctx, filter_info 정보를 채우는 함수 -> 해당 정보를 위에서 전달받아야 할듯
{
	int col_cnt = 6;		// c0 - c5 // [K] 하드코딩값 -> 전달 받을 정보
	int exp_cnt = 1;		// >= 6789 // [K] 하드코딩값 -> 전달 받을 정보

  	// expression context initialize
	// exp_ctx->column_vals = (uchar **)malloc(sizeof(uchar *) * col_cnt); // [K] 컬럼 개수만큼 공간 마련

	// filter info initialize
	filter_info->exp_cnt = exp_cnt; 
	filter_info->col_cnt = col_cnt;

		// operation GE (>=)
	filter_info->op_types =
					 (exp_op_type_t *)malloc(sizeof(exp_op_type_t));
	filter_info->op_types[0] = EXP_OP_GE;// [K] operator 지정

		// column (c0)
	filter_info->exp_l_nos = (int16_t *)malloc(sizeof(int16_t));// [K] left value
	filter_info->exp_l_nos[0] = 0; // [K] 하드코딩값 -> 전달 받을 정보
		// contant value (6789)
	filter_info->exp_r_nos = (int16_t *)malloc(sizeof(int16_t));// [K] right value
	filter_info->exp_r_nos[0] = -1; // [K] 하드코딩값 -> 전달 받을 정보
	filter_info->const_vals_cnt = 1; // [K] 하드코딩값 -> 전달 받을 정보
	filter_info->const_vals = (uchar **)malloc(sizeof(uchar *));
	filter_info->const_vals[0] = (uchar *)malloc(sizeof(uchar) * 3);
	filter_info->const_vals[0][0] = 3; // [K] 하드코딩값 -> 전달 받을 정보
	filter_info->const_vals[0][1] = 195; // [K] 하드코딩값 -> 전달 받을 정보
	filter_info->const_vals[0][2] = 194; // [K] 하드코딩값 -> 전달 받을 정보
} 

