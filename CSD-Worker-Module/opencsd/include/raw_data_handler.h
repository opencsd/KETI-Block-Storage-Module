#pragma once

#include "header.h"
#include "utils.h"

using namespace std;

inline kValue calcul_plus_operation(kValue lv, kValue rv, string oper){
    kValue result;

    switch(lv.type_){
        case KETI_VALUE_TYPE::INT32:{
            if(rv.type_ == KETI_VALUE_TYPE::DECIMAL){
                int base = static_cast<int>(pow(100, rv.real_));
                result.int64_ = (lv.int_ * base) + rv.int64_;
                result.real_ = rv.real_;
                result.type_ = KETI_VALUE_TYPE::DECIMAL;
            }else{
                result.int_ = lv.int_ + rv.int_;
                result.type_ = lv.type_;
            }
            break;
        }case KETI_VALUE_TYPE::INT64:
        case KETI_VALUE_TYPE::DECIMAL:{
            result.int64_ = lv.int64_ + rv.int64_;
            result.type_ = lv.type_;
            break;
        }case KETI_VALUE_TYPE::FLOAT32:{
            result.float_ = lv.float_ + rv.float_;
            result.type_ = lv.type_;
            break;
        }case KETI_VALUE_TYPE::DOUBLE:{
            result.double_ = lv.double_ + rv.double_;
            result.type_ = lv.type_;
            break;
        }default:{
            KETILOG::ERRORLOG("raw_data_handler", "@error undefined calcul_plus_operation type");
            break;
        }
    }

    return result;
}

inline kValue calcul_minus_operation(kValue lv, kValue rv, string oper){
    kValue result;

    switch(lv.type_){
        case KETI_VALUE_TYPE::INT32:{
            if(rv.type_ == KETI_VALUE_TYPE::DECIMAL){
                int base = static_cast<int>(pow(100, rv.real_));
                result.int64_ = (lv.int_ * base) - rv.int64_;
                result.real_ = rv.real_;
                result.type_ = KETI_VALUE_TYPE::DECIMAL;
            }else{
                result.int_ = lv.int_ - rv.int_;
                result.type_ = lv.type_;
            }
            break;
        }case KETI_VALUE_TYPE::INT64:
        case KETI_VALUE_TYPE::DECIMAL:{
            result.int64_ = lv.int64_ - rv.int64_;
            result.type_ = lv.type_;
            break;
        }case KETI_VALUE_TYPE::FLOAT32:{
            result.float_ = lv.float_ - rv.float_;
            result.type_ = lv.type_;
            break;
        }case KETI_VALUE_TYPE::DOUBLE:{
            result.double_ = lv.double_ - rv.double_;
            result.type_ = lv.type_;
            break;
        }default:{
            KETILOG::ERRORLOG("raw_data_handler", "@error undefined calcul_plus_operation type");
            break;
        }
    }
    
    return result;
}

inline kValue calcul_multiple_operation(kValue lv, kValue rv, string oper){
    kValue result;

    switch(lv.type_){
        case KETI_VALUE_TYPE::INT32:{
            result.int_ = lv.int_ * rv.int_;
            break;
        }case KETI_VALUE_TYPE::INT64:{
            result.int64_ = lv.int64_ * rv.int64_;
            break;
        }case KETI_VALUE_TYPE::DECIMAL:{
            if(rv.type_ == KETI_VALUE_TYPE::INT32){
                int base = static_cast<int>(pow(100, lv.real_));
                result.int64_ = lv.int64_ * (rv.int_ * base);
                result.real_ = lv.real_;
            }else{
                result.int64_ = lv.int64_ * rv.int64_;
                result.real_ = lv.real_ + rv.real_;
            }
            break;
        }case KETI_VALUE_TYPE::FLOAT32:{
            result.float_ = lv.float_ * rv.float_;
            break;
        }case KETI_VALUE_TYPE::DOUBLE:{
            result.double_ = lv.double_ * rv.double_;
            break;
        }default:{
            KETILOG::ERRORLOG("raw_data_handler", "@error undefined calcul_plus_operation type");
            break;
        }
    }
    result.type_ = lv.type_;
    
    return result;
}

inline kValue calcul_divide_operation(kValue lv, kValue rv, string oper){
    kValue result;

    switch(lv.type_){
        case KETI_VALUE_TYPE::INT32:{
            result.int_ = lv.int_ / rv.int_;
            break;
        }case KETI_VALUE_TYPE::INT64:
        case KETI_VALUE_TYPE::DECIMAL:{
            result.int64_ = lv.int64_ / rv.int64_;
            break;
        }case KETI_VALUE_TYPE::FLOAT32:{
            result.float_ = lv.float_ / rv.float_;
            break;
        }case KETI_VALUE_TYPE::DOUBLE:{
            result.double_ = lv.double_ / rv.double_;
            break;
        }default:{
            KETILOG::ERRORLOG("raw_data_handler", "@error undefined calcul_plus_operation type");
            break;
        }
    }
    result.type_ = lv.type_;
    
    return result;
}

inline kValue calcul_equal_operation(kValue lv, kValue rv, string oper, bool not_falg = false){
    kValue result;
    result.type_ = KETI_VALUE_TYPE::BOOLEAN;

    switch(lv.type_){
        case KETI_VALUE_TYPE::INT32:{
            result.bool_ = (lv.int_ == rv.int_)? true : false;
            break;
        }case KETI_VALUE_TYPE::INT64:
        case KETI_VALUE_TYPE::DECIMAL:{
            result.bool_ = (lv.int64_ == rv.int64_)? true : false;
            break;
        }case KETI_VALUE_TYPE::FLOAT32:{
            result.bool_ = (lv.float_ == rv.float_)? true : false;
            break;
        }case KETI_VALUE_TYPE::DOUBLE:{
            result.bool_ = (lv.double_ == rv.double_)? true : false;
            break;
        }case KETI_VALUE_TYPE::STRING:{
            lv.string_ = convert_to_lowercase(lv.string_);
            rv.string_ = convert_to_lowercase(rv.string_);            
            result.bool_ = (lv.string_ == rv.string_)? true : false;
            break;
        }default:{
            KETILOG::ERRORLOG("raw_data_handler", "@error undefined calcul_plus_operation type");
            break;
        }
    }

    if(not_falg){
        result.bool_ = !result.bool_;
    }
    
    return result;
}

inline kValue calcul_not_equal_operation(kValue lv, kValue rv, string oper){
    return calcul_equal_operation(lv,rv,oper,true);
}

inline kValue get_column_value(const char* origin_row_data, SchemaInfo& schema_info, Operand& operand, vector<int>& column_offset_list, int index = 0){
    kValue result;
    string column_name = operand.values[index];
    int column_index = schema_info.column_index_map[column_name];
    int column_type = schema_info.column_type[column_index];
    int start_offset = column_offset_list[column_index];
    int end_offset = column_offset_list[column_index+1];
    int column_length = end_offset-start_offset;
    // cout << column_name << " " << column_index << " "  << start_offset << " " << end_offset << " " << column_length << " " << column_type << " " << endl;
    int tune = -1;
    
    switch(column_type){
        case MySQL_DataType::MySQL_BYTE:
        case MySQL_DataType::MySQL_INT16:
        case MySQL_DataType::MySQL_INT32:
        case MySQL_DataType::MySQL_DATE:{
            char column_data[4] = {0};
            memcpy(column_data, origin_row_data+start_offset, column_length);
            result.int_ = *((int32_t *)column_data);
            result.type_ = KETI_VALUE_TYPE::INT32;
            break;
        }case MySQL_DataType::MySQL_INT64:{
            char column_data[8] = {0};
            memcpy(column_data, origin_row_data+start_offset, column_length);
            result.int64_ = *((int64_t *)column_data);
            result.type_ = KETI_VALUE_TYPE::INT64;
            break;
        }case MySQL_DataType::MySQL_NEWDECIMAL:{
            //tpch decimal(15,2)만 고려한 상황 -> col_len = 7 (integer:6/real:1)
            char column_data[column_length] = {0};
            memcpy(column_data, origin_row_data+start_offset, column_length);

            bool is_negative = false;
            if(std::bitset<8>(column_data[0])[7] == 0){//음수일때 not +1
                is_negative = true;
                for(int i = 0; i<7; i++){
                    column_data[i] = ~column_data[i];//not연산
                }
            }
            column_data[0] ^= 0x80;
            
            char integer[8];
            char real[1];
            memset(integer, 0, 8);
            for(int k=0; k<6; k++){
                integer[k] = column_data[5-k];
            }
            real[0] = column_data[6];

            int64_t ivalue = *((int64_t *)integer); 
            int8_t rvalue = *((int8_t *)real);
            int64_t decimal_to_int = ivalue * 100 + rvalue;
            if(is_negative){
                decimal_to_int *= -1;
            }
            result.int64_ = decimal_to_int;
            result.real_ = 1;
            result.type_ = KETI_VALUE_TYPE::DECIMAL;
            break;
        }case MySQL_DataType::MySQL_STRING:{
            tune = 0;
        }case MySQL_DataType::MySQL_VARSTRING:{
            if(tune < 0){
                tune = (column_length < 255) ? 1 : 2;
            }
            char column_data[column_length-tune];
            memcpy(column_data, origin_row_data+start_offset+tune, column_length);
            string raw_value(column_data, column_length-tune);
            result.string_ = trim_(raw_value);
            result.type_ = KETI_VALUE_TYPE::STRING;
            break;
        }case MySQL_DataType::MySQL_FLOAT32:
        case MySQL_DataType::MySQL_DOUBLE:
        case MySQL_DataType::MySQL_TIMESTAMP:
        default:{
            KETILOG::ERRORLOG("raw_data_handler", "@error undefined get_column_value type");
            break;
        }
    }
    return result;
} 

inline kValue get_literal_value(const char* origin_row_data, SchemaInfo& schema_info, Operand& operand, vector<int>& column_offset_list, int index){
    kValue result;

    switch(operand.types[index]){
        case KETI_VALUE_TYPE::INT8:
        case KETI_VALUE_TYPE::INT16:
        case KETI_VALUE_TYPE::INT32:
        case KETI_VALUE_TYPE::DATE:{
            result.int_ = stoi(operand.values[index]);
            result.type_ = KETI_VALUE_TYPE::INT32;
            break;
        }case KETI_VALUE_TYPE::INT64:{
            result.int64_ = stoll(operand.values[index]);
            result.type_ = KETI_VALUE_TYPE::INT64;
            break;
        }case KETI_VALUE_TYPE::FLOAT32:{
            result.double_ = stof(operand.values[index]);
            result.type_ = KETI_VALUE_TYPE::FLOAT32;
            break;
        }case KETI_VALUE_TYPE::DOUBLE:{
            result.double_ = stod(operand.values[index]);
            result.type_ = KETI_VALUE_TYPE::DOUBLE;
            break;
        }case KETI_VALUE_TYPE::DECIMAL:{
            result.double_ = stod(operand.values[index]);
            result.int64_ = result.double_ * 100;
            result.real_ = 1;
            result.type_ = KETI_VALUE_TYPE::DECIMAL;
            break;
        }case KETI_VALUE_TYPE::STRING:{
            result.string_ = operand.values[index];
            result.type_ = KETI_VALUE_TYPE::STRING;
            break;
        }default:{
            KETILOG::ERRORLOG("raw_data_handler", "@error undefined get_literal_value type");
            break;
        }
    }

    return result;
}

inline kValue get_postfix_value(const char* origin_row_data, SchemaInfo& schema_info, Operand& operand, vector<int>& column_offset_list){
    kValue result;
    stack<kValue> oper_stack;

    for(int i=0; i<operand.types.size(); i++){
        kValue median;
        int type = operand.types[i];
        string value = operand.values[i];

        if(type != KETI_VALUE_TYPE::OPERATOR){
            if(type == KETI_VALUE_TYPE::COLUMN){
                median = get_column_value(origin_row_data, schema_info, operand, column_offset_list,i);
            }else{
                median = get_literal_value(origin_row_data, schema_info, operand, column_offset_list,i);
            }
        }else{
            kValue rv = oper_stack.top();
            oper_stack.pop();
            kValue lv = oper_stack.top();
            oper_stack.pop();

            if(value == "+"){
                median = calcul_plus_operation(lv,rv,value);
            }else if(value == "-"){
                median = calcul_minus_operation(lv,rv,value);
            }else if(value == "*"){
                median = calcul_multiple_operation(lv,rv,value);
            }else if(value == "/"){ 
                median = calcul_divide_operation(lv,rv,value);
            }else if(value == "="){
                median = calcul_equal_operation(lv,rv,value);
            }else if(value == "<>"){
                median = calcul_not_equal_operation(lv,rv,value);
            }else{
                KETILOG::ERRORLOG("raw_data_handler", "@error undefined get_postfix_value type");
            }
        }
        oper_stack.push(median);
    }

    result = oper_stack.top();
    oper_stack.pop();

    return result;
}

inline kValue calcul_substring_operation(const char* origin_row_data, SchemaInfo& schema_info, Operand& operand, vector<int>& column_offset_list){
    //["SUBSTRING","c_phone","1","2"]
    kValue result;
    
    string column_name = operand.values[1];
    int column_index = schema_info.column_index_map[column_name];
    int start_offset = column_offset_list[column_index];

    int from = stoi(operand.values[2]) - 1;
    int to = stoi(operand.values[3]);

    char column_data[to];
    memcpy(column_data, origin_row_data+start_offset+from, to);

    result.string_ = string(column_data,to);
    result.type_ = KETI_VALUE_TYPE::STRING;

    return result;
}

inline kValue calcul_case_operation(const char* origin_row_data, SchemaInfo& schema_info, Operand& operand, vector<int>& column_offset_list){
    //["CASE","WHEN","o_orderpriority","1-urgent","=","OR","o_orderpriority","2-high","=","THEN","1","ELSE","0","END"]
    //["CASE","WHEN","o_orderpriority","1-urgent","<>","AND","o_orderpriority","2-high","<>","THEN","1","ELSE","0","END"]
    Operand sliced_operand;
    vector<pair<int,int>> when_then_offset;
    int else_offset, when, then = 0;

    for(int i = 0; i<operand.values.size(); i++){
        if(operand.values[i] == "WHEN"){
            when = i;
        }else if(operand.values[i] == "THEN"){
            then = i;
            when_then_offset.push_back({when,then});
        }else if(operand.values[i] == "ELSE"){
            else_offset = i;
        }
    }

    int index;
    bool passed = false;
    vector<pair<int,int>>::iterator iter;
    for (iter = when_then_offset.begin(); iter != when_then_offset.end(); ++iter){
        when = (*iter).first;
        then = (*iter).second;
        index = when + 1;

        while(index < then+1){
            if(operand.values[index] == "AND"){
                kValue value;
                value = get_postfix_value(origin_row_data, schema_info, sliced_operand, column_offset_list);
                if(value.bool_ == true){
                    sliced_operand.values.clear();
                    sliced_operand.types.clear(); 
                }else{
                    passed = false;
                    break;
                }
            }else if(operand.values[index] == "OR"){
                kValue value;
                value = get_postfix_value(origin_row_data, schema_info, sliced_operand, column_offset_list);
                if(value.bool_ == true){
                    passed = true;
                    break;
                }else{
                    sliced_operand.values.clear();
                    sliced_operand.types.clear(); 
                } 
            }else if(operand.values[index] == "THEN"){
                kValue value;
                value = get_postfix_value(origin_row_data, schema_info, sliced_operand, column_offset_list);
                passed = value.bool_;
            }else{
                sliced_operand.values.push_back(operand.values[index]);
                sliced_operand.types.push_back(operand.types[index]);
            }
            index++;
        }

        if(passed){
            index = then + 1;
            sliced_operand.values.clear();
            sliced_operand.types.clear(); 
            while(true){
                if(operand.values[index] == "WHEN" || 
                    operand.values[index] == "ELSE" || 
                    operand.values[index] == "END"){
                    break;
                }else{
                    sliced_operand.values.push_back(operand.values[index]);
                    sliced_operand.types.push_back(operand.types[index]);
                }
                index++;
            }

            kValue value;
            value = get_postfix_value(origin_row_data, schema_info, sliced_operand, column_offset_list);
            
            return value;
        }
    }
    
    //else 처리
    sliced_operand.values.clear();
    sliced_operand.types.clear(); 
    index = else_offset + 1;
    while(true){
        if(operand.values[index] == "END"){
            break;
        }else{
            sliced_operand.values.push_back(operand.values[index]);
            sliced_operand.types.push_back(operand.types[index]);
        }
        index++;
    }

    kValue value;
    value = get_postfix_value(origin_row_data, schema_info, sliced_operand, column_offset_list);
    
    return value;
}

inline kValue calcul_extract_operation(const char* origin_row_data, SchemaInfo& schema_info, Operand& operand, vector<int>& column_offset_list){
    //["EXTRACT","YEAR",col_name]
    kValue result;
    result.type_ = KETI_VALUE_TYPE::INT32;
    string unit = convert_to_lowercase(operand.values[1]);

    kValue colum_value = get_column_value(origin_row_data, schema_info, operand, column_offset_list, 2);
    int date = colum_value.int_;

    if(unit == "year"){
        result.int_ = date / 512;
    }else if(unit == "month"){
        date %= 512;
        result.int_ = date / 32;
    }else if(unit == "day"){
        date %= 512;
        date %= 32;
        result.int_ = date;
    }else{
        KETILOG::ERRORLOG("raw_data_handler", "@error undefined calcul_extract_operation type");
    }

    return result;
}

inline kValue get_derived_value(const char* origin_row_data, SchemaInfo& schema_info, Operand& operand, vector<int>& column_offset_list){
    kValue result;
    string lower_operator = convert_to_lowercase(operand.values[0]);
    
    if(lower_operator == "substring" || lower_operator == "substr"){//현재는 substring("src", pos, len) 만 처리
        result = calcul_substring_operation(origin_row_data, schema_info, operand, column_offset_list);
    }else if(lower_operator == "case"){
        result = calcul_case_operation(origin_row_data, schema_info, operand, column_offset_list);
    }else if(lower_operator == "extract"){
        result = calcul_extract_operation(origin_row_data, schema_info, operand, column_offset_list);
    }else{
        KETILOG::ERRORLOG("raw_data_handler", "@error undefined get_derived_value type");
    }

    return result;
}


inline kValue calcul_operand_value(const char* origin_row_data, SchemaInfo& schema_info, Operand& operand, vector<int>& column_offset_list, int index = 0){
    kValue value;
    if(operand.types.size() == 1 || index != 0){
        if(operand.types[index] == KETI_VALUE_TYPE::COLUMN){
            value = get_column_value(origin_row_data, schema_info, operand, column_offset_list, index);
        }else{
            value = get_literal_value(origin_row_data, schema_info, operand, column_offset_list, index);
        }
    }else{
        if(operand.types[0] == KETI_VALUE_TYPE::OPERATOR){
            value = get_derived_value(origin_row_data, schema_info, operand, column_offset_list);
        }else{
            value = get_postfix_value(origin_row_data, schema_info, operand, column_offset_list);
        }
    }

    return value;
}

inline int convert_value_to_char(char* dest, kValue value, int offset){
    switch(value.type_){
         case KETI_VALUE_TYPE::INT32:{
            int src = value.int_;
            char out[4];
            memcpy(dest+offset, &src, sizeof(int));
            return 4;
        }case KETI_VALUE_TYPE::INT64:{
            int64_t src = value.int64_;
            char out[8];
            memcpy(dest+offset, &src, sizeof(int64_t));
            return 8;
        }case KETI_VALUE_TYPE::DECIMAL:{
            bool is_negative = false;
            if(value.int64_<0){
                is_negative = true;
                value.int64_ *= -1;
            }
            int64_t integer;
            int32_t real;
            int base = static_cast<int>(pow(100, value.real_));
             
            integer = value.int64_ / base;
            real = value.int64_ % base;

            char integer_char[8];
            char real_char[4];
            char reversed_integer_char[6];

            memcpy(integer_char, &integer, sizeof(int64_t));
            memcpy(real_char, &real, sizeof(int32_t));

            for(int i=0; i<6; i++){
                reversed_integer_char[5-i] = integer_char[i];
            }
            reversed_integer_char[0] ^= 0x80;

            if(is_negative){
                for(int i=0; i<7; i++){
                    dest[i] = ~dest[i];
                }
            }

            memcpy(dest+offset, reversed_integer_char, 6);
            memcpy(dest+offset+6, real_char, value.real_);

            return 6 + value.real_;            
        }case KETI_VALUE_TYPE::STRING:{
            string src = value.string_;
            int length = src.length();
            char char_array[length];
            strcpy(char_array, src.c_str());
            memcpy(dest+offset, char_array, length);
            return length;
        }default:{
            KETILOG::ERRORLOG("raw_data_handler", "@error undefined convert_value_to_char type");
            return 0;
        }
    }
}

inline int projection_raw_data(const char* origin_row_data, SchemaInfo& schema_info, Operand& operand, vector<int>& column_offset_list, char* dest, int current_row_length){
    int column_length;

    if(operand.types.size() == 1){
        string column_name = operand.values[0];
        int column_index = schema_info.column_index_map[column_name];
        int column_type = schema_info.column_type[column_index];
        int start_offset = column_offset_list[column_index];
        int end_offset = column_offset_list[column_index+1];
        column_length = end_offset-start_offset;
        memcpy(dest+current_row_length, origin_row_data+start_offset, column_length);
    }else{
        kValue value;
        value = calcul_operand_value(origin_row_data,schema_info,operand,column_offset_list);
        column_length = convert_value_to_char(dest, value, current_row_length);
    }
    return column_length;
}
