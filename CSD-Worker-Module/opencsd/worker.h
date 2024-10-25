#pragma once

#include "return.h"
#include "data_structure.h"
#include "tb_block.h" // tmax library

#define T_SE_MERGING_TCP_PORT 40209

static void context_prepare(/*exp_ctx_t *exp_ctx, */filter_info_t *filter_info);

#define TPRINTF(tcnum, tctag, ...)\
	printf("[TEST CASE %d] %s | ", tcnum, tctag);\
	printf(__VA_ARGS__); printf("\n");

struct chunk{
  uint64_t block_dba_;
  uint64_t block_size_;
};

struct tSnippet {
  int id_;
    vector<chunk> chunks_;
    string block_dir_;
    filter_info_t filter_info_;

    tSnippet(const char* json_){
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
    }
};

class Worker{
    public:
        Worker(){
            this->work_queue_ = new WorkQueue<shared_ptr<tSnippet>>;
            this->return_queue_ = new WorkQueue<res_chunk_t*>;
        }
        void tmax_working();
        void tmax_return();

        ~Worker(){
            delete return_queue_;
            delete work_queue_;
        }

        void enqueue_worker(shared_ptr<tSnippet> snippet){
            work_queue_->push_work(snippet);
        }

        void enqueue_return(res_chunk_t* result){
            return_queue_->push_work(result);
        }

    private:
        WorkQueue<shared_ptr<tSnippet>>* work_queue_;
        WorkQueue<res_chunk_t*>* return_queue_;
};
