#!/usr/bin/env bash

desc="/root/opencsd/T-CSD-Worker-Module"
password="1234"

for((i=1;i<2;i++)); 
    do
        ip="10.1.$i.2"
        echo scp -rp ./build/t-csd-worker-module root@$ip:$desc/ copying...
        sshpass -p $password scp -rp -o ConnectTimeout=60 ./build/t-csd-worker-module root@$ip:$desc/
    done
