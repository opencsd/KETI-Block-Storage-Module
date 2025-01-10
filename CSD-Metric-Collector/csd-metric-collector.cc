#include "csd-metric-collector.h"

inline string readStatisticsField(const string &fieldName) {
    ifstream file(fieldName);
    string line;
    string value;
    file >> value;
    return value;
}

void CsdMetricCollector::run_collect(){
    while(true){
        update_cpu();
        update_memory();
        update_network();
        update_storage();
        // update_power();
        get_csd_working_block_count();
        calcul_csd_metric_score();

        if(getenv("MOD") == "debug"){
            print_metric();
        }

        send_metric();

        sleep(5);
    }
}

void CsdMetricCollector::print_metric(){
    std::cout << "ip: " << csd_ip_ << std::endl;

    std::cout << "CPU Total: " << cpu_.total << std::endl;
    std::cout << "CPU Used: " << cpu_.used << std::endl;
    std::cout << "CPU Utilization(%): " << cpu_.utilization << "%" << std::endl;

    std::cout << "Memory Total(KB): " << memory_.total << std::endl;
    std::cout << "Memory Used(KB): " << memory_.used << std::endl;
    std::cout << "Memory Utilization(%): " << memory_.utilization << "%" << std::endl;

    std::cout << "Storage Total(KB): " << storage_.total << std::endl;
    std::cout << "Storage Used(KB): " << storage_.used << std::endl;
    std::cout << "Storage Utilization(%): " << storage_.utilization << "%" << std::endl;

    // std::cout << "Power Total: " << power_.total << std::endl;
    // std::cout << "Power Used: " << power_.used << std::endl;
    // std::cout << "Power Utilization: " << power_.utilization << "%" << std::endl;

    std::cout << "Network RX Bytes(Byte): " << network_.rxData << std::endl;
    std::cout << "Network TX Bytes(Byte): " << network_.txData << std::endl;
    std::cout << "Network Used(kbps): " << network_.used << std::endl;

    std::cout << "CSD Working Block Count: " << working_block_count_ << std::endl;  
    std::cout << "CSD Metric Score: " << csd_metric_score_ << std::endl;  
}

void CsdMetricCollector::init_metric(){
    {
        FILE* fp = popen("grep -c processor /metric/proc/cpuinfo", "r");
        int coreCount;
        
        if (!fp) {
            std::cerr << "Error: popen failed." << std::endl;
        }else{
            if (fscanf(fp, "%d", &coreCount) != 1) {
                std::cerr << "Error: Failed to read CPU core count." << std::endl;
            }
            
            cpu_.total = coreCount;
        }
    
        pclose(fp);
    }
    
    {
        FILE *pStat = NULL;
        char cpuID[6] = {0};

        pStat = fopen("/metric/proc/stat", "r");
        if (pStat == NULL) {
            std::cerr << "cannot open file: /metric/proc/stat" << std::endl;
            return;
        }

        fscanf(pStat, "%5s %d %d %d %d", cpuID, &cpu_.stJiffies_.user,
                &cpu_.stJiffies_.nice, &cpu_.stJiffies_.system, &cpu_.stJiffies_.idle);
        fclose(pStat);
    }

    {
        std::ifstream meminfo("/metric/proc/meminfo");
         if (!meminfo.is_open()) {
            std::cerr << "cannot open file: /metric/proc/meminfo" << std::endl;
            return;
        }

        std::string line;
        while (std::getline(meminfo, line)) {
            std::istringstream iss(line);
            std::string key;
            long value;

            iss >> key >> value;

            if (key == "MemTotal:") {
                memory_.total = value / 1024.0 / 1024.0;;
            }
        }
    }

    {
        string statisticsFilePath = "/metric/net/";
        // string statisticsFilePath = "/sys/class/net/eno1/statistics/";
        
        string rxBytesFieldName = statisticsFilePath + "rx_bytes";
        string txBytesFieldName = statisticsFilePath + "tx_bytes";

        string currentRxBytesStr = readStatisticsField(rxBytesFieldName);
        string currentTxBytesStr = readStatisticsField(txBytesFieldName);
        
        network_.rxBytes = std::stoll(currentRxBytesStr);
        network_.txBytes = std::stoll(currentTxBytesStr);
    }
}

string CsdMetricCollector::serialize_response(){
    string json_;

    StringBuffer block_buf;
    Writer<StringBuffer> writer(block_buf);

    writer.StartObject();

    writer.Key("ip");
    writer.String(csd_ip_.c_str());

    writer.Key("cpuTotal");
    writer.Int(static_cast<int>(std::round(cpu_.total)));
    writer.Key("cpuUsed");
    writer.Double(cpu_.used); 
    writer.Key("cpuUtilization");
    writer.Double(cpu_.utilization);

    writer.Key("memoryTotal");
    writer.Double(memory_.total);
    writer.Key("memoryUsed");
    writer.Double(memory_.used); 
    writer.Key("memoryUtilization");
    writer.Double(memory_.utilization);

    writer.Key("diskTotal");
    writer.Double(storage_.total);
    writer.Key("diskUsed");
    writer.Double(storage_.used); 
    writer.Key("diskUtilization");
    writer.Double(storage_.utilization);

    writer.Key("networkRxData");
    writer.Int64(network_.rxData);
    writer.Key("networkTxData");
    writer.Int64(network_.txData);
    writer.Key("networkBandwidth");
    writer.Int64(network_.used);

    // writer.Key("powerTotal");
    // writer.Int64(power_.total);
    // writer.Key("powerUsed");
    // writer.Int64(power_.used);
    // writer.Key("powerUtilization");
    // writer.Int64(power_.utilization);

    writer.Key("csdMetricScore");
    writer.Double(csd_metric_score_);

    writer.Key("csdWorkingBlockCount");
    writer.Int64(working_block_count_);

    writer.EndObject();

    string jsonStr = block_buf.GetString();

    return jsonStr;
}

void CsdMetricCollector::send_metric(){
    cout << "send_metric" << endl;
    string jsonStr = serialize_response();

    if(getenv("MOD") == "debug"){
        cout << jsonStr << endl;
    }

    int sockfd;
    struct sockaddr_in serv_addr;
    sockfd = socket(PF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        close(sockfd);
        return;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(STORAGE_METRIC_COLLECTOR_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(CSD_HOST_SERVER_IP);

    int response = connect(sockfd,(const sockaddr*)&serv_addr,sizeof(serv_addr));
    if( 0 != response ) {
        cout << "connect host server failed"  << endl;
        close(sockfd);
        return;
    } 

    send(sockfd, jsonStr.c_str(), strlen(jsonStr.c_str()), 0);
    close(sockfd);
}

void CsdMetricCollector::update_cpu(){
    FILE *pStat = NULL;
    char cpuID[6] = {0};

    stJiffies curJiffies, diffJiffies;

    pStat = fopen("/metric/proc/stat", "r");
    fscanf(pStat, "%s %d %d %d %d", cpuID, &curJiffies.user,
            &curJiffies.nice, &curJiffies.system, &curJiffies.idle);

    fclose(pStat);

    diffJiffies.user =  curJiffies.user - cpu_.stJiffies_.user;
    diffJiffies.nice =  curJiffies.nice - cpu_.stJiffies_.nice;
    diffJiffies.system =  curJiffies.system - cpu_.stJiffies_.system;
    diffJiffies.idle =  curJiffies.idle - cpu_.stJiffies_.idle;

    int totalJiffies = diffJiffies.user + diffJiffies.nice + diffJiffies.system + diffJiffies.idle;

    cpu_.utilization = std::round(100.0 * (1.0 - (diffJiffies.idle / static_cast<double>(totalJiffies))) * 100) / 100;
    cpu_.used = std::round(cpu_.total * (1.0 - (diffJiffies.idle / static_cast<double>(totalJiffies))) * 100) / 100;
    
    cpu_.stJiffies_ = curJiffies;
}

void CsdMetricCollector::update_memory(){
    std::ifstream meminfo("/metric/proc/meminfo");

    std::string line;
    while (std::getline(meminfo, line)) {
        std::istringstream iss(line);
        std::string key;
        long value;

        iss >> key >> value;

        if (key == "MemFree:") {
            memory_.free = value;
        } else if (key == "Buffers:") {
            memory_.buffers = value;
        } else if (key == "Cached:") {
            memory_.cached = value;
        }
    }
    
    double freeGB = (memory_.free + memory_.buffers + memory_.cached) / 1024.0 / 1024.0;
    memory_.used = memory_.total - freeGB;
    memory_.utilization = std::round((memory_.used / memory_.total) * 100.0 * 100) / 100;
}

void CsdMetricCollector::update_network(){
    string statisticsFilePath = "/metric/net/";
    // string statisticsFilePath = "/sys/class/net/eno1/statistics/";
    
    string rxBytesFieldName = statisticsFilePath + "rx_bytes";
    string txBytesFieldName = statisticsFilePath + "tx_bytes";

    string currentRxBytesStr = readStatisticsField(rxBytesFieldName);
    string currentTxBytesStr = readStatisticsField(txBytesFieldName);
    
    long long currentRxBytes = std::stoll(currentRxBytesStr);
    long long currentTxBytes = std::stoll(currentTxBytesStr);

    network_.rxData= currentRxBytes - network_.rxBytes;
    network_.txData = currentTxBytes - network_.txBytes;

    network_.used = (network_.rxData + network_.txData) / 5 * 8; //5초동안 총 네트워크 전송량
    
    network_.rxBytes = currentRxBytes;
    network_.txBytes = currentTxBytes;
}

void CsdMetricCollector::update_storage(){
    std::string dfCommand = "df -h";
    std::string dfOutput;
    FILE* pipe = popen(dfCommand.c_str(), "r");
    char buffer[128];
    
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        dfOutput += buffer;
    }
    pclose(pipe);

    std::istringstream stream(dfOutput);
    std::string line;

    std::getline(stream, line); 

    while (std::getline(stream, line)) {
        std::istringstream lineStream(line);
        std::string filesystem, size, used, avail, usePercent, mountPoint;

        lineStream >> filesystem >> size >> used >> avail >> usePercent >> mountPoint;

        if (filesystem == "/dev/ngd-blk3") {
            // Remove 'G' from size and used, and convert to double
            size.erase(size.find_last_not_of("G") + 1);
            used.erase(used.find_last_not_of("G") + 1);

            double total = std::stod(size);   // Total size in GB
            double usedSpace = std::stod(used); // Used space in GB
            double utilization = std::round((usedSpace / total) * 100 * 100) / 100; // Utilization in %

            storage_.total = total;
            storage_.used = usedSpace;
            storage_.utilization = utilization;
        }
    }
}

void CsdMetricCollector::update_power(){
    string powerFilePath1 = "intel-rapl:0";
    string powerFilePath2 = "intel-rapl:1";

    string energyFieldName1 = "/sys/class/powercap/" + powerFilePath1 + "/energy_uj"; 
    string energyFieldName2 = "/sys/class/powercap/" + powerFilePath2 + "/energy_uj"; 

    string currentEnergyStr1 = readStatisticsField(energyFieldName1);
    string currentEnergyStr2 = readStatisticsField(energyFieldName2);

    long long currentEnergy1 = std::stoll(currentEnergyStr1);
    long long currentEnergy2 = std::stoll(currentEnergyStr2);

    power_.used= ((currentEnergy1 - power_.energy1) + (currentEnergy2 - power_.energy2));
}

void CsdMetricCollector::get_csd_working_block_count(){
    string address = "http://localhost:" + std::to_string(CSD_WORKER_MODULE_PORT);

    httplib::Client cli(address.c_str());
    auto res = cli.Get("/blockcount");

    if (res && res->status == 200) {
        rapidjson::Document document;

        if (document.Parse(res->body.c_str()).HasParseError()) {
            return;
        }
        
        if (document.HasMember("current_block_count")) {
            working_block_count_ = document["current_block_count"].GetInt();
        }
    } 
}

void CsdMetricCollector::calcul_csd_metric_score(){
    unique_lock<mutex> lock(weight_mutex_);

    double totalScore = cpu_weight_ * (100 - cpu_.utilization) +
                        memory_weight_ * (100 - memory_.utilization) +
                        storage_weight_ * (100 - memory_.utilization);

    csd_metric_score_ = std::round(totalScore * 100) / 100;
}

void CsdMetricCollector::handle_get_csd_metric(const httplib::Request& request, httplib::Response& response){
    string responseStr = serialize_response();
    response.set_content(responseStr, "application/json");
}

void CsdMetricCollector::handle_set_score_weight(const httplib::Request& request, httplib::Response& response){
    unique_lock<mutex> lock(weight_mutex_);
 
    string str = request.get_param_value("cpu_weight");
    cpu_weight_ = std::stof(str);

    str = request.get_param_value("memory_weight");
    memory_weight_ = std::stof(str);

    str = request.get_param_value("storage_weight");
    storage_weight_ = std::stof(str);

    // str = request.get_param_value("network_weight");
    // network_weight_ = std::stof(str);

    // str = request.get_param_value("power_weight");
    // power_weight_ = std::stof(str);
    
    response.status = 200;
}