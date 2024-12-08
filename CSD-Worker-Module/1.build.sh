#!/bin/bash
registry="ketidevit2"
image_name="csd-worker-module"
version="v3.0"

if [ -z "$1" ]; then
  echo "Please provide an argument (cross or local):"
  read arg
else
  arg=$1
fi

case $arg in
  cross)
    echo "Executing command for cross"
    (cd build && make -j)
    ;;
  local)
    echo "Executing command for local"
    (cd keti && make -j)
    ;;
  docker)
    echo "Executing command for docker"
    (# make image
    docker build -t $image_name:$version . && \
    # add tag
    docker tag $image_name:$version $registry/$image_name:$version && \
    # login
    docker login && \
    # push image
    docker push $registry/$image_name:$version)
    ;;
  *)
    echo "Invalid argument. Please provide either 'cross' or 'local'."
    ;;
esac