#!/usr/bin/env bash

desc="/root/csd-worder-module"
#desc="/home/ngd/workspace/KETI-Block-Storage-Module/CSD-Worker-Module"
password="1234"

for((i=1;i<9;i++)); 
do
    ip="10.1.$i.2"
    echo scp -rp ./build/main root@$ip:$desc copying...
    sshpass -p $password scp -rp -o ConnectTimeout=60 ./build/main root@$ip:$desc
done


