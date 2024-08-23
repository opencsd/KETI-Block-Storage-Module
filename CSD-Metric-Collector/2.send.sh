# 컨테이너 빌드에 필요한 파일들 csd 폴더로 전송

desc="/root/opencsd/CSD-Metric-Collector/"
password="1234"

for((i=1;i<9;i++));
do
    ip="10.1.$i.2"
    echo scp -rp csd-metric-collector-tcpip-aarch64 dockerfile container-processing.sh root@$ip:$desc copying...
    sshpass -p $password scp -rp -o ConnectTimeout=60 csd-metric-collector-tcpip-aarch64 dockerfile container-processing.sh root@$ip:$desc

    # sshpass -p $password scp -rp -o ConnectTimeout=60 $file_name root@$ip:$file_path2
done
