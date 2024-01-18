#pragma once

#include <mutex>
#include <climits>
#include <thread>
#include "httplib.h"

using namespace std;

class BlockCountManager{
  public: 
    static void AddBlockCount(int add_block){
        GetInstance().addBlockCount(add_block);
    }

    static void SubtractCurrentBlockCount(int subtract_block){
        GetInstance().subtractCurrentBlockCount(subtract_block);
    }

    static BlockCountManager& GetInstance(){
        static BlockCountManager manager;
        return manager;
    }

    static void HandleGetBlockCount(const httplib::Request& request, httplib::Response& response) {
        unsigned int current_block_count = GetCurrentBlockCount();
        unsigned long long accumulate_block_count = GetAccumulateBlockCount();

        std::string json_response = "{ \"current_block_count\": " + std::to_string(current_block_count) +
                                ", \"accumulate_block_count\": " + std::to_string(accumulate_block_count) + " }";
        response.set_content(json_response, "application/json");
    }

    static unsigned int GetCurrentBlockCount(){
        return GetInstance().getCurrentBlockCount();
    }

    static unsigned int GetAccumulateBlockCount(){
        return GetInstance().getAccumulateBlockCount();
    }

  private:
    BlockCountManager() {
        Current_Working_Block_Count = 0;
        Accumulate_Working_Block_Count = 0;
    };
    BlockCountManager(const BlockCountManager&);
    ~BlockCountManager() {};
    BlockCountManager& operator=(const BlockCountManager&){
        return *this;
    }   

    unsigned int getCurrentBlockCount(){
        unique_lock<mutex> lock(safe_mutex);
        return GetInstance().Current_Working_Block_Count;
    }

    unsigned long long getAccumulateBlockCount(){
        unique_lock<mutex> lock(safe_mutex);
        return GetInstance().Accumulate_Working_Block_Count;
    }

    void addBlockCount(int add_block){
        unique_lock<mutex> lock(safe_mutex);

        Current_Working_Block_Count += add_block;
        
        if (Accumulate_Working_Block_Count > ULONG_LONG_MAX - add_block) {
            Accumulate_Working_Block_Count = ULONG_LONG_MAX - Accumulate_Working_Block_Count;
        } else {
            Accumulate_Working_Block_Count += add_block;
        }
    }

    void subtractCurrentBlockCount(int subtract_block){
        unique_lock<mutex> lock(safe_mutex);
        if (Current_Working_Block_Count < subtract_block) {
            Current_Working_Block_Count = 0;
        } else {
            Current_Working_Block_Count -= subtract_block;
        }
    }

  private:
    mutex safe_mutex;
    unsigned int Current_Working_Block_Count;
    unsigned long long Accumulate_Working_Block_Count;
};
