cmake_minimum_required(VERSION 3.22.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_SYSTEM_PROCESSOR aarch64)

project(main C CXX)
SET(COMPILER_ROOT /usr/bin/aarch64-linux-gnu-)

SET(CMAKE_C_COMPILER ${COMPILER_ROOT}gcc)
SET(CMAKE_CXX_COMPILER ${COMPILER_ROOT}g++)
SET(CMAKE_LINKER ${COMPILER_ROOT}ld)
SET(CMAKE_NM ${COMPILER_ROOT}nm)
SET(CMAKE_OBJCOPY ${COMPILER_ROOT}objcopy)
SET(CMAKE_OBJDUMP ${COMPILER_ROOT}objdump)
SET(CMAKE_RANLIB ${COMPILER_ROOT}ranlib)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/rocksdb/include")

add_subdirectory(rocksdb)
add_executable(main 
"keti/csd_table_manager.cc"
"keti/filter.cc"
"keti/main.cc"
"keti/merge_manager.cc"
"keti/return.cc"
"keti/scan.cc"

)
target_link_libraries(main
rocksdb
pthread
)


