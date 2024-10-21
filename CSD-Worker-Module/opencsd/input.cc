#include "input.h"

void Input::input_worker(){   
    int server_fd;
    int client_fd;
    int opt = 1;
    struct sockaddr_in serv_addr;
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(INPUT_IF_PORT);

    if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while(1){
        if ((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen)) < 0){
            perror("accept");
                exit(EXIT_FAILURE);
        }

        std::string json;
        char buffer[BUFFER_SIZE] = {0};
        
        size_t length;
        int temp = read( client_fd , &length, sizeof(length));
        if(temp == 0){
            cout << "read error" << endl;
        }

        int numread;
        while(1) {
            if ((numread = read( client_fd , buffer, BUFFER_SIZE - 1)) == -1) {
            cout << "read error" << endl;
            perror("read");
            exit(1);
            }
            length -= numread;
            buffer[numread] = '\0';
            json += buffer;

            if (length == 0)
            break;
        }

        if(KETILOG::IsLogLevelUnder(TRACE)){
            cout << "*******************Snippet JSON*****************" << endl;
            cout << json.c_str() << endl;
            cout << "************************************************" << endl;
        }

        parse_snippet(json.c_str());

        close(client_fd);   
    }

    close(server_fd);
}

void Input::parse_snippet(const char* _json){
    Document document;
    document.Parse(_json);

    if(document["type"].GetInt() == SNIPPET_TYPE::TMAX_SNIPPET){
        tSnippet* parsed_snippet = new tSnippet;

        // tmax_worker_->enqueue_scan(parsed_snippet); // *티맥스 처리 병합

        return;
    }else{
        shared_ptr<Snippet> parsed_snippet = std::make_shared<Snippet>();

        parsed_snippet->type = document["type"].GetInt();
        parsed_snippet->query_id = document["query_id"].GetInt();
        parsed_snippet->work_id = document["work_id"].GetInt();

        Value &_query_info = document["query_info"];
        parsed_snippet->query_info.table_name = _query_info["table_name"][0].GetString();

        Value &_filtering = _query_info["filtering"];
        for(int i = 0; i < _filtering.Size(); i++){
            Filtering filter;

            filter.operator_ = _filtering[i]["operator"].GetInt();

            if(!(filter.operator_ == KETI_OPER_TYPE::AND || filter.operator_ == KETI_OPER_TYPE::OR)){
                Operand lv, rv;
                Value &_lv =  _filtering[i]["lv"];
                Value &_rv =  _filtering[i]["rv"];

                for(int j=0; j < _lv["type"].Size(); j++){
                    filter.lv.types.push_back(_lv["type"][j].GetInt());
                    filter.lv.values.push_back(_lv["value"][j].GetString());
                }

                for(int j=0; j < _rv["type"].Size(); j++){
                    filter.rv.types.push_back(_rv["type"][j].GetInt());
                    filter.rv.values.push_back(_rv["value"][j].GetString());
                }
            }
    
            parsed_snippet->query_info.filtering.push_back(filter);
        }

        Value &_projection = _query_info["projection"];
        for(int i = 0; i < _projection.Size(); i++){
            Projectioning projection;

            projection.select_type = _projection[i]["select_type"].GetInt();
            Value &_value =  _projection[i]["value"];
            Value &_value_type =  _projection[i]["value_type"];
            for(int j=0; j < _value.Size(); j++){
                Operand info;
                projection.expression.values.push_back(_value[j].GetString());
                projection.expression.types.push_back(_value_type[j].GetInt());
            }
            parsed_snippet->query_info.projection.push_back(projection);
        }

        int column_offset = 0;

        Value &_column_list = document["schema_info"]["column_list"];
        
        for(int i = 0; i < _column_list.Size(); i++){ // insert pk info first
            if(_column_list[i]["primary"].GetBool() == true){
                parsed_snippet->schema_info.column_offset.push_back(column_offset);
                parsed_snippet->schema_info.column_name.push_back(_column_list[i]["name"].GetString());
                parsed_snippet->schema_info.column_length.push_back(_column_list[i]["length"].GetInt());
                parsed_snippet->schema_info.column_type.push_back(_column_list[i]["type"].GetInt());
                parsed_snippet->schema_info.pk_column.push_back(_column_list[i]["name"].GetString());
                column_offset += _column_list[i]["length"].GetInt();
            }
        }

        for(int i = 0; i < _column_list.Size(); i++){
            if(_column_list[i]["primary"].GetBool() == false){
                parsed_snippet->schema_info.column_offset.push_back(column_offset);
                parsed_snippet->schema_info.column_name.push_back(_column_list[i]["name"].GetString());
                parsed_snippet->schema_info.column_length.push_back(_column_list[i]["length"].GetInt());
                parsed_snippet->schema_info.column_type.push_back(_column_list[i]["type"].GetInt());
                column_offset += _column_list[i]["length"].GetInt();
            }
            
            if(_column_list[i]["index"].GetBool() == true){
                parsed_snippet->schema_info.index_column.push_back(_column_list[i]["name"].GetString());
            }

            if(_column_list[i]["type"].GetInt() == MySQL_DataType::MySQL_VARSTRING){
                parsed_snippet->schema_info.has_var_char = true;
            }
        }
        
        parsed_snippet->schema_info.column_offset.push_back(column_offset);

        for(int i = 0; i < parsed_snippet->schema_info.column_name.size(); i++){
            parsed_snippet->schema_info.column_index_map.insert({parsed_snippet->schema_info.column_name[i],i});
        }

        parsed_snippet->schema_info.table_index_number = document["schema_info"]["table_index_number"].GetInt();

        Value &_block_info = document["block_info"];

        parsed_snippet->block_info.partition = _block_info["partition"].GetString();
        for(int i=0; i < _block_info["block"].Size(); i++){
            Block block;
            for(int j=0; j < _block_info["block"][i]["offset"].Size(); j++){
                block.offset.push_back(stoull(_block_info["block"][i]["offset"][j].GetString()));
            }
            for(int j=0; j < _block_info["block"][i]["length"].Size(); j++){
                block.length.push_back(_block_info["block"][i]["length"][j].GetInt()) ;
            }
            parsed_snippet->block_info.block_list.push_back(block);
        }
        
        if(document.HasMember("wal_info")){
            Value &_deleted_key = document["wal_info"]["deleted_key"];
            for(int i=0; i < _deleted_key.Size(); i++){
                parsed_snippet->wal_info.deleted_key.push_back(_deleted_key[i].GetString());
            }
            Value &_inserted_key = document["wal_info"]["inserted_key"];
            Value &_inserted_value = document["wal_info"]["inserted_value"];
            for(int i=0; i < _inserted_key.Size(); i++){
                parsed_snippet->wal_info.inserted_key.push_back(_inserted_key[i].GetString());
                parsed_snippet->wal_info.inserted_value.push_back(_inserted_value[i].GetString());
            }
        }
        
        Value &_result_info = document["result_info"];
        parsed_snippet->result_info.table_alias = _result_info["table_alias"].GetString();
        for(int i=0; i < _result_info["column_alias"].Size(); i++){
            parsed_snippet->result_info.column_alias.push_back(_result_info["column_alias"][i].GetString());
            parsed_snippet->result_info.return_column_type.push_back(_result_info["return_column_type"][i].GetInt());
            parsed_snippet->result_info.return_column_length.push_back(_result_info["return_column_length"][i].GetInt());
        }
        parsed_snippet->result_info.total_block_count = _result_info["total_block_count"].GetInt();
        parsed_snippet->result_info.csd_block_count = _result_info["csd_block_count"].GetInt();

        char msg[200];
        memset(msg, '\0', sizeof(msg));
        sprintf(msg,"Receive Snippet {ID : %d-%d} from Storage Engine Instance",parsed_snippet->query_id, parsed_snippet->work_id);
        KETILOG::INFOLOG(LOGTAG, msg);

        scan_layer_->enqueue_scan(parsed_snippet);

        return;
    }
}