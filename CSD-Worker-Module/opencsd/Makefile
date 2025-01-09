#include ../rocksdb/make_config.mk

ifndef DISABLE_JEMALLOC
	ifdef JEMALLOC
		PLATFORM_CXXFLAGS += -DROCKSDB_JEMALLOC -DJEMALLOC_NO_DEMANGLE
	endif
	EXEC_LDFLAGS := $(JEMALLOC_LIB) $(EXEC_LDFLAGS) -lpthread -lbz2 -llz4 -lsnappy -lz -lzstd -ldl
	PLATFORM_CXXFLAGS += $(JEMALLOC_INCLUDE)
endif

ifneq ($(USE_RTTI), 1)
	CXXFLAGS += -fno-rtti
endif

CFLAGS += -Wstrict-prototypes
CROSS_COMPILER = arm-linux-gnueabi-g++

SOURCES += input.cc scan.cc filter.cc projection.cc return.cc

.PHONY: clean librocksdb

all: main

clean:
	rm -rf ./server
	rm -rf ./test
	rm -rf ./sstopen

librocksdb:
	cd ../rocksdb && $(MAKE) static_lib

main: librocksdb main.cc
	$(CXX) $(CXXFLAGS) $@.cc $(SOURCES) -g -o$@ ../rocksdb/librocksdb.a -I../rocksdb/include -I./include -L/usr/local/lib -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)	
