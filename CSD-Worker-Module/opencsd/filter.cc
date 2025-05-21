#include "filter.h"

void Filter::filter_worker(){
    while (1){
        CsdResult result = filter_queue_->wait_and_pop();

        filtering(result);
    }
}

void Filter::filtering(CsdResult& result){        
    CsdResult filter_result(result.snippet, result.data.scanned_row_count, 0, result.data.current_block_count);

    vector<int> column_offset_list = result.snippet->schema_info.column_offset;
    
    for(int i=0; i<result.data.row_count; i++){
        int row_length = result.data.row_offset[i+1] - result.data.row_offset[i];
        char origin_row_data[row_length];
        memcpy(origin_row_data, result.data.raw_data+result.data.row_offset[i], row_length);

        if(result.snippet->schema_info.has_var_char){
            calcul_column_offset(origin_row_data, result.snippet->schema_info, column_offset_list);
            column_offset_list.push_back(row_length);
        }

        bool passed = true;
        bool running = true;
        int index = 0;

        while((index < result.snippet->query_info.filtering.size()) && running){
            switch (result.snippet->query_info.filtering[index].operator_){ // assume and,or not mixed
                case KETI_OPER_TYPE::GE:{
                    passed = compare_ge(origin_row_data, result.snippet->schema_info, result.snippet->query_info.filtering[index], column_offset_list);
                    break;
                }case KETI_OPER_TYPE::LE:{
                    passed = compare_le(origin_row_data, result.snippet->schema_info, result.snippet->query_info.filtering[index], column_offset_list);
                    break;
                }case KETI_OPER_TYPE::GT:{
                    passed = compare_gt(origin_row_data, result.snippet->schema_info, result.snippet->query_info.filtering[index], column_offset_list);
                    break;
                }case KETI_OPER_TYPE::LT:{
                    passed = compare_lt(origin_row_data, result.snippet->schema_info, result.snippet->query_info.filtering[index], column_offset_list);
                    break;
                }case KETI_OPER_TYPE::EQ:{
                    passed = compare_eq(origin_row_data, result.snippet->schema_info, result.snippet->query_info.filtering[index], column_offset_list);
                    break;
                }case KETI_OPER_TYPE::NE:{
                    passed = compare_ne(origin_row_data, result.snippet->schema_info, result.snippet->query_info.filtering[index], column_offset_list);
                    break;
                }case KETI_OPER_TYPE::LIKE:{
                    passed = compare_like(origin_row_data, result.snippet->schema_info, result.snippet->query_info.filtering[index], column_offset_list);
                    break;
                }case KETI_OPER_TYPE::NOTLIKE:{
                    passed = compare_not_like(origin_row_data, result.snippet->schema_info, result.snippet->query_info.filtering[index], column_offset_list);
                    break;
                }case KETI_OPER_TYPE::BETWEEN:{
                    passed = compare_between(origin_row_data, result.snippet->schema_info, result.snippet->query_info.filtering[index], column_offset_list);
                    break;
                }case KETI_OPER_TYPE::IN:{
                    passed = compare_in(origin_row_data, result.snippet->schema_info, result.snippet->query_info.filtering[index], column_offset_list);
                    break;
                }case KETI_OPER_TYPE::NOTIN:{
                    passed = compare_not_in(origin_row_data, result.snippet->schema_info, result.snippet->query_info.filtering[index], column_offset_list);
                    break;
                }case KETI_OPER_TYPE::IS:{
                    passed = compare_is(origin_row_data, result.snippet->schema_info, result.snippet->query_info.filtering[index], column_offset_list);
                    break;
                }case KETI_OPER_TYPE::ISNOT:{
                    passed = compare_is_not(origin_row_data, result.snippet->schema_info, result.snippet->query_info.filtering[index], column_offset_list);
                    break;
                }case KETI_OPER_TYPE::AND:{
                    running = passed ? true : false;
                    break;
                }case KETI_OPER_TYPE::OR:{
                    running = passed ? false : true;
                    break;
                }default:{
                    KETILOG::ERRORLOG(LOGTAG, "@error undefined filter type");
                    break;
                }
            }
            index++;
        }  

        if(passed){
            filter_result.data.row_offset.push_back(filter_result.data.data_length);
            memcpy(filter_result.data.raw_data + filter_result.data.data_length, origin_row_data, row_length);
            filter_result.data.data_length += row_length;
            filter_result.data.row_count++;
            filter_result.data.filtered_row_count++;
        }
    }
    
    filter_result.data.row_offset.push_back(filter_result.data.data_length);
    //cout << "[Filter]filter process working {ID:" << to_string(filter_result.snippet->query_id) << "|" << to_string(filter_result.snippet->work_id) << "}... (filtered:" << to_string(filter_result.data.filtered_row_count) << "/" << to_string(filter_result.data.scanned_row_count) << ")" << endl;
    MonitoringManager::AddFilteredRow(filter_result.data.row_count);
    projection_layer_->enqueue_projection(filter_result);
}

bool Filter::compare_ge(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list){
    kValue lv = calcul_operand_value(origin_row_data, schema_info, filtering.lv, column_offset_list);
    kValue rv = calcul_operand_value(origin_row_data, schema_info, filtering.rv, column_offset_list);
    bool passed = false;

    switch(lv.type_){
        case KETI_VALUE_TYPE::INT32:{
            passed = (lv.int_ >= rv.int_) ? true : false;
            ///*debugg*/cout << lv.int_ << " >= " << rv.int_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::INT64:{
            passed = (lv.int64_ >= rv.int64_) ? true : false;
            ///*debugg*/cout << lv.int64_ << " >= " << rv.int64_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::FLOAT32:{
            passed = (lv.float_ >= rv.float_) ? true : false;
            ///*debugg*/cout << lv.float_ << " >= " << rv.float_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::DOUBLE:{
            passed = (lv.double_ >= rv.double_) ? true : false;
            ///*debugg*/cout << lv.double_ << " >= " << rv.double_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::STRING:{
            // lv.string_ = convert_to_lowercase(lv.string_);
            // rv.string_ = convert_to_lowercase(rv.string_);
            passed = (lv.string_ >= rv.string_) ? true : false;
            ///*debugg*/cout << lv.string_ << " >= " << rv.string_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::DECIMAL:{
            passed = (lv.int64_ >= rv.int64_) ? true : false;
            ///*debugg*/cout << lv.int64_ << " >= " << rv.int64_ << " : " << passed << endl;
            break;
        }default:{
            KETILOG::ERRORLOG(LOGTAG, "@error undefined compare_ge type");
            break;
        }
    }

    return passed;
}

bool Filter::compare_le(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list){
    kValue lv = calcul_operand_value(origin_row_data, schema_info, filtering.lv, column_offset_list);
    kValue rv = calcul_operand_value(origin_row_data, schema_info, filtering.rv, column_offset_list);
    bool passed = false;

    switch(lv.type_){
        case KETI_VALUE_TYPE::INT32:{
            passed = (lv.int_ <= rv.int_) ? true : false;
            ///*debugg*/cout << lv.int_ << " <= " << rv.int_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::INT64:{
            passed = (lv.int64_ <= rv.int64_) ? true : false;
            ///*debugg*/cout << lv.int64_ << " <= " << rv.int64_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::FLOAT32:{
            passed = (lv.float_ <= rv.float_) ? true : false;
            ///*debugg*/cout << lv.float_ << " <= " << rv.float_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::DOUBLE:{
            passed = (lv.double_ <= rv.double_) ? true : false;
            ///*debugg*/cout << lv.double_ << " <= " << rv.double_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::STRING:{
            // lv.string_ = convert_to_lowercase(lv.string_);
            // rv.string_ = convert_to_lowercase(rv.string_);
            passed = (lv.string_ <= rv.string_) ? true : false;
            ///*debugg*/cout << lv.string_ << " <= " << rv.string_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::DECIMAL:{
            passed = (lv.int64_ <= rv.int64_) ? true : false;
            ///*debugg*/cout << lv.int64_ << " <= " << rv.int64_ << " : " << passed << endl;
            break;
        }default:{
            KETILOG::ERRORLOG(LOGTAG, "@error undefined compare_le type");
            break;
        }
    }

    return passed;
}

bool Filter::compare_gt(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list){
    kValue lv = calcul_operand_value(origin_row_data, schema_info, filtering.lv, column_offset_list);
    kValue rv = calcul_operand_value(origin_row_data, schema_info, filtering.rv, column_offset_list);
    bool passed = false;

    switch(lv.type_){
        case KETI_VALUE_TYPE::INT32:{
            passed = (lv.int_ > rv.int_) ? true : false;
            ///*debugg*/cout << lv.int_ << " > " << rv.int_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::INT64:{
            passed = (lv.int64_ > rv.int64_) ? true : false;
            ///*debugg*/cout << lv.int64_ << " > " << rv.int64_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::FLOAT32:{
            passed = (lv.float_ > rv.float_) ? true : false;
            ///*debugg*/cout << lv.float_ << " > " << rv.float_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::DOUBLE:{
            passed = (lv.double_ > rv.double_) ? true : false;
            ///*debugg*/cout << lv.double_ << " > " << rv.double_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::STRING:{
            // lv.string_ = convert_to_lowercase(lv.string_);
            // rv.string_ = convert_to_lowercase(rv.string_);
            passed = (lv.string_ > rv.string_) ? true : false;
            ///*debugg*/cout << lv.string_ << " > " << rv.string_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::DECIMAL:{
            passed = (lv.int64_ > rv.int64_) ? true : false;
            ///*debugg*/cout << lv.int64_ << " > " << rv.int64_ << " : " << passed << endl;
            break;
        }default:{
            KETILOG::ERRORLOG(LOGTAG, "@error undefined compare_gt type");
            break;
        }
    }

    return passed;
}

bool Filter::compare_lt(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list){
    kValue lv = calcul_operand_value(origin_row_data, schema_info, filtering.lv, column_offset_list);
    kValue rv = calcul_operand_value(origin_row_data, schema_info, filtering.rv, column_offset_list);
    bool passed = false;

    switch(lv.type_){
        case KETI_VALUE_TYPE::INT32:{
            passed = (lv.int_ < rv.int_) ? true : false;
            ///*debugg*/cout << lv.int_ << " < " << rv.int_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::INT64:{
            passed = (lv.int64_ < rv.int64_) ? true : false;
            ///*debugg*/cout << lv.int64_ << " < " << rv.int64_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::FLOAT32:{
            passed = (lv.float_ < rv.float_) ? true : false;
            ///*debugg*/cout << lv.float_ << " < " << rv.float_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::DOUBLE:{
            passed = (lv.double_ < rv.double_) ? true : false;
            ///*debugg*/cout << lv.double_ << " < " << rv.double_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::STRING:{
            // lv.string_ = convert_to_lowercase(lv.string_);
            // rv.string_ = convert_to_lowercase(rv.string_);
            passed = (lv.string_ < rv.string_) ? true : false;
            ///*debugg*/cout << lv.string_ << " < " << rv.string_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::DECIMAL:{
            passed = (lv.int64_ < rv.int64_) ? true : false;
            ///*debugg*/cout << lv.int64_ << " < " << rv.int64_ << " : " << passed << endl;
            break;
        }default:{
            KETILOG::ERRORLOG(LOGTAG, "@error undefined compare_lt type");
            break;
        }
    }

    return passed;
}

bool Filter::compare_eq(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list){
    kValue lv = calcul_operand_value(origin_row_data, schema_info, filtering.lv, column_offset_list);
    kValue rv = calcul_operand_value(origin_row_data, schema_info, filtering.rv, column_offset_list);
    bool passed = false;

    switch(lv.type_){
        case KETI_VALUE_TYPE::INT32:{
            passed = (lv.int_ == rv.int_) ? true : false;
            ///*debugg*/cout << lv.int_ << " == " << rv.int_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::INT64:
        case KETI_VALUE_TYPE::DECIMAL:{
            passed = (lv.int64_ == rv.int64_) ? true : false;
            ///*debugg*/cout << lv.int64_ << " == " << rv.int64_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::FLOAT32:{
            passed = (lv.float_ == rv.float_) ? true : false;
            ///*debugg*/cout << lv.float_ << " == " << rv.float_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::DOUBLE:{
            passed = (lv.double_ == rv.double_) ? true : false;
            ///*debugg*/cout << lv.double_ << " == " << rv.double_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::STRING:{
            // lv.string_ = convert_to_lowercase(lv.string_);
            // rv.string_ = convert_to_lowercase(rv.string_);
            passed = (lv.string_ == rv.string_) ? true : false;
            ///*debugg*/cout << "'" << lv.string_ << "' " << " == '" << rv.string_ << "' : " << passed << endl;
            break;
        }default:{
            KETILOG::ERRORLOG(LOGTAG, "@error undefined compare_eq type");
            break;
        }
    }

    return passed;
}

bool Filter::compare_ne(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list){
    return !compare_eq(origin_row_data, schema_info, filtering, column_offset_list);
}

bool Filter::compare_like(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list){
    kValue lv = calcul_operand_value(origin_row_data, schema_info, filtering.lv, column_offset_list);
    kValue rv = calcul_operand_value(origin_row_data, schema_info, filtering.rv, column_offset_list);
    int start_offset = 0;
    bool passed = true;

    // lv.string_ = convert_to_lowercase(lv.string_);
    // rv.string_ = convert_to_lowercase(rv.string_);
    
    vector<string> parts = split(rv.string_, '%');

    for(int i=0; i<parts.size(); i++){
        string target = parts[i];
        size_t offset = lv.string_.find(target, start_offset);
        if(offset == string::npos){
            passed = false;
            break;
        }else{
            start_offset += target.size() + offset;
        }
    }

    if(passed){
        if(rv.string_[0] != '%'){
            if(lv.string_.find(parts[0]) != 0){
                passed = false;
            }
        }

        if(rv.string_[rv.string_.size() - 1] != '%'){
            if(lv.string_.find(parts[parts.size()-1]) != lv.string_.size()-parts[parts.size()-1].size()){
                passed = false;
            }
        }        
    }

    // /*debugg*/cout << "'" << lv.string_ << "' like '" << rv.string_ << "' : " << passed << endl;

    return passed;
}

bool Filter::compare_not_like(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list){
    return !compare_like(origin_row_data, schema_info, filtering, column_offset_list);
}

bool Filter::compare_between(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list){
    kValue lv = calcul_operand_value(origin_row_data, schema_info, filtering.lv, column_offset_list);
    kValue min = calcul_operand_value(origin_row_data, schema_info, filtering.rv, column_offset_list, 0);
    kValue max = calcul_operand_value(origin_row_data, schema_info, filtering.rv, column_offset_list, 1);

    bool passed;
    
    switch(lv.type_){
        case KETI_VALUE_TYPE::INT32:{
            passed = (min.int_ <= lv.int_  && lv.int_ <= max.int_) ? true : false; 
            ///*debugg*/cout << lv.int_ << " between " << min.int_ << " and " << max.int_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::DECIMAL:
        case KETI_VALUE_TYPE::INT64:{
            passed = (min.int64_ <= lv.int64_  && lv.int64_ <= max.int64_) ? true : false; 
            ///*debugg*/cout << lv.int64_ << " between " << min.int64_ << " and " << max.int64_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::FLOAT32:{
            passed = (min.float_ <= lv.float_  && lv.float_ <= max.float_) ? true : false; 
            ///*debugg*/cout << lv.float_ << " between " << min.float_ << " and " << max.float_ << " : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::DOUBLE:{
            passed = (min.double_ <= lv.double_  && lv.double_ <= max.double_) ? true : false; 
            ///*debugg*/cout << lv.double_ << " between " << min.double_ << " and " << max.double_ << " : " << passed << endl;
            break;
        }default:{
            KETILOG::ERRORLOG(LOGTAG, "@error undefined compare_eq type");
            break;
        }
    }

    return passed;
}

bool Filter::compare_in(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list){
    kValue lv = calcul_operand_value(origin_row_data, schema_info, filtering.lv, column_offset_list, 0);
    bool passed = false;
    
    switch(lv.type_){
        case KETI_VALUE_TYPE::INT32:{
            ///*debugg*/cout << lv.int_ << " in ("; 
            for(int i=0; i<filtering.rv.values.size(); i++){
                ///*debugg*/cout<< filtering.rv.values[i] << ",";
                if(lv.int_ == stoi(filtering.rv.values[i])){
                    passed = true;
                    break;
                }
            }
            ///*debugg*/cout << ") : " << passed << endl;
            break;
        }case KETI_VALUE_TYPE::STRING:{
            // lv.string_ = convert_to_lowercase(lv.string_);
            ///*debugg*/cout << "'" << lv.string_ << "' in ("; 
            for(int i=0; i<filtering.rv.values.size(); i++){
                // string comparator = convert_to_lowercase(filtering.rv.values[i]);
                ///*debugg*/cout<< "'" << comparator << "',";
                if(lv.string_ == filtering.rv.values[i]){
                    passed = true;
                    break;
                }
            }
            ///*debugg*/cout << ") : " << passed << endl;
            break;
        }default:{
            KETILOG::ERRORLOG(LOGTAG, "@error undefined compare_in type");
            break;
        }
    }
    
    return passed;
}

bool Filter::compare_not_in(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list){
    return !compare_not_in(origin_row_data, schema_info, filtering, column_offset_list);
}

bool Filter::compare_is(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list){
    kValue lv = calcul_operand_value(origin_row_data, schema_info, filtering.lv, column_offset_list, 0);

    bool passed = false;

    //* 구현 아직, tpch에 포함 X, bool/null 판단

    return passed;
}

bool Filter::compare_is_not(const char* origin_row_data, SchemaInfo& schema_info, Filtering& filtering, vector<int>& column_offset_list){
    return !compare_is(origin_row_data, schema_info, filtering, column_offset_list);
}