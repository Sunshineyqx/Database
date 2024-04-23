//===----------------------------------------------------------------------===//
//
//                         CMU-DB Project (15-445/645)
//                         ***DO NO SHARE PUBLICLY***
//
// Identification: src/page/b_plus_tree_leaf_page.cpp
//
// Copyright (c) 2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <sstream>
#include <utility>

#include "common/config.h"
#include "common/exception.h"
#include "common/macros.h"
#include "common/rid.h"
#include "storage/page/b_plus_tree_leaf_page.h"
#include "storage/page/b_plus_tree_page.h"

namespace bustub {

/*****************************************************************************
 * HELPER METHODS AND UTILITIES
 *****************************************************************************/

/**
 * Init method after creating a new leaf page
 * Including set page type, set current size to zero, set next page id and set max size
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::Init(int max_size) {
  SetPageType(IndexPageType::LEAF_PAGE);
  SetSize(0);
  SetMaxSize(max_size);
  SetNextPageId(INVALID_PAGE_ID);
}

/*
 * 补充函数
 *
 */
// 将自己一半的kv划分给new_page,并修改next_page_id、size
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::SplitTo(B_PLUS_TREE_LEAF_PAGE_TYPE* new_page, page_id_t new_page_id) -> void {
  int n = GetSize();
  int start = 0;
  for(int i = n/2; i < n; i++){
    new_page->array_[start++].first = this->array_[i].first;
    new_page->array_[start++].second = this->array_[i].second;
  }

  int gap = n - (n / 2);
  new_page->SetSize(gap);
  this->SetSize(n - gap);
  new_page->SetNextPageId(this->GetNextPageId());
  this->SetNextPageId(new_page_id);
}

// 根据key二分查找第一个key==K的节点的kv的index
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::LookUpIndex(const KeyType &key, KeyComparator &cmp) const
    -> int {
  int left = 0;
  int right = this->GetSize() - 1;
  int index = -1;
  while (left <= right) {
    int mid = (right - left) / 2 + left;
    if (cmp(key, array_[mid].first) < 0) {
      right = mid - 1;
    } else if (cmp(key, array_[mid].first) > 0) {
      left = mid + 1;
    } else {
      index = mid;
      break;
    }
  }
  return index;
}

// 在节点内根据key寻找是否存在该key
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::LookUpIfExist(const KeyType& key, KeyComparator &cmp) const -> bool{
  int index = LookUpIndex(key, cmp);
  return index != -1;
}
// 在节点内根据key寻找key==K的节点的val
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::LookUpV(const KeyType &key, KeyComparator &cmp) const -> ValueType {
  int index = LookUpIndex(key, cmp);
  if(index == -1){
    throw Exception("B_PLUS_TREE_LEAF_PAGE_TYPE::LookUpV(): 未找到相应的key");
  }
  return array_[index].second;
}

// 插入kv, 必须保证不会满，且不能重复
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::InsertKV(const KeyType &key, const ValueType &value, KeyComparator &cmp) -> bool {
  BUSTUB_ASSERT(this->GetSize() < this->GetMaxSize(),
                "B_PLUS_TREE_LEAF_PAGE_TYPE::InsertKV(): 继续插入将使得叶子节点的kv溢出");
  // 特判:空(当创建一个叶子节点作为root插入kv时...)
  if(this->GetSize() == 0){
    SetKeyAt(0, key);
    SetValAt(0, value);
    this->IncreaseSize(1);
    return true;
  }
  // 不为空
  int left = 0;
  int right = this->GetSize() - 1;
  while (left < right) {
    int mid = (right - left) / 2 + left;
    if (cmp(key, array_[mid].first) <= 0) {
      right = mid;
    } else {
      left = mid + 1;
    }
  }
  auto index = left;
  auto k = array_[index].first;
  // 不可以重复
  if (cmp(key, k) == 0) {
    return false;
  }
  // key是最大的,插入到最后一个
  if(cmp(key, k) > 0){
    SetKeyAt(index + 1, key);
    SetValAt(index + 1, value);
    this->IncreaseSize(1);
    return true;
  }
  // 插入到index所指的位置
  for(int i = this->GetSize(); i > index; i--){
    SetKeyAt(i, KeyAt(i - 1));
    SetValAt(i, ValAt(i - 1));
  }
  SetKeyAt(index, key);
  SetValAt(index, value);
  this->IncreaseSize(1);
  return true;
}

/**
 * Helper methods to set/get next page id
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::GetNextPageId() const -> page_id_t { return next_page_id_; }

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::SetNextPageId(page_id_t next_page_id) { next_page_id_ = next_page_id; }

/*
 * Helper method to find and return the key associated with input "index"(a.k.a
 * array offset)
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::KeyAt(int index) const -> KeyType {
  BUSTUB_ASSERT(index >= 0, "keyAt(int index): index必须大于等于0.");
  KeyType key{array_[index].first};
  return key;
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::ValAt(int index) const -> ValueType {
  BUSTUB_ASSERT(index >= 0, "ValAt(int index): index必须大于等于0.");
  ValueType val{array_[index].second};
  return val;
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::SetKeyAt(int index, const KeyType& key) -> void{
  BUSTUB_ASSERT(index >= 0, "B_PLUS_TREE_LEAF_PAGE_TYPE::SetKeyAt(): index < 0");
  this->array_[index].first = key;
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::SetValAt(int index, const ValueType& val) -> void{
  BUSTUB_ASSERT(index >= 0, "B_PLUS_TREE_LEAF_PAGE_TYPE::SetValAt(): index < 0");
  this->array_[index].second = val;
}

template class BPlusTreeLeafPage<GenericKey<4>, RID, GenericComparator<4>>;
template class BPlusTreeLeafPage<GenericKey<8>, RID, GenericComparator<8>>;
template class BPlusTreeLeafPage<GenericKey<16>, RID, GenericComparator<16>>;
template class BPlusTreeLeafPage<GenericKey<32>, RID, GenericComparator<32>>;
template class BPlusTreeLeafPage<GenericKey<64>, RID, GenericComparator<64>>;
}  // namespace bustub
