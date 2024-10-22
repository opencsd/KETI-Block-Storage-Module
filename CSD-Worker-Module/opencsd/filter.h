#pragma once

#include "projection.h"
#include "header.h"
#include "raw_data_handler.h"

using namespace std;

class Filter{
public:
    Filter(Projection* projection_layer){
        this->projection_layer_ = projection_layer;
        this->filter_queue_ = new WorkQueue<CsdResult>;
    }

    ~Filter(){
        delete filter_queue_;
    }

    void filter_worker();
    void filtering(CsdResult& result);
    
    bool compare_ge(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list);
    bool compare_le(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list);
    bool compare_gt(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list);
    bool compare_lt(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list);
    bool compare_eq(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list);
    bool compare_ne(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list);
    bool compare_like(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list);
    bool compare_not_like(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list);
    bool compare_between(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list);
    bool compare_in(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list);
    bool compare_not_in(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list);
    bool compare_is(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list);
    bool compare_is_not(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list);

    void enqueue_filter(CsdResult result){
        filter_queue_->push_work(result);
    }
        
private:
    Projection* projection_layer_;
    WorkQueue<CsdResult>* filter_queue_;

    inline const static std::string LOGTAG = "CSD Filter";
    char msg[200];
};