#include <mutex>
#include <queue>
#include <unordered_map>
#include <condition_variable>
#include <list>
#include <memory>

using namespace std;

typedef std::pair<int, int> pair_key;
struct pair_hash{
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2> &pair) const{
        return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
};

template <typename T>
class IdMap
{
    mutex mu;
    unordered_map<pair_key,int,pair_hash> id_block_map; //initial_data : csd_total_block_count
    vector<T*> merge_manager_list;

public:
    IdMap(){}

    void insert_merge_manager_pointer(T* merge_manager_ptr){
        merge_manager_list.push_back(merge_manager_ptr);
    }

    void create_id_block_map(pair_key key, int csd_total_block_count){
        unique_lock<mutex> lock(mu);

        if (id_block_map.find(key) == id_block_map.end()) {
            id_block_map[key] = csd_total_block_count;
        }
    }

    void count_out_block(pair_key key, int block_count){
        unique_lock<mutex> lock(mu);

        if (id_block_map.find(key) != id_block_map.end()) {
            id_block_map[key] -= block_count;
        } 
    }

    bool check_work_done(pair_key key){
        unique_lock<mutex> lock(mu);

        if (id_block_map.find(key) != id_block_map.end()) {
            if (id_block_map[key] == 0){
                id_block_map.erase(key);

                for (const auto& merge_manager : merge_manager_list) {
                    merge_manager->EraseKey(key);
                }
                return true;
            }
        }
        return false;
    }
};