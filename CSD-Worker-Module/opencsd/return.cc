#include "return.h"

void Return::return_worker(){
    while (1){
        CsdResult result = return_queue_->wait_and_pop();

        send_data(result);        
    }
}

void Return::send_data(CsdResult &result){
    //마지막 블록과 관련한 로그 작업
    string json_;

    StringBuffer block_buf;
    PrettyWriter<StringBuffer> writer(block_buf);

    writer.StartObject();

    writer.Key("query_id");
    writer.Int(result.snippet->query_id);

    writer.Key("work_id");
    writer.Int(result.snippet->work_id);

    writer.Key("row_count");
    writer.Int(result.data.row_count);

    writer.Key("row_offset");
    writer.StartArray();
    for (int i = 0; i < result.data.row_offset.size(); i++){
        writer.Int(result.data.row_offset[i]);
    }
    writer.EndArray();

    writer.Key("length");
    writer.Int(result.data.data_length);

    writer.Key("return_column_type");
    writer.StartArray();
    for (int i = 0; i < result.snippet->result_info.return_column_type.size(); i++){
        writer.Int(result.snippet->result_info.return_column_type[i]);
    }
    writer.EndArray();

    writer.Key("return_column_length");
    writer.StartArray();
    for (int i = 0; i < result.snippet->result_info.return_column_length.size(); i++){
        writer.Int(result.snippet->result_info.return_column_length[i]);
    }
    writer.EndArray();

    cout << result.data.current_block_count << endl;

    writer.Key("current_block_count");
    writer.Int(result.data.current_block_count);

    writer.Key("scanned_row_count");
    writer.Int(result.data.scanned_row_count);

    writer.Key("filtered_row_count");
    writer.Int(result.data.filtered_row_count);

    writer.Key("column_alias");
    writer.StartArray();
    for (int i = 0; i < result.snippet->result_info.column_alias.size(); i++){
        writer.String(result.snippet->result_info.column_alias[i].c_str());
    }
    writer.EndArray();

    writer.Key("total_block_count");
    writer.Int(result.snippet->result_info.total_block_count);
    
    writer.Key("table_alias");
    writer.String(result.snippet->result_info.table_alias.c_str());

    writer.EndObject();

    string block_buf_ = block_buf.GetString();

    int sockfd;
    struct sockaddr_in serv_addr;
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(BUFF_M_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(BUFF_M_IP);

    int response = connect(sockfd,(const sockaddr*)&serv_addr,sizeof(serv_addr));
    if( 0 != response ) {
        KETILOG::INFOLOG(LOGTAG, "[error] return interface connection failed");
    } 
    
    size_t len = strlen(block_buf_.c_str());
	send(sockfd, &len, sizeof(len), 0);
    send(sockfd, (char*)block_buf_.c_str(), len, 0);

    static char cBuffer[PACKET_SIZE];
    if (recv(sockfd, cBuffer, PACKET_SIZE, 0) == 0){
        KETILOG::FATALLOG(LOGTAG,"client recv Error");
        return;
    };

    len = result.data.data_length;
    send(sockfd,&len,sizeof(len),0);
    send(sockfd, result.data.raw_data, BUFFER_SIZE, 0);
    
    close(sockfd);
}