#pragma once
#include <stack>
#include <bitset>
#include <math.h>
#include <mutex>

#include "return.h"
#include "id_map.h"

struct Result;
struct FilterInfo;
class MergeManager;

extern WorkQueue<Result> MergeQueue;
extern WorkQueue<MergeResult> ReturnQueue;
extern IdMap<MergeManager> MergeIdMap;

struct T{
  string varString;
  int varInt;
  int64_t varLong;
  float varFloat;
  double varDouble;
  int real_size = 0;//소수점 길이//여기추가
};

class MergeManager{
public:
    void Merging();
    void MergeBlock(Result &result);

    inline const static std::string LOGTAG = "CSD Merge Manager";
    char msg[200];
    int ID;

    void EraseKey(const pair_key key);

private:
    unordered_map<pair_key, MergeResult, pair_hash> m_MergeManager;// key=<qid,wid>

    pair<T,int> stack_charToValue(char* dest, int type, int len);
    int stack_valueToChar(char* dest, int dest_type, T value, int type);
    void getColOffset(const char *origin_row_data, FilterInfo filter_info, int* col_offset);
    int calculCase(FilterInfo filter_info, char* origin_row_data, int* col_offset, int l, char* dest);
    int calculSubstring(FilterInfo filter_info, char* origin_row_data, int* col_offset, int l, char* dest);
    int calculExtract(FilterInfo filter_info, char* origin_row_data, int* col_offset, int l, char* dest);
    int calculPostfix(vector<string> values, vector<int> types, FilterInfo filter_info, char* origin_row_data, int* col_offset, char* dest, int projection_datatype);
};


struct Projection{
    int projection; // KETI_SELECT_TYPE
    vector<string> values;
    vector<int> types; // KETI_ValueType
};

struct FilterInfo{
    vector<string> table_col;//스캔테이블
    vector<int> table_offset;
    vector<int> table_offlen;
    vector<int> table_datatype;
    unordered_map<string, int> colindexmap;//col index
    string table_filter;//where절 정보
    vector<Projection> column_projection;//select절 정보
    vector<int> projection_datatype;//*컬럼 프로젝션 후 컬럼의 데이터타입
    vector<int> projection_length;//*컬럼 프로젝션 후 컬럼의 길이
    
    FilterInfo(){}
    FilterInfo(
      vector<string> table_col_, vector<int> table_offset_, 
      vector<int> table_offlen_, vector<int> table_datatype_,
      unordered_map<string, int> colindexmap_, string table_filter_,
      vector<Projection> column_projection_, vector<int> projection_datatype_,
      vector<int> projection_length_)
      : table_col(table_col_),
        table_offset(table_offset_),
        table_offlen(table_offlen_),
        table_datatype(table_datatype_),
        colindexmap(colindexmap_),
        table_filter(table_filter_),
        column_projection(column_projection_),
        projection_datatype(projection_datatype_),
        projection_length(projection_length_){}
};

struct Result{
    int query_id;
    int work_id;
    string csd_name;
    FilterInfo filter_info;
    string storage_engine_port;
    int sst_total_block_count;
    int csd_total_block_count;
    int table_total_block_count;
    string table_alias;
    vector<string> column_alias;
    int row_count;
    int length;
    char data[BUFF_SIZE];
    vector<int> row_offset;
    vector<vector<int>> row_column_offset;
    int result_block_count;
    int scanned_row_count;
    int filtered_row_count;
    bool is_debug_mode;

    //scan, filter의 최초 생성자
    Result(
      int query_id_, int work_id_, string csd_name_, 
      FilterInfo filter_info_, string storage_engine_port_, 
      int sst_total_block_count_, int csd_total_block_count_, int table_total_block_count_,
      string table_alias_, vector<string> column_alias_, 
      bool is_debug_mode_  = false, int result_block_count_ = 0, 
      int scanned_row_count_ = 0, int filtered_row_count_ = 0){
          query_id = query_id_;
          work_id = work_id_; 
          csd_name = csd_name_;
          filter_info = filter_info_;
          storage_engine_port = storage_engine_port_;
          sst_total_block_count = sst_total_block_count_;
          csd_total_block_count = csd_total_block_count_;
          table_total_block_count = table_total_block_count_;
          table_alias = table_alias_;
          column_alias = column_alias_;
          row_count = 0;
          length = 0;
          memset(&data, 0, sizeof(BUFF_SIZE));
          row_offset.clear();
          row_column_offset.clear();
          is_debug_mode = is_debug_mode_;
          result_block_count = result_block_count_;
          scanned_row_count = scanned_row_count_;
          filtered_row_count = filtered_row_count_;
        } 
 
    void InitResult(){
      row_count = 0;
      length = 0;
      memset(&data, 0, sizeof(BUFF_SIZE));
      row_offset.clear();
      row_column_offset.clear();
      result_block_count = 0;
      scanned_row_count = 0;
      filtered_row_count = 0;
    }
};

inline std::string& rtrim_(std::string& s, const char* t = " \t\n\r\f\v\0"){
	s.erase(s.find_last_not_of(t) + 1);
	return s;
}

inline std::string& ltrim_(std::string& s, const char* t = " \t\n\r\f\v\0"){
	s.erase(0, s.find_first_not_of(t));
	return s;
}

inline std::string& trim_(std::string& s, const char* t = " \t\n\r\f\v\0"){
	return ltrim_(rtrim_(s, t), t);
}