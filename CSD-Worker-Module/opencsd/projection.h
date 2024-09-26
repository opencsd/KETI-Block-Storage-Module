#pragma once

#include "return.h"
#include "header.h"
#include "raw_data_handler.h"

class Projection{
public:
    Projection(Return* return_layer){
        this->return_layer_ = return_layer;
        this->projection_queue_ = new WorkQueue<Result>;
    }

    void projection_worker();
    void projectioning(MergeBuffer* merge_buffer, Result& result);
    void make_block_count_map(string key, int total_block_count);
    void block_count_down_and_release_buffer(string key, int block_count);

    void enqueue_projection(Result result){
        projection_queue_->push_work(result);
    }

private:
    Return* return_layer_;
    WorkQueue<Result>* projection_queue_;
    vector<MergeBuffer*> merge_buffer_list_;
    map<string, int> id_block_count_map_;
    mutex mu;

    inline const static std::string LOGTAG = "CSD Merge Manager";
    char msg[200];
};