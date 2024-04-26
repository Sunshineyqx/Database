//===----------------------------------------------------------------------===//
//
//                         CMU-DB Project (15-445/645)
//                         ***DO NO SHARE PUBLICLY***
//
// Identification: src/include/index/index_iterator.h
//
// Copyright (c) 2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
/**
 * index_iterator.h
 * For range scan of b+ tree
 */
#pragma once
#include "buffer/buffer_pool_manager.h"
#include "common/config.h"
#include "storage/page/b_plus_tree_leaf_page.h"
#include "storage/page/b_plus_tree_page.h"

namespace bustub {

#define INDEXITERATOR_TYPE IndexIterator<KeyType, ValueType, KeyComparator>

INDEX_TEMPLATE_ARGUMENTS
class IndexIterator {
 public:
  // you may define your own constructor based on your member variables
  IndexIterator(BufferPoolManager *bpm, page_id_t cur_page_id, int cur_index);
  ~IndexIterator();  // NOLINT
  // 返回迭代器是否指向最后一个键/值对
  auto IsEnd() -> bool;
  // 返回迭代器当前指向的键/值对
  auto operator*() -> const MappingType &;
  // 返回迭代器是否指向最后一个键/值对
  auto operator++() -> IndexIterator &;
  // 返回两个迭代器是否相等
  auto operator==(const IndexIterator &itr) const -> bool {
    return this->bpm_ == itr.bpm_ && this->cur_page_id_ == itr.cur_index_ && this->cur_page_id_ == itr.cur_page_id_;
  }
  // 返回两个迭代器是否不相等
  auto operator!=(const IndexIterator &itr) const -> bool { return !(*this == itr); }

 private:
  // add your own private member variables here
  BufferPoolManager *bpm_;
  page_id_t cur_page_id_;
  int cur_index_;
  MappingType entry_;
};

}  // namespace bustub
