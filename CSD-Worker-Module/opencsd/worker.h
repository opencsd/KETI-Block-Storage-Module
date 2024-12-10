#pragma once

#include "return.h"
#include "data_structure.h"
#include "tb_block.h" // tmax library
#include "keti_log.h"
#include "monitoring_manager.h"

#define TPRINTF(tcnum, tctag, ...)\
	printf("[TEST CASE %d] %s | ", tcnum, tctag);\
	printf(__VA_ARGS__); printf("\n");

struct chunk{
    uint64_t offset;
    uint64_t length;
};

struct result{
    int id;
    uchar* data;
    int length;
    bool last;
    int chunk_count;

    result(int id_, int buffer_size_) {
        this->data = new uchar[buffer_size_];
        this->id = id_;
        this->length = 0;
        this->last = false;
        this->chunk_count = 0;
    }

    result(const result& other) {
        this->id = other.id;
        this->length = other.length;
        this->last = other.last;
        this->chunk_count = other.chunk_count;
        this->data = new uchar[other.length];
        std::copy(other.data, other.data + other.length, this->data);
    }

    result& operator=(const result& other) {
        if (this != &other) {
            delete[] this->data;
            this->id = other.id;
            this->length = other.length;
            this->last = other.last;
            this->chunk_count = other.chunk_count;
            this->data = new uchar[other.length];
            std::copy(other.data, other.data + other.length, this->data);
        }
        return *this;
    }

    ~result() {
        delete[] data;
    }

    void init_result(int buffer_size){
        memset(data, 0, buffer_size);
        this->length = 0;
        this->chunk_count = 0;
    }
};

struct tSnippet {
    int id;
    int block_size;
    int buffer_size;
    vector<chunk> chunks;
    string filter_info;
    string file_name;
};

class Worker{
    public:
        Worker(){
            this->work_queue_ = new WorkQueue<shared_ptr<tSnippet>>;
            this->return_queue_ = new WorkQueue<std::shared_ptr<result>>;
        }
        void tmax_working();
        void tmax_return();

        ~Worker(){
            delete return_queue_;
            delete work_queue_;
        }

        void enqueue_worker(shared_ptr<tSnippet> snippet){
            work_queue_->push_work(snippet);
        }

        void enqueue_return(const result& result_obj) {
            return_queue_->push_work(std::make_shared<result>(result_obj));
        }

    private:
        WorkQueue<shared_ptr<tSnippet>>* work_queue_;
        WorkQueue<std::shared_ptr<result>>* return_queue_;
};
