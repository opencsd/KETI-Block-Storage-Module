cmake -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
      -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
      -DCMAKE_FIND_ROOT_PATH=/usr/lib/aarch64-linux-gnu \
      -DCMAKE_INSTALL_PREFIX=/usr/lib/aarch64-linux-gnu \
      -DWITH_ALL_TESTS=OFF \
      -DWITH_TESTS=OFF \
      -DWITH_TOOLS=OFF \
      -DWITH_GFLAGS=OFF \
      -DPORTABLE=ON  ..