# 배열에 여러 개의 IP 주소를 저장
# ip_addresses=("192.168.1.1" "192.168.1.2" "192.168.1.3")
#num_of_csd=$1

# 배열을 순회하면서 각 IP에 접속
#for((i=1;i<$num_of_csd+1;i++)); 

for((i=1;i<9;i++)); 
do
    ip="10.1.$i.2"
    echo "Connecting to $ip"

    # 1.csd 접속
    ssh -n root@$ip "cd /root/opencsd/CSD-Metric-Collector && ./container-processing.sh" 
    echo "Connection to $ip completed"
done
