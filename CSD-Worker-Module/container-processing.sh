# 컨테이너 실행 중지 및 컨테이너, 이미지 삭제
docker stop csd-worker-module
docker rm csd-worker-module
docker rmi csd-worker-module

# 컨테이너 이미지 빌드시 csd ip 환경 변수로 부여
interface_name="ngdtap0"
interface_info=$(ifconfig "$interface_name" 2>/dev/null)
if [ $? -eq 0 ]; then
    inet_address=$(echo "$interface_info" | grep -oP 'inet\s+\K[0-9.]+') # csd ip 추출 

    if [ -n "$inet_address" ]; then
        echo "IP Address of $interface_name: $inet_address"
    else
        echo "No IP address found for $interface_name"
    fi
else
    echo "Error: Unable to retrieve information for $interface_name"
fi

# 2.컨테이너 이미지 빌드(dockerfile 기반)
docker build -t csd-worker-module --build-arg CSD_IP=$inet_address --build-arg CSD_HOST_SERVER_IP=$CSD_HOST_SERVER_IP .

# 3.컨테이너 실행 
docker run -d -it --restart=always --privileged -p 40302:40302 -p 40301:40301 \
    -v /home/ngd/storage:/home/ngd/storage \
    -e LOG_LEVEL=$1 -e STORAGE_ENGINE_HOST_SERVER_IP=$STORAGE_ENGINE_HOST_SERVER_IP \
    --name csd-worker-module csd-worker-module 
    