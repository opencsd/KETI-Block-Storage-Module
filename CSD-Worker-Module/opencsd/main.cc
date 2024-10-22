//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory)

#include "input.h"
#include "header.h"

using namespace std;

int main(int argc, char** argv) {
    if (argc >= 2) {
        KETILOG::SetLogLevel(stoi(argv[1]));
    }else{
        KETILOG::SetDefaultLogLevel();
    }

    if (argc >= 3) {
        KETILOG::SetRunningMode(stoi(argv[1]));
    }else{
        KETILOG::SetRunningMode(RUNNING_MODE::LOCAL);
    }

    Return return_layer;
    Projection projection_layer(&return_layer);
    Filter filter_layer(&projection_layer);
    Scan scan_layer(&filter_layer, &projection_layer);
    Input input_layer(&scan_layer);

    thread input_layer_thread(&Input::input_worker, &input_layer);
    
    thread scan_layer_thread1(&Scan::scan_worker, &scan_layer);
    thread scan_layer_thread2(&Scan::scan_worker, &scan_layer);

    thread filter_layer_thread1(&Filter::filter_worker, &filter_layer);
    thread filter_layer_thread2(&Filter::filter_worker, &filter_layer);

    thread projection_layer_thread1(&Projection::projection_worker, &projection_layer);
    thread projection_layer_thread2(&Projection::projection_worker, &projection_layer);

    thread return_layer_thread(&Return::return_worker, &return_layer);

    httplib::Server server;
    server.Get("/blockcount", BlockCountManager::HandleGetBlockCount);
    server.Get("/log-level", KETILOG::HandleSetLogLevel);

    KETILOG::INFOLOG("CSD Worker Module", "/blockcount & /log-level Host listening on port 40305...\n");
    server.listen("0.0.0.0", 40305);
    
    string line;
    getline(cin, line);
    
    server.stop();

    input_layer_thread.join();
    scan_layer_thread1.join();
    scan_layer_thread2.join();
    filter_layer_thread1.join();
    filter_layer_thread2.join();
    projection_layer_thread1.join();
    projection_layer_thread2.join();
    return_layer_thread.join();

    return 0;
}
