#!/usr/bin/env bash

desc="/root/opencsd/CSD-Worker-Module"
password="1234"
project="local"
ip="220.94.140.246"

if [ -z "$1" ]; then
  echo "Please provide an argument (cross or copy path):"
  read arg
else
  arg=$1
fi

if [ "$2" = "" ]; then
    if [ "$arg" = "cross" ]; then
        for((i=1;i<9;i++)); 
            do
                ip="10.1.$i.2"
                echo scp -rp container-processing.sh ./build/$project dockerfile root@$ip:$desc copying...
                sshpass -p $password scp -rp -o ConnectTimeout=60 container-processing.sh dockerfile ./build/$project root@$ip:$desc
            done
    else
        for((i=1;i<9;i++)); 
            do
                ip="10.1.$i.2"
                echo scp -rp $arg root@$ip:$desc/$arg copying...
                sshpass -p $password scp -rp -o ConnectTimeout=60 $arg root@$ip:$desc/$arg
            done
    fi
else
    echo scp -rp container-processing.sh dockerfile ./build/$project root@$ip:/root/workspace/keti/CSD-Worker-Module copying...
    sshpass -p ketidbms! scp -rp -o ConnectTimeout=60 container-processing.sh dockerfile ./build/$project root@$ip:/root/workspace/keti/CSD-Worker-Module
fi

