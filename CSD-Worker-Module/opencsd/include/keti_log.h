#ifndef LOG_MSG_H_INCLUDED
#define LOG_MSG_H_INCLUDED

#pragma once

#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <string.h>
#include "httplib.h"

#define MSQID 12345
#define MSGMAAX 4096

using namespace std;

struct log_data {
    char msg[MSGMAAX];
};

struct message {
    long msg_type;
    log_data data;
};

typedef enum DEBUGG_LEVEL {
    TRACE = 0,
    DEBUG = 1,
    METRIC = 2,
    INFO = 3,
    WARN = 4,
    ERROR = 5,
    FATAL = 6
}KETI_DEBUGG_LEVEL;

typedef enum RUNNING_MODE {
    LOCAL = 0,
    CLOUD = 1,
}KETI_RUNNING_MODE;

class KETILOG {
    public:
        static void HandleSetLogLevel(const httplib::Request& request, httplib::Response& response) {
            try {
                string log_level = request.get_param_value("log_level");
                SetLogLevel(stoi(log_level));
                KETILOG::INFOLOG("Handle Set Log Level", "log level changed to "+log_level);
                response.status = 200;
            }catch (std::exception &e) {
                KETILOG::INFOLOG("Handle Set Log Level", e.what());
                response.status = 501;
            }
        }

        static void SetRunningMode(int mode){
            GetInstance().RUNNING_MODE = mode;
        }

        static void SetDefaultLogLevel(){
            GetInstance().LOG_LEVEL = DEBUG;
        }
        
        static void SetLogLevel(int level){
            GetInstance().LOG_LEVEL = level;
        }

        static int GetLogLevel(){
            return GetInstance().LOG_LEVEL;
        }

        static void TRACELOG(std::string id, const char msg[]){
            if(GetInstance().LOG_LEVEL <= TRACE){
                printf("[%s] %s\n", id.c_str(), msg);
            }
        }

        static void DEBUGLOG(std::string id, const char msg[]){
            if(GetInstance().LOG_LEVEL <= DEBUG){
                printf("[%s] %s\n", id.c_str(), msg);
            }
        }

        static void DEBUGLOG(std::string id, string msg){
            if(GetInstance().LOG_LEVEL <= INFO){
                printf("[%s] %s\n", id.c_str(), msg.c_str());
            }
        }

        static void INFOLOG(std::string id, const char msg[]){
            if(GetInstance().LOG_LEVEL <= INFO){
                printf("[%s] %s\n", id.c_str(), msg);
            }
        }

        static void INFOLOG(std::string id, string msg){
            if(GetInstance().LOG_LEVEL <= INFO){
                printf("[%s] %s\n", id.c_str(), msg.c_str());
            }
        }

        static void WARNLOG(std::string id, const char msg[]){
            if(GetInstance().LOG_LEVEL <= WARN){
                printf("[%s] %s\n", id.c_str(), msg);
            }
        }

        static void WARNLOG(std::string id, string msg){
            if(GetInstance().LOG_LEVEL <= ERROR){
                printf("[%s] %s\n", id.c_str(), msg.c_str());
            }
        }

        static void ERRORLOG(std::string id, const char msg[]){
            if(GetInstance().LOG_LEVEL <= ERROR){
                printf("[%s] %s\n", id.c_str(), msg);
            }
        }

        static void ERRORLOG(std::string id, string msg){
            if(GetInstance().LOG_LEVEL <= FATAL){
                printf("[%s] %s\n", id.c_str(), msg.c_str());
            }
        }

        static void FATALLOG(std::string id, const char msg[]){
            if(GetInstance().LOG_LEVEL <= FATAL){
                printf("[%s] %s\n", id.c_str(), msg);
            }
        }

        static void FATALLOG(std::string id, string msg){
            if(GetInstance().LOG_LEVEL <= FATAL){
                printf("[%s] %s\n", id.c_str(), msg.c_str());
            }
        }

        static bool IsLogLevelUnder(int level){
            bool flag = (GetInstance().LOG_LEVEL <= level);
            return flag;
        }

        static bool IsRunningMode(int mode){
            return GetInstance().RUNNING_MODE == mode;
        }

    private:
        KETILOG(){};
        KETILOG(const KETILOG&);
        KETILOG& operator=(const KETILOG&){
            return *this;
        }

        static KETILOG& GetInstance(){
            static KETILOG _instance;
            return _instance;
        }

        int LOG_LEVEL;
        int RUNNING_MODE;

        unordered_map<int, string> csd_log_map;// key=<qid>
};

#endif // LOG_MSG_H_INCLUDED