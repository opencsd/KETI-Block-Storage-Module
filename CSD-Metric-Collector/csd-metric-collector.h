#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <sys/statvfs.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "httplib.h"

using namespace std;
using namespace rapidjson;

#define ONE_LINE 80
#define PAST 0
#define PRESENT 1
#define JIFFIES_NUM 4
#define PACKET_SIZE 36
#define BUFFER_SIZE 4096

#define CSD_HOST_SERVER_IP getenv("CSD_HOST_SERVER_IP")
#define STORAGE_METRIC_COLLECTOR_PORT 40304
#define CSD_WORKER_MODULE_PORT 40302
#define CSD_METRIC_COLLECTOR_PORT 40303

struct stJiffies{
    int user;
    int nice;
    int system;
    int idle;
};

struct Cpu{
    stJiffies stJiffies_;
    float total;
    float used;
    float utilization;
};

struct Memory{
    long free;
    long buffers;
    long cached;
    int total;
    int used;
    float utilization;
};

struct Storage{
    int total;
    int used;
    float utilization;
};

struct Power{
    long long energy1;
    long long energy2;
    float total;
    float used;
    float utilization;
};

struct Network{
    long long rxBytes;
    long long txBytes;
    long long used;
    long long rxData; 
    long long txData; 
};

class CsdMetricCollector{
    public:

        static CsdMetricCollector& GetInstance() {
            static CsdMetricCollector csdMetricCollector;
            return csdMetricCollector;
        }

        static void RunCollect(){
            return GetInstance().run_collect();
        }

        static void HandleGetCsdMetric(const httplib::Request& request, httplib::Response& response){
            return GetInstance().handle_get_csd_metric(request, response);
        }

        static void HandleSetScoreWeight(const httplib::Request& request, httplib::Response& response){
            return GetInstance().handle_set_score_weight(request, response);
        }

    private:
        CsdMetricCollector() {
            csd_ip_ = std::getenv("CSD_IP");
            init_metric();
        }
        CsdMetricCollector(const CsdMetricCollector&);
        CsdMetricCollector& operator=(const CsdMetricCollector&){
            return *this;
        };
        ~CsdMetricCollector() {}

        void print_metric();
        string serialize_response();
        void send_metric();
        void init_metric();
        void run_collect();
        void update_cpu();
        void update_memory();
        void update_network();
        void update_storage();
        void update_power();
        void get_csd_working_block_count();
        void calcul_csd_metric_score();
        void handle_get_csd_metric(const httplib::Request& request, httplib::Response& response);
        void handle_set_score_weight(const httplib::Request& request, httplib::Response& response);

        Cpu cpu_;
        Memory memory_;
        Storage storage_;
        Power power_;
        Network network_;
        int working_block_count_;
        double csd_metric_score_;

        double cpu_weight_ = 0.4;
        double memory_weight_ = 0.3;
        double storage_weight_ = 0.3;
        // double network_weight_ = 0.1;
        // double power_weight_ = 0.25;

        string csd_ip_;
        mutex weight_mutex_;

        std::thread collecting_thread_;
};