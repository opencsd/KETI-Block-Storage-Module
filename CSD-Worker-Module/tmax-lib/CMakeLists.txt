set(SOURCE_FILES
    data_blk.cc
    ex.cc
)

# 정적 라이브러리 tmax 생성
add_library(tmax STATIC ${SOURCE_FILES})
target_include_directories(tmax PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)