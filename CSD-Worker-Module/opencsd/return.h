#pragma once

#include "header.h"

using namespace std;
using namespace rapidjson;

class Return{
public:
    Return(){
        this->return_queue_ = new WorkQueue<Result>;
    }

    ~Return(){
        delete return_queue_;
    }

    void return_worker();
    void send_data(Result &result);

    void enqueue_return(Result result){
        return_queue_->push_work(result);
    }

private:
    WorkQueue<Result>* return_queue_;

    inline const static std::string LOGTAG = "CSD Return";
    char msg[200];
};