#pragma once

#include "header.h"

extern WorkQueue<res_chunk_t> return_queue_;

class Return{
    public:
        Return(){
            thread return_working_layer = thread(&Return::send_result_block,this);
            return_working_layer.detach();
        }
        void send_result_block(){
            while (1){
                res_chunk_t res_chunk_t = return_queue_.wait_and_pop();
                int sockfd;
                struct sockaddr_in serv_addr;
                sockfd = socket(PF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    perror("ERROR opening socket");
                }
                memset(&serv_addr, 0, sizeof(serv_addr));
                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons(T_SE_MERGING_TCP_PORT);

                if (inet_pton(AF_INET, "10.0.4.80", &serv_addr.sin_addr) <= 0) {
                    perror("ERROR invalid IP address");
                    close(sockfd);
                    continue; // Skip to the next iteration of the loop
                }

                int response = connect(sockfd,(const sockaddr*)&serv_addr,sizeof(serv_addr));
                if( 0 != response ) {
                    cout << "ERROR return interface connection failed" << endl;
                } 

                send(sockfd, &res_chunk_t.res_len, sizeof(int), 0);
                send(sockfd, &res_chunk_t.res_buf, res_chunk_t.res_len, 0);
                
                close(sockfd);
            }

            
        }
};