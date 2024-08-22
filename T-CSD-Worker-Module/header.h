#pragma once

#include <iostream>
#include <unordered_map>
#include <unistd.h>
#include <thread>
#include <fcntl.h>
#include <vector>
#include <cstring>

#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"

#include "internal_queue.h"
#include "tb_block.h" // tmax library

#define T_SE_MERGING_TCP_PORT 40209
#define T_CSD_WORKER_MODULE_PORT 40308
#define BUFF_SIZE 4096

using namespace rapidjson;
using namespace std;
