/**
 * index_iterator.cpp
 */
#include <cassert>
#include "common/config.h"
#include "common/macros.h"
#include "storage/page/b_plus_tree_leaf_page.h"
#include "storage/page/page_guard.h"

#include "storage/index/index_iterator.h"

namespace bustub {

/*
 * NOTE: you can change the destructor/constructor method here
 * set your own input parameters
 */
INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::IndexIterator(BufferPoolManager *bpm, page_id_t cur_page_id, int cur_index)
    : bpm_(bpm), cur_page_id_(cur_page_id), cur_index_(cur_index) {
  if(cur_page_id == INVALID_PAGE_ID){
    return;
  }
  ReadPageGuard page_readguard = bpm_->FetchPageRead(cur_page_id_);
  auto *page = page_readguard.As<B_PLUS_TREE_LEAF_PAGE_TYPE>();
  entry_.first = page->KeyAt(cur_index_);
  entry_.second = page->ValAt(cur_index_);
}

INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::~IndexIterator() = default;  // NOLINT

INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::IsEnd() -> bool {
  ReadPageGuard page_readguard = bpm_->FetchPageRead(cur_page_id_);
  auto *page = page_readguard.As<B_PLUS_TREE_LEAF_PAGE_TYPE>();
  return (page->GetNextPageId() == INVALID_PAGE_ID) && (cur_index_ == page->GetSize() - 1);
}

INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::operator*() -> const MappingType & {
  BUSTUB_ASSERT(this->cur_page_id_ != INVALID_PAGE_ID, "INDEXITERATOR_TYPE::operator*(): 对尾迭代器解引用 不可以。");
  return entry_;
}

INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::operator++() -> INDEXITERATOR_TYPE & {
  // 若当前迭代器已经是尾迭代器
  if (cur_page_id_ == INVALID_PAGE_ID) {
    return *this;
  }
  // 若下一个是尾迭代器
  if (IsEnd()) {
    cur_page_id_ = INVALID_PAGE_ID;
    cur_index_ = -1;
    return *this;
  }
  // 令迭代器指向下一个键值对
  ReadPageGuard page_readguard = bpm_->FetchPageRead(cur_page_id_);
  auto *page = page_readguard.As<B_PLUS_TREE_LEAF_PAGE_TYPE>();
  // 若cur_index_指向本页面最后一个键值对
  if (cur_index_ == page->GetSize() - 1) {
    ReadPageGuard next_readguard = bpm_->FetchPageRead(page->GetNextPageId());
    auto *next_page = next_readguard.As<B_PLUS_TREE_LEAF_PAGE_TYPE>();
    cur_page_id_ = next_readguard.PageId();
    cur_index_ = 0;
    entry_.first = next_page->KeyAt(0);
    entry_.second = next_page->ValAt(0);
    return *this;
  }
  // cur_index_指向的kv后有kvs
  cur_index_++;
  entry_.first = page->KeyAt(cur_index_);
  entry_.second = page->ValAt(cur_index_);
  return *this;
}

template class IndexIterator<GenericKey<4>, RID, GenericComparator<4>>;

template class IndexIterator<GenericKey<8>, RID, GenericComparator<8>>;

template class IndexIterator<GenericKey<16>, RID, GenericComparator<16>>;

template class IndexIterator<GenericKey<32>, RID, GenericComparator<32>>;

template class IndexIterator<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
