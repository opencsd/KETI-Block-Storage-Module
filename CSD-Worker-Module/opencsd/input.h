#pragma once

#include "scan.h"
#include "header.h"

class Input{
public:
    Input(Scan* scan_layer){
        this->scan_layer_ = scan_layer;
    }
    void input_worker();
    void parse_snippet(const char* json);
    void calcul_return_column_type(shared_ptr<Snippet> snippet);

private:
    Scan* scan_layer_;
    // Worker* tmax_worker_;

    inline const static std::string LOGTAG = "CSD Input";
    char msg[200];
};
