// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory)
#include <thread>

#include "input.h"

using namespace std;

// ---------------현재 사용하는 인자(compression, cache X)-----------
// bool blocks_maybe_compressed = false;
// bool blocks_definitely_zstd_compressed = false;
// uint32_t read_amp_bytes_per_bit = 0;
// const bool immortal_table = false;
// std::string dev_name = "/dev/sda";
// -----------------------------------------------------------------

WorkQueue<Snippet> ScanQueue;
WorkQueue<Result> FilterQueue;
WorkQueue<Result> MergeQueue;
WorkQueue<MergeResult> ReturnQueue;

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

    CSDTableManager::InitCSDTableManager();    

    thread InputInterface = thread(&Input::InputSnippet, Input());
    // Scan scan = Scan(CSDTableManager);
    // scan.Scanning();
    thread ScanLayer = thread(&Scan::Scanning, Scan());
    // thread ScanLayer2 = thread(&Scan::Scanning, Scan());
    thread FilterLayer = thread(&Filter::Filtering, Filter());
    // thread FilterLayer2 = thread(&Filter::Filtering, Filter());
    // thread FilterLayer3 = thread(&Filter::Filtering, Filter());
    // thread FilterLayer4 = thread(&Filter::Filtering, Filter());
    thread MergeLayer = thread(&MergeManager::Merging, MergeManager());
    // thread MergeLayer2 = thread(&MergeManager::Merging, MergeManager());
    // thread MergeLayer3 = thread(&MergeManager::Merging, MergeManager());
    // thread MergeLayer4 = thread(&MergeManager::Merging, MergeManager());
    thread ReturnInterface = thread(&Return::ReturnResult, Return());

    httplib::Server server;
    server.Get("/blockcount", BlockCountManager::HandleGetBlockCount);
    server.Get("/log-level", KETILOG::HandleSetLogLevel);

    KETILOG::INFOLOG("CSD Worker Module", "/blockcount & /log-level Host listening on port 40305...\n");
    server.listen("0.0.0.0", 40305);
    
    string line;
    getline(cin, line);
    
    server.stop();

    InputInterface.join();
    ScanLayer.join();
    // ScanLayer2.join();
    FilterLayer.join();
    // FilterLayer2.join();
    // FilterLayer3.join();
    // FilterLayer4.join();
    MergeLayer.join();
    // MergeLayer2.join();
    // MergeLayer3.join();
    // MergeLayer4.join();
    ReturnInterface.join();

    return 0;
}
