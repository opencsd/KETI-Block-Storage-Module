#pragma once

#include "header.h"

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

struct chunk{
  uint64_t block_dba_;
  uint64_t block_size_;
};

struct t_snippet{
    int id_;
    vector<chunk> chunks_;
    string block_dir_;
    filter_info_t filter_info_;

    t_snippet(const char* json_){
        Document document;
        document.Parse(json_);
        
        id_ = document["id"].GetInt();
        
        chunks_.clear();
        Value &chunks = document["chunks"];
        for(int i = 0; i<chunks.Size(); i++){
            chunk chunk;
            chunk.block_dba_ = chunks[i]["block_dba"].GetInt64();
            chunk.block_size_ = chunks[i]["block_size"].GetInt64();
			chunks_.push_back(chunk);
        }   

        block_dir_ = document["block_dir"].GetString();

        context_prepare(&filter_info_); // 임의 init
    }
};