#pragma once

#include "worker.h"
#include "scan.h"
#include "header.h"

class Input{
public:
    Input(Scan* scan_layer, Worker* tmax_worker){
        this->scan_layer_ = scan_layer;
        this->tmax_worker_ = tmax_worker;
    }
    void input_worker();
    void parse_snippet(const char* json);

private:
    Scan* scan_layer_;
    Worker* tmax_worker_;

    inline const static std::string LOGTAG = "CSD Input";
    char msg[200];
};
