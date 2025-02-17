#pragma once

#include "header.h"

using namespace std;
using namespace rapidjson;

class Return{
public:
    Return(){
        this->return_queue_ = new WorkQueue<CsdResult>;
    }

    ~Return(){
        delete return_queue_;
    }

    void return_worker();
    void send_data(CsdResult &result);

    void enqueue_return(CsdResult result, bool flag = false){
        if(flag){
            memset(msg, '\0', sizeof(msg));
            sprintf(msg,"Snippet {ID : %d|%d} Return Complete",result.snippet->query_id, result.snippet->work_id);
            KETILOG::INFOLOG(LOGTAG, msg);
        }
        return_queue_->push_work(result);
    }

private:
    WorkQueue<CsdResult>* return_queue_;

    inline const static std::string LOGTAG = "CSD Return";
    char msg[100];
};