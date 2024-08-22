#include "header.h"
#include "worker.h"

using namespace rapidjson;
using namespace std;

WorkQueue<t_snippet> work_queue_;
WorkQueue<res_chunk_t> return_queue_;

int main() {
    thread return_layer = thread(&Return::send_result_block,Return());
    thread working_layer = thread(&Worker::working,Worker());

    int server_fd;
    int client_fd;
    int opt = 1;
    struct sockaddr_in serv_addr; // 소켓주소
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
    serv_addr.sin_port = htons(T_CSD_WORKER_MODULE_PORT);

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
        char buffer[BUFF_SIZE] = {0};
        
        size_t length;
        int temp = read( client_fd , &length, sizeof(length));
        if(temp == 0){
        cout << "read error" << endl;
        }

        int numread;
        while(1) {
        if ((numread = read( client_fd , buffer, BUFF_SIZE - 1)) == -1) {
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

        cout << "*******************Snippet JSON*****************" << endl;
        cout << json.c_str() << endl;
        cout << "************************************************" << endl;

        t_snippet parsedSnippet(json.c_str());

        work_queue_.push_work(parsedSnippet);

        close(client_fd);
    }

    close(server_fd);
    working_layer.join();

    return_layer.join();
    return 0;
}
