#pragma once


#include "return.h"
#include "snippet.h"

extern WorkQueue<t_snippet> work_queue_;
extern WorkQueue<res_chunk_t> return_queue_;

#define TPRINTF(tcnum, tctag, ...)\
	printf("[TEST CASE %d] %s | ", tcnum, tctag);\
	printf(__VA_ARGS__); printf("\n");

class Worker{
    public:
        Worker(){}
        int working();
};