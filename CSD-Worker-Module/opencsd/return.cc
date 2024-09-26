#include "return.h"

void Return::return_worker(){
    while (1){
        Result result = return_queue_->wait_and_pop();

        send_data(result);        
    }
}

void Return::send_data(Result &result){
    //마지막 블록과 관련한 로그 작업

    string json_;

    

    int sockfd;
    struct sockaddr_in serv_addr;
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(BUFF_M_PORT);

    if(KETILOG::IsRunningMode(KETI_RUNNING_MODE::LOCAL)){
        serv_addr.sin_addr.s_addr = inet_addr(STORAGE_ENGINE_LOCAL_IP);
    }else{
        serv_addr.sin_addr.s_addr = inet_addr(STORAGE_ENGINE_POD_IP);
    }

    int response = connect(sockfd,(const sockaddr*)&serv_addr,sizeof(serv_addr));
    if( 0 != response ) {
        KETILOG::INFOLOG(LOGTAG, "[error] return interface connection failed");
    } 
	
	size_t len = strlen(json_.c_str());
	send(sockfd, &len, sizeof(len), 0);
    send(sockfd, (char*)json_.c_str(), len, 0);

    static char cBuffer[PACKET_SIZE];
    if (recv(sockfd, cBuffer, PACKET_SIZE, 0) == 0){
        KETILOG::FATALLOG(LOGTAG,"client recv Error");
        return;
    };

    // len = result.length;
    // send(sockfd,&len,sizeof(len),0);
    // send(sockfd, result.data, BUFF_SIZE, 0);
    
    close(sockfd);
}