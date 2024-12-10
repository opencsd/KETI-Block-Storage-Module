#pragma once

#include <mutex>
#include <climits>
#include <thread>
#include "httplib.h"

using namespace std;

class MonitoringManager{
  public: 
    static void T_AddBlockCount(int add_block){
        GetInstance().t_addBlockCount(add_block);
    }

    static void T_AddWorkingId(int id){
        GetInstance().t_addWorkingId(id);
    }

    static void T_RemoveWorkingId(int id){
        GetInstance().t_removeWorkingId(id);
    }

    static void AddBlockCount(int add_block){
        GetInstance().addBlockCount(add_block);
    }

    static void SubtractCurrentBlockCount(int subtract_block){
        GetInstance().subtractCurrentBlockCount(subtract_block);
    }

    static MonitoringManager& GetInstance(){
        static MonitoringManager manager;
        return manager;
    }

    static void T_HandleGetMonitoring(const httplib::Request& request, httplib::Response& response) {
        unsigned int total_block_count = T_GetAccumulateBlockCount();
        vector<int> working_id_list = T_GetWorkingId();

        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < working_id_list.size(); ++i) {
            oss << working_id_list[i];
            if (i < working_id_list.size() - 1) {
                oss << ", ";
            }
        }
        oss << "]";

        std::string json_response = "{ \"total_block_count\": " + std::to_string(total_block_count) +
                                ", \"working_id_list\": " + oss.str() + " }";
        response.set_content(json_response, "application/json");
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

    static unsigned int T_GetAccumulateBlockCount(){
        return GetInstance().t_getAccumulateBlockCount();
    }

    static vector<int> T_GetWorkingId(){
        return GetInstance().t_getWorkingId();
    }

  private:
    MonitoringManager() {
        Current_Working_Block_Count = 0;
        Accumulate_Working_Block_Count = 0;
    };
    MonitoringManager(const MonitoringManager&);
    ~MonitoringManager() {};
    MonitoringManager& operator=(const MonitoringManager&){
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

    unsigned long long t_getAccumulateBlockCount(){
        unique_lock<mutex> lock(safe_mutex);
        return GetInstance().T_Accumulate_Working_Block_Count;
    }

    vector<int> t_getWorkingId(){
        unique_lock<mutex> lock(safe_mutex);
        return GetInstance().T_Working_Id_List;
    }

    void t_addBlockCount(int add_block){
        unique_lock<mutex> lock(safe_mutex);

        if (T_Accumulate_Working_Block_Count > ULONG_LONG_MAX - add_block) {
            T_Accumulate_Working_Block_Count = ULONG_LONG_MAX - T_Accumulate_Working_Block_Count;
        } else {
            T_Accumulate_Working_Block_Count += add_block;
        }
    }

    void t_addWorkingId(int id){
        unique_lock<mutex> lock(safe_mutex);

        T_Working_Id_List.push_back(id);
    }

    void t_removeWorkingId(int id){
        unique_lock<mutex> lock(safe_mutex);

        T_Working_Id_List.erase(std::remove(T_Working_Id_List.begin(), T_Working_Id_List.end(), id), T_Working_Id_List.end());
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
    unsigned long long T_Accumulate_Working_Block_Count;
    vector<int> T_Working_Id_List;
};
