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
    void read_data_block(CsdResult &scan_result, shared_ptr<Snippet> snippet, vector<uint64_t> offset, vector<uint64_t> length, rocksdb::Slice& seek_key, int& left_block_count, int& key_index);
    void data_block_full_scan(shared_ptr<Snippet> snippet);
    void read_index_block(CsdResult &scan_result, shared_ptr<Snippet> snippet, vector<uint64_t> offset, vector<uint64_t> length, rocksdb::Slice& seek_key, int& left_block_count, int& key_index);   
    void data_block_index_scan(shared_ptr<Snippet> snippet);  
    void index_block_scan(shared_ptr<Snippet> snippet);  
    void sst_file_full_scan(shared_ptr<Snippet> snippet);
    void scan_wal(shared_ptr<Snippet> snippet);
    void enqueue_scan_result(CsdResult scan_result, bool flag = false);
    
    void generate_seek_key(int number, char* table_index_number);
    string convert_key_to_value(const rocksdb::Slice& key, SchemaInfo& schema_info);

    void enqueue_scan(shared_ptr<Snippet> snippet){
        scan_queue_->push_work(snippet);
    }
    
private:
    Filter* filter_layer_;
    Projection* projection_layer_;
    WorkQueue<shared_ptr<Snippet>>* scan_queue_;

    inline const static std::string LOGTAG = "CSD Scan";
    char msg[100];
};