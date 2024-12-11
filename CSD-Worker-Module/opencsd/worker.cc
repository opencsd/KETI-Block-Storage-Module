#include <string>
#include <vector>
#include <stdexcept>

#include "worker.h"

#define BLOCK_DIR "/home/ngd/storage/tmax/"
#define CSD_INFO_DIR "/home/ngd/storage/tmax/csd_info.dump"

std::vector<unsigned char> Base64Decode(const std::string& input) {
    static const int decode_table[128] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0-15
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 16-31
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, // 32-47 ('+'=62, '/'=63)
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1,  0, -1, -1, // 48-63 ('0'-'9'=52-61)
        -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, // 64-79 ('A'-'Z'=0-25)
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, // 80-95
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, // 96-111 ('a'-'z'=26-51)
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1  // 112-127
    };

    std::vector<unsigned char> output;
    int buffer = 0;
    int bits_collected = 0;

    for (unsigned char c : input) {
        if (std::isspace(c)) continue; // Skip whitespace
        if (c == '=') break;          // Padding

        if (c > 127 || decode_table[c] == -1) {
            throw std::invalid_argument("Invalid Base64 character");
        }

        buffer = (buffer << 6) | decode_table[c];
        bits_collected += 6;

        if (bits_collected >= 8) {
            bits_collected -= 8;
            output.push_back(static_cast<unsigned char>((buffer >> bits_collected) & 0xFF));
        }
    }

    return output;
}

void Worker::tmax_working(){
    while (1){
        shared_ptr<tSnippet> t_snippet = work_queue_->wait_and_pop();
        MonitoringManager::T_AddWorkingId(t_snippet->id);

        result t_result = result(t_snippet->id, t_snippet->buffer_size);
        
        string path = BLOCK_DIR + t_snippet->file_name;

        int fd = open(path.c_str(), O_RDONLY); 
        if (fd < 0){
            printf("cannot open block file %s\n", path.c_str());
            continue;
        }

        filter_info_t *filter_info;
        std::vector<unsigned char> decoded_data = Base64Decode(t_snippet->filter_info);
        uchar* data_ptr = decoded_data.data();
        filter_info = deserialize_filter_info(&data_ptr);
        /* [tmax] tibero 서버에서 전달한 constant 값 및 bind parameter 값, compare 정보(GT/LT/GE/LE/EQ), 
                column info(column여부 및 column index), datatype 정보 deserialize
                tibero 서버에서 byte stream으로 보낸 filter 정보를 4세부 서버에서 받아 deserialize*/

        for(int i=0; i<t_snippet->chunks.size(); i++){
            uint64_t offset = t_snippet->chunks[i].offset;
            uint64_t length = t_snippet->chunks[i].length;

            char chunk_buffer[length];
            int  chunk_idx = 0;

            if (lseek(fd, offset, SEEK_SET) == -1) {
                perror("lseek failed");
                close(fd);
                continue;
            }

            if (read(fd, chunk_buffer, length) != length){ 
                printf("cannot read from block file %s\n", path.c_str());
                close(fd);
                continue;
            }

            bool pass = true;
            
            _blk_t *blk = (_blk_t *)chunk_buffer;  // get block pointer
            _dblk_dl_t *dl;
            dl = dblk_get_dl(blk);
            if (dl->rowcnt == 0){
                printf("There is no row in this block %s\n", path.c_str());
                t_result.chunk_count++;
                continue;
            }

            chunk_list_t *chunk_list;
	        chunk_list = create_chunk_list();
            /* [tmax] chunk list를 생성하는 함수
                    본 함수를 호출 후 csd_filter_and_eval_out 인자로 넘겨줘야 함.
            */

            csd_filter_and_eval_out(blk, filter_info, chunk_list);
            /* [tmax] 읽어들인 block들에 대해 filtering 및 후처리 작업
                    deserialize한 filter info와 block 및 chunk_list를 인자로 넘겨줘야 함.
                    block 들을 iteration하며 block 각각에 대해 본 함수를 호출해야 하며, chunk list는 block들을 iterate 하기 전 create_chunk_list()를 통해 한번만 생성되면 됨.*/
            
            bool finished = false;
            while(!finished){
                finished = write_chunk_list_to_buffer(chunk_list, &t_result.data, t_snippet->buffer_size, &chunk_idx, &t_result.length);
                /* [tmax] big chunk (통 buffer)에 chunk list 결과 serialize
                    big chunk size를 넘어갈 경우 어느 chunk까지 썼는지 last_chunk_no 에 기록해두고,
                    다시 함수 들어왔을 때 이어서 쓸 수 있도록 함. */
                // chunk list에 포함된 여러 개의 chunk data(big chunk)를 하나의 큰 버퍼에 이어 붙임
                // chunk list와 big chunk, big chunk의 크기를 인자로 넘겨줌
                // buffer_length에 big chunk에 쓴 총 길이를 업데이트
                // big chunk가 다 찼을 경우, 현재 읽고 있는 index 위치를 current_chunk_idx에 저장하고 return false
                if (!finished) {
                    cout << t_snippet->id << " return result " << endl;
                    enqueue_return(t_result);
                    t_result.init_result(t_snippet->buffer_size);
                    sleep(5);
                }
            }
            
            MonitoringManager::T_AddBlockCount(t_result.chunk_count);
            t_result.chunk_count++;
        }

        close(fd);

        char msg[50];
        memset(msg, '\0', sizeof(msg));
        sprintf(msg,"Complete Tmax Work {ID : %d}",t_snippet->id);
        KETILOG::INFOLOG("T", msg);

        MonitoringManager::T_AddBlockCount(t_result.chunk_count);
        MonitoringManager::T_RemoveWorkingId(t_snippet->id);
        enqueue_return(t_result);
    }
}

void Worker::tmax_return(){
    while (1){
        auto result = return_queue_->wait_and_pop();

        string json_;

        StringBuffer block_buf;
        PrettyWriter<StringBuffer> writer(block_buf);

        writer.StartObject();

        writer.Key("id");
        writer.Int(result->id);

        writer.Key("length");
        writer.Int(result->length);

        writer.Key("chunk_count");
        writer.Int(result->chunk_count);

        writer.EndObject();

        string block_buf_ = block_buf.GetString();
        
        if(KETILOG::IsLogLevelUnder(TRACE)){
            cout << block_buf_ << endl;
        }

        int sockfd;
        struct sockaddr_in serv_addr;
        sockfd = socket(PF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("ERROR opening socket");
        }
        
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(T_SE_MERGING_TCP_PORT);
        serv_addr.sin_addr.s_addr = inet_addr(STORAGE_ENGINE_IP);

        int response = connect(sockfd,(const sockaddr*)&serv_addr,sizeof(serv_addr));
        if( 0 != response ) {
            cout << "ERROR return interface connection failed" << endl;
        }

        size_t len = strlen(block_buf_.c_str());
        send(sockfd, &len, sizeof(len), 0);
        send(sockfd, (char*)block_buf_.c_str(), len, 0);

        static char cBuffer[PACKET_SIZE];
        if (recv(sockfd, cBuffer, PACKET_SIZE, 0) == 0){
            perror("ERROR invalid IP address");
            continue;
        };

        size_t size_length = static_cast<size_t>(result->length);
        send(sockfd, &size_length, sizeof(size_length), 0);
        send(sockfd, result->data, result->length, 0);
        
        close(sockfd);
    }    
}
