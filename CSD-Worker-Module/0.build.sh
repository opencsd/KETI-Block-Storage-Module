#!/bin/bash

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
  *)
    echo "Invalid argument. Please provide either 'cross' or 'local'."
    ;;
esac