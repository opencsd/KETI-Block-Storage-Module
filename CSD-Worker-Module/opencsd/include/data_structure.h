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
  int offset;
  vector<int> length;
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

  Data(){
    row_offset.clear();
    data_length = 0;
    row_count = 0;
    current_block_count = 0;
  }

  void clear(){
    memset(raw_data,0,BUFFER_SIZE);
    row_offset.clear();
    data_length = 0;
    row_count = 0;
    current_block_count = 0;
  }
};

struct Result {
  shared_ptr<Snippet> snippet;
  Data data;

  Result() : snippet(nullptr) {}
  Result(shared_ptr<Snippet> snippet_) : snippet(snippet_) {}
};

struct MergeBuffer {
  map<string, Result> id_buffer_map;
  mutex mu;

  MergeBuffer(){}

  void check_id_buffer(string key){
    unique_lock<mutex> lock(mu);

    if(id_buffer_map.find(key)==id_buffer_map.end()){
      Result merge_result = Result();
      id_buffer_map.insert({key,merge_result});
    }
  }

  void release_buffer(string key){
    unique_lock<mutex> lock(mu);

    auto it = id_buffer_map.find(key);
    if (it != id_buffer_map.end()) {
      id_buffer_map.erase(it);
    }
  }
};

struct tSnippet {

};