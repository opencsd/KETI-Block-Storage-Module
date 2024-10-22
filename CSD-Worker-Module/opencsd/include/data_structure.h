#pragma once
#include <iostream>
#include <algorithm>

using namespace std;

struct kValue{
  int type_;
  string string_;
  int int_;
  int64_t int64_;
  float float_;
  double double_;
  bool bool_;
  int real_ = 0; // 소수점 길이
};

struct Operand {
  vector<int> types;
  vector<string> values;
};

struct Projectioning {
  int select_type; // KETI_SELECT_TYPE
  Operand expression;
};

struct Filtering {
  Operand lv;
  int operator_;
  Operand rv;
};

struct QueryInfo {
  string table_name;
  vector<Filtering> filtering;
  vector<Projectioning> projection;
  vector<string> seek_key;
  Filtering condition;
};

struct SchemaInfo {
  vector<string> column_name;
  vector<int> column_offset;
  vector<int> column_length;
  vector<int> column_type;
  unordered_map<string,int> column_index_map;
  vector<string> pk_column;
  vector<string> index_column;
  int table_index_number;
  bool has_var_char;
};

struct Block {
  vector<uint64_t> offset;
  vector<uint64_t> length;
};

struct BlockInfo {
  string partition;
  vector<Block> block_list;
  void clear(){
    block_list.clear();
  }
};

struct WalInfo {
  vector<string> deleted_key;
  vector<string> inserted_key;
  vector<string> inserted_value;

  void clear(){
    deleted_key.clear();
    inserted_key.clear();
    inserted_value.clear();
  }
};

struct ResultInfo {
  string table_alias;
  vector<string> column_alias;
  vector<int> return_column_type;
  vector<int> return_column_length;
  int total_block_count;
  int csd_block_count;
};

struct Snippet {
  int type;
  int query_id;
  int work_id;
  QueryInfo query_info;
  SchemaInfo schema_info;
  BlockInfo block_info;
  WalInfo wal_info;
  ResultInfo result_info;
};

struct Data {
  char raw_data[BUFFER_SIZE];
  vector<int> row_offset;
  size_t data_length;
  int row_count;
  int current_block_count;
  int scanned_row_count;
  int filtered_row_count;

  Data(){
    row_offset.clear();
    data_length = 0;
    row_count = 0;
    current_block_count = 0;
    scanned_row_count = 0;
    filtered_row_count = 0;
  }

  void clear(){
    memset(raw_data,0,BUFFER_SIZE);
    row_offset.clear();
    data_length = 0;
    row_count = 0;
    current_block_count = 0;
  }
};

struct CsdResult {
  shared_ptr<Snippet> snippet;
  Data data;

  CsdResult() : snippet(nullptr) {}
  CsdResult(shared_ptr<Snippet> snippet_, int scanned_row_count = 0, int current_block_count = 0)
   : snippet(snippet_) {
    data.scanned_row_count = scanned_row_count;
    data.current_block_count = current_block_count;
   }
};

struct MergeBuffer {
  map<string, CsdResult> id_buffer_map;
  mutex mu;

  MergeBuffer(){}

  void check_id_buffer(string key, shared_ptr<Snippet> snippet){
    unique_lock<mutex> lock(mu);

    if(id_buffer_map.find(key)==id_buffer_map.end()){
      CsdResult merge_result = CsdResult(snippet);
      id_buffer_map.insert({key,merge_result});
    }
  }

  void release_buffer(string key){
    unique_lock<mutex> lock(mu);
    
    if (id_buffer_map.find(key) != id_buffer_map.end()) {
        id_buffer_map.erase(key);
    }
  }
};

struct tSnippet {

};