# 컨테이너 빌드에 필요한 파일들 csd 폴더로 전송

desc="/root/opencsd/CSD-Metric-Collector/"
password="1234"
arg=$1

if [ "$arg" = "" ]; then
    for((i=1;i<9;i++));
    do
        ip="10.1.$i.2"
        echo scp -rp container-processing.sh csd-metric-collector-aarch64 dockerfile root@$ip:$desc copying...
        sshpass -p $password scp -rp -o ConnectTimeout=60 dockerfile container-processing.sh csd-metric-collector-aarch64 root@$ip:$desc
    done
else
    echo scp -rp dockerfile csd-metric-collector-aarch64 2.send.sh container-processing.sh root@10.0.4.84:/root/workspace/keti/CSD-Metric-Collector copying...
    sshpass -p ketidbms! scp -rp -o ConnectTimeout=60 dockerfile csd-metric-collector-aarch64 2.send.sh container-processing.sh root@10.0.4.84:/root/workspace/keti/CSD-Metric-Collector
fi

