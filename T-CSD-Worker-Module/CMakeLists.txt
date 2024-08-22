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

# set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++ ")

# Include directories
set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tmax-lib/include)
include_directories(${INC_DIR})


# # 디버그 빌드 타입 설정
# set(CMAKE_BUILD_TYPE Debug)

# # 디버그 플래그 추가
# set(CMAKE_C_FLAGS_DEBUG "-g")
# set(CMAKE_CXX_FLAGS_DEBUG "-g")


# Add library
add_library(tmax STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/tmax-lib/data_blk.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/tmax-lib/ex.cc"
)

# Add executable`
add_executable(t-csd-worker-module 
    "main.cc"
    "worker.cc"
)

# Link libraries
target_link_libraries(t-csd-worker-module 
  tmax
  pthread
)

# Debugging messages
message(STATUS "CMake C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "CMake CXX Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMake Project Source Dir: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMake Project Binary Dir: ${CMAKE_BINARY_DIR}")
