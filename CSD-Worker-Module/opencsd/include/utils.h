#pragma once

#include "header.h"

using namespace std;

inline vector<string> split(string input, char delimiter){
    vector<string> parts;
    stringstream ss(input);
    string temp;

    while(getline(ss, temp, delimiter)) {
        if (!temp.empty()) {
            parts.push_back(temp);
        }
    }

    return parts;
}

inline string make_map_key(int query_id, int work_id){
    return to_string(query_id) + "|" + to_string(work_id);
}

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

inline std::string convert_to_lowercase(std::string target){
    string dest = target;
    transform(dest.begin(), dest.end(), dest.begin(), ::tolower);
    return dest;
}

inline void calcul_column_offset(const char* origin_row_data, SchemaInfo& schema_info, vector<int>& column_offset_list){
    int type, length, offset, new_offset = 0;
    int col_count = schema_info.column_name.size();
    int tune = 0;
    
    column_offset_list.clear();
    
    for(int i=0; i<col_count; i++){
        type = schema_info.column_type[i];
        length = schema_info.column_length[i];
        offset = schema_info.column_offset[i];
           
        new_offset = offset + tune;

        if(type == MySQL_DataType::MySQL_VARSTRING){
            if(length < 256){//0~255
                char var_len[1];
                var_len[0] = origin_row_data[new_offset];
                uint8_t var_len_ = *((uint8_t *)var_len);
                tune += var_len_ + 1 - length;
            }else{//0~65535
                char var_len[2];
                var_len[0] = origin_row_data[new_offset];
                var_len[1] = origin_row_data[new_offset + 1];
                uint16_t var_len_ = *((uint16_t *)var_len);
                tune += var_len_ + 2 - length;
            }
        }

        column_offset_list.push_back(new_offset);
    }
}
