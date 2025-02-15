//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once
#include "memory/memory_allocator.h"
#include "table/block_based/block.h"
#include "table/block_based/block_type.h"
#include "table/format.h"
#include "table/persistent_cache_options.h"

namespace ROCKSDB_NAMESPACE {

// Retrieves a single block of a given file. Utilizes the prefetch buffer and/or
// persistent cache provided (if any) to try to avoid reading from the file
// directly. Note that both the prefetch buffer and the persistent cache are
// optional; also, note that the persistent cache may be configured to store either
// compressed or uncompressed blocks.
//
// If the retrieved block is compressed and the do_uncompress flag is set,
// BlockFetcher uncompresses the block (using the uncompression dictionary,
// if provided, to prime the compression algorithm), and returns the resulting
// uncompressed block data. Otherwise, it returns the original block.
//
// Two read options affect the behavior of BlockFetcher: if verify_checksums is
// true, the checksum of the (original) block is checked; if fill_cache is true,
// the block is added to the persistent cache if needed.
//
// Memory for uncompressed and compressed blocks is allocated as needed
// using memory_allocator and memory_allocator_compressed, respectively
// (if provided; otherwise, the default allocator is used).

class BlockFetcher {
 public:
  BlockFetcher(RandomAccessFileReader* file,
               FilePrefetchBuffer* prefetch_buffer,
               const Footer& footer /* ref retained */,
               const ReadOptions& read_options,
               const BlockHandle& handle /* ref retained */,
               BlockContents* contents,
               const ImmutableOptions& ioptions /* ref retained */,
               bool do_uncompress, bool maybe_compressed, BlockType block_type,
               const UncompressionDict& uncompression_dict /* ref retained */,
               const PersistentCacheOptions& cache_options /* ref retained */,
               MemoryAllocator* memory_allocator = nullptr,
               MemoryAllocator* memory_allocator_compressed = nullptr,
               bool for_compaction = false,
               const int fd = 0)
      : file_(file),
        prefetch_buffer_(prefetch_buffer),
        footer_(footer),
        read_options_(read_options),
        handle_(handle),
        contents_(contents),
        ioptions_(ioptions),
        do_uncompress_(do_uncompress),
        maybe_compressed_(maybe_compressed),
        block_type_(block_type),
        block_size_(static_cast<size_t>(handle_.size())),
        block_size_with_trailer_(block_size_ + footer.GetBlockTrailerSize()),
        uncompression_dict_(uncompression_dict),
        cache_options_(cache_options),
        memory_allocator_(memory_allocator),
        memory_allocator_compressed_(memory_allocator_compressed),
        for_compaction_(for_compaction),
        fd_(fd) {
    io_status_.PermitUncheckedError();  // TODO(AR) can we improve on this?
  }

  IOStatus ReadBlockContents();
  IOStatus ReadBlockContentsByOffset();//추가

  inline CompressionType get_compression_type() const {
    return compression_type_;
  }
  inline size_t GetBlockSizeWithTrailer() const {
    return block_size_with_trailer_;
  }
  inline Slice GetData() const {
    return slice_;
  }

#ifndef NDEBUG
  int TEST_GetNumStackBufMemcpy() const { return num_stack_buf_memcpy_; }
  int TEST_GetNumHeapBufMemcpy() const { return num_heap_buf_memcpy_; }
  int TEST_GetNumCompressedBufMemcpy() const {
    return num_compressed_buf_memcpy_;
  }

#endif
 private:
#ifndef NDEBUG
  int num_stack_buf_memcpy_ = 0;
  int num_heap_buf_memcpy_ = 0;
  int num_compressed_buf_memcpy_ = 0;

#endif
  static const uint32_t kDefaultStackBufferSize = 5000;

  RandomAccessFileReader* file_;
  FilePrefetchBuffer* prefetch_buffer_;
  const Footer& footer_;
  const ReadOptions read_options_;
  const BlockHandle& handle_;
  BlockContents* contents_;
  const ImmutableOptions& ioptions_;
  const bool do_uncompress_;
  const bool maybe_compressed_;
  const BlockType block_type_;
  const size_t block_size_;
  const size_t block_size_with_trailer_;
  const UncompressionDict& uncompression_dict_;
  const PersistentCacheOptions& cache_options_;
  MemoryAllocator* memory_allocator_;
  MemoryAllocator* memory_allocator_compressed_;
  IOStatus io_status_;
  Slice slice_;
  char* used_buf_ = nullptr;
  AlignedBuf direct_io_buf_;
  CacheAllocationPtr heap_buf_;
  CacheAllocationPtr compressed_buf_;
  char stack_buf_[kDefaultStackBufferSize];
  bool got_from_prefetch_buffer_ = false;
  CompressionType compression_type_;
  bool for_compaction_ = false;
  const int fd_;//추가

  // return true if found
  bool TryGetUncompressBlockFromPersistentCache();
  // return true if found
  bool TryGetFromPrefetchBuffer();
  bool TryGetCompressedBlockFromPersistentCache();
  void PrepareBufferForBlockFromFile();
  // Copy content from used_buf_ to new heap_buf_.
  void CopyBufferToHeapBuf();
  // Copy content from used_buf_ to new compressed_buf_.
  void CopyBufferToCompressedBuf();
  void GetBlockContents();
  void InsertCompressedBlockToPersistentCacheIfNeeded();
  void InsertUncompressedBlockToPersistentCacheIfNeeded();
  void ProcessTrailerIfPresent();
};


class PbaBlockFetcher {
 public:
  PbaBlockFetcher(
    std::vector<uint64_t> _block_size, std::string _dev_name, std::vector<uint64_t> _block_offset,
    FilePrefetchBuffer* _prefetch_buffer, const ReadOptions& read_options, 
    BlockContents* contents,
    BlockType block_type, bool _using_zstd, 
    const UncompressionDict& uncompression_dict,
    MemoryAllocator* memory_allocator = nullptr)
    : prefetch_buffer(_prefetch_buffer),
      read_options_(read_options),
      contents_(contents),
      block_type_(block_type),
      uncompression_dict_(uncompression_dict),
      memory_allocator_(memory_allocator){
      // block_size = block_size_with_trailer - footer_block_trailer_size;
      block_size_with_trailer = 0;
      for(uint64_t i=0; i<_block_size.size(); i++){
        block_size_with_trailer += _block_size[i];
      }
      dev_name = _dev_name;
      block_offset = _block_offset;
      block_size = _block_size;
      using_zstd = _using_zstd;
      io_status_.PermitUncheckedError(); 
  }
  ~PbaBlockFetcher(){
	  //free(used_buf_);
  }
  IOStatus ReadBlockContentsByPBA();

  inline CompressionType get_compression_type() const {
    return compression_type_;
  }
  inline size_t GetBlockSizeWithTrailer() const {
    return block_size_with_trailer;
  }
  inline Slice GetData() const {
    return slice_;
  }

#ifndef NDEBUG
  int TEST_GetNumStackBufMemcpy() const { return num_stack_buf_memcpy_; }
  int TEST_GetNumHeapBufMemcpy() const { return num_heap_buf_memcpy_; }
  int TEST_GetNumCompressedBufMemcpy() const {
    return num_compressed_buf_memcpy_;
  }

#endif
 private:
#ifndef NDEBUG
  int num_stack_buf_memcpy_ = 0;
  int num_heap_buf_memcpy_ = 0;
  int num_compressed_buf_memcpy_ = 0;

#endif
  static const uint32_t kDefaultStackBufferSize = 5000;

  uint64_t block_size_with_trailer;
  std::vector<uint64_t> block_size;
  std::string dev_name;
  std::vector<uint64_t> block_offset;
  FilePrefetchBuffer* prefetch_buffer;
  const ReadOptions read_options_;
  bool using_zstd;
  CompressionType compression_type_;
  BlockContents* contents_;
  const BlockType block_type_;
  IOStatus io_status_;
  size_t footer_block_trailer_size = 5;
  const UncompressionDict& uncompression_dict_;

  Slice slice_;
  
  MemoryAllocator* memory_allocator_;
  CacheAllocationPtr heap_buf_;
  char* used_buf_ = nullptr;
  char internal_buf[5000];

  // return true if found
  
  void GetBlockContentsByPBA();
  void CopyBufferToHeapBuf();
  void PrepareBufferForBlockFromFile();
  void ProcessTrailerIfPresent();

};
}  // namespace ROCKSDB_NAMESPACE
