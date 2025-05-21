//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory)

#include "input.h"
#include "header.h"

using namespace std;

int main(int argc, char** argv) {
    if (argc >= 2){
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
    Worker tmax_worker;
    Input input_layer(&scan_layer, &tmax_worker);

    thread input_layer_thread(&Input::input_worker, &input_layer);

    thread tmax_worker_thread1(&Worker::tmax_working, &tmax_worker);
    thread tmax_worker_thread2(&Worker::tmax_working, &tmax_worker);
    thread tmax_return_thread(&Worker::tmax_return, &tmax_worker);
    
    thread scan_layer_thread1(&Scan::scan_worker, &scan_layer);
    thread scan_layer_thread2(&Scan::scan_worker, &scan_layer);

    thread filter_layer_thread1(&Filter::filter_worker, &filter_layer);
    thread filter_layer_thread2(&Filter::filter_worker, &filter_layer);

    thread projection_layer_thread1(&Projection::projection_worker, &projection_layer);
    thread projection_layer_thread2(&Projection::projection_worker, &projection_layer);

    thread return_layer_thread(&Return::return_worker, &return_layer);

    std::thread send_scanned_row_count_thread(MonitoringManager::SendScannedRowCount);
    std::thread send_filtered_row_count_thread(MonitoringManager::SendFilteredRowCount);

    httplib::Server server;
    server.Get("/tmax/monitoring", MonitoringManager::T_HandleGetMonitoring);
    server.Get("/blockcount", MonitoringManager::HandleGetBlockCount);
    server.Get("/log-level", KETILOG::HandleSetLogLevel);

    KETILOG::INFOLOG("CSD Worker Module", "/blockcount & /log-level Host listening on port"+to_string(CSD_WORKER_MODULE_HTTP_PORT)+"\n");
    server.listen("0.0.0.0", CSD_WORKER_MODULE_HTTP_PORT);
    
    string line;
    getline(cin, line);
    
    server.stop();

    input_layer_thread.join();
    tmax_worker_thread1.join();
    tmax_worker_thread2.join();
    tmax_return_thread.join();
    scan_layer_thread1.join();
    scan_layer_thread2.join();
    filter_layer_thread1.join();
    filter_layer_thread2.join();
    projection_layer_thread1.join();
    projection_layer_thread2.join();
    return_layer_thread.join();
    send_scanned_row_count_thread.join();
    send_filtered_row_count_thread.join();

    return 0;
}
