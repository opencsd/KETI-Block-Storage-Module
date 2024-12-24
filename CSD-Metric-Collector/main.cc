#include "csd-metric-collector.h"
void runServer(){
    httplib::Server server;
    server.Get("/csd-metric", CsdMetricCollector::HandleGetCsdMetric);
    

    server.Get("/score-weight", CsdMetricCollector::HandleSetScoreWeight);
    
    cout << "[CSD Metric Collector] run on 0.0.0.0:" << CSD_METRIC_COLLECTOR_PORT << endl;

    server.listen("0.0.0.0", CSD_METRIC_COLLECTOR_PORT);
}

int main(int argc, char *argv[]){    
    std::thread server_thread(runServer);

    CsdMetricCollector::RunCollect();

    server_thread.join();

    return 0;
}

/*
ip: 10.1.1.2
CPU Total: 4
CPU Used: 0.113227
CPU Utilization(%): 2.83067%
Memory Total(KB): 6111708
Memory Used(KB): 908420
Memory Utilization(%): 14.8636%
Storage Total(KB): 208562268
Storage Used(KB): 34237280
Storage Utilization(%): 16.4159%
Network RX Bytes(Byte): 2940
Network TX Bytes(Byte): 2388
Network Used(kbps): 8520
CSD Working Block Count: 0
CSD Metric Score: 89.9496
connect host server failed
*/

/*
{
    "ip": "10.1.1.2",
    "cpuTotal": 4.0,
    "cpuUsed": 0.08205128461122513,
    "cpuUtilization": 2.0512821674346926,
    "memoryTotal": 6111708,
    "memoryUsed": 2502884,
    "memoryUtilization": 40.9522819519043,
    "storageTotal": 208562268,
    "storageUsed": 34810856.0,
    "storageUtilization": 16.69087028503418,
    "networkRxData": 516,
    "networkTxData": 1380,
    "networkBandwidth": 3032,
    "csdMetricScore": 74.60811691284181,
    "csdWorkingBlockCount": 0
}
*/
