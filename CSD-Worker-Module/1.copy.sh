#!/usr/bin/env bash

desc="/root/opencsd/CSD-Worker-Module"
password="1234"

if [ -z "$1" ]; then
  echo "Please provide an argument (cross or copy path):"
  read arg
else
  arg=$1
fi

if [ "$arg" = "cross" ]; then
    for((i=1;i<9;i++)); 
        do
            ip="10.1.$i.2"
            echo scp -rp ./build/test root@$ip:$desc/build copying...
            sshpass -p $password scp -rp -o ConnectTimeout=60 ./build/test root@$ip:$desc/build
        done
else
    for((i=1;i<9;i++)); 
        do
            ip="10.1.$i.2"
            echo scp -rp $arg root@$ip:$desc/$arg copying...
            sshpass -p $password scp -rp -o ConnectTimeout=60 $arg root@$ip:$desc/$arg
        done
fi