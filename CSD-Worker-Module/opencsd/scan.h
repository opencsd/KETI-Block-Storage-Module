#pragma once

#include "filter.h"
#include "header.h"

class Scan{
public:
    Scan(Filter* filter_layer, Projection* projection_layer){
        this->filter_layer_ = filter_layer;
        this->projection_layer_ = projection_layer;
        this->scan_queue_ = new WorkQueue<shared_ptr<Snippet>>;
    }

    ~Scan(){
        delete scan_queue_;
    }
    
    void scan_worker();
    void data_block_full_scan(shared_ptr<Snippet> snippet);
    void data_block_index_scan(shared_ptr<Snippet> snippet);  
    void index_block_scan(shared_ptr<Snippet> snippet);  
    void scan_wal(shared_ptr<Snippet> snippet);
    void enqueue_scan_result(Result scan_result);
    
    void generate_seek_key(SchemaInfo& schema_info, char* table_index_number);
    string convert_key_to_value(const rocksdb::Slice& key, SchemaInfo& schema_info);

    void enqueue_scan(shared_ptr<Snippet> snippet){
        scan_queue_->push_work(snippet);
    }
    
private:
    Filter* filter_layer_;
    Projection* projection_layer_;
    WorkQueue<shared_ptr<Snippet>>* scan_queue_;

    inline const static std::string LOGTAG = "CSD Scan";
    char msg[200];
};