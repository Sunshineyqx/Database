//===----------------------------------------------------------------------===//
//
//                         CMU-DB Project (15-445/645)
//                         ***DO NO SHARE PUBLICLY***
//
// Identification: src/page/b_plus_tree_internal_page.cpp
//
// Copyright (c) 2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <cassert>
#include <iostream>
#include <sstream>

#include "common/config.h"
#include "common/exception.h"
#include "common/macros.h"
#include "storage/page/b_plus_tree_page.h"
#include "storage/page/b_plus_tree_internal_page.h"

namespace bustub {
/*****************************************************************************
 * HELPER METHODS AND UTILITIES
 *****************************************************************************/
/*
 * Init method after creating a new internal page
 * Including set page type, set current size, and set max page size
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Init(int max_size) {
  SetPageType(IndexPageType::INTERNAL_PAGE);
  SetSize(0);
  SetMaxSize(max_size);
}

/*
* 补充: 在节点内根据key寻找第一个key <= K的kv的val (but 从1开始查询 (内部节点第一个kv的k无效))
*      1. 如果key大于全部k，返回最后一个非空kv的val
*      2. 否则如果key == k，返回此val
*      3. 否则如果key < k, 返回前一个val
*/
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::LookUpV(const KeyType& key, KeyComparator &cmp) const  -> ValueType {
  int left = 1;
  int right = this->GetSize() - 1;
  while(left < right){
    int mid = (right - left)/2 + left;
    if(cmp(key, array_[mid].first) <= 0){
      right = mid;
    }
    else{
      left = mid + 1;
    }
  }
  auto k = array_[left].first;
  auto v = array_[left].second;
  if(cmp(key, k) >= 0){
    return v;
  }
  return array_[left -1].second;
}

// 分裂
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::SplitTo(B_PLUS_TREE_INTERNAL_PAGE_TYPE* new_page) -> void {
  int n = GetSize();
  int start = 0;
  for(int i = n/2; i < n; i++){
    new_page->SetKeyAt(start, KeyAt(i));
    new_page->SetValAt(start, ValueAt(i));
    start++;
  }
  int gap = n - (n / 2);
  new_page->SetSize(gap);
  this->SetSize(n - gap);
}

// 将本内部节点作为空root，插入key并修改v
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::InsertAsRoot(page_id_t old_page_id, const KeyType& key, page_id_t new_page_id) -> void{
  BUSTUB_ASSERT(this->GetSize() == 0, "B_PLUS_TREE_INTERNAL_PAGE_TYPE::InsertAsRoot(): 新root的size不为0");
  this->array_[0].second = old_page_id;
  this->array_[1].first = key;
  this->array_[1].second = new_page_id;
  this->SetSize(2);
}

// 在本内部节点内部插入key，并修改v
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::InsertKVAfter(const KeyType &key, page_id_t new_page_id, KeyComparator &cmp) -> void {
  int left = 1;
  int right = this->GetSize() - 1;
  while(left < right){
    int mid = (right - left)/2 + left;
    if(cmp(key, KeyAt(mid)) < 0){
      right = mid;
    }
    else if(cmp(key, KeyAt(mid)) == 0){
      throw Exception("B_PLUS_TREE_INTERNAL_PAGE_TYPE::InsertKVAfter(): 存在重复的key...无法插入");
    }
    else{
      left = mid + 1;
    }
  }
  auto k = array_[left].first;
  // 插入到最后
  if(cmp(key, k) > 0){
    int index = this->GetSize();
    this->SetKeyAt(index, key);
    this->SetValAt(index, new_page_id);
    this->IncreaseSize(1);
    return;
  }
  // cmp(key, k) < 0; 插入到index前面
  BUSTUB_ASSERT(this->GetSize() < this->GetMaxSize(), "B_PLUS_TREE_INTERNAL_PAGE_TYPE::InsertKVAfter(): 节点已满，无法插入");
  int index = left;
  int n = this->GetSize();
  for(int i = n; i > index; i--){
    this->SetKeyAt(i, KeyAt(i-1));
    this->SetValAt(i, ValueAt(i-1));
  }
  this->SetKeyAt(index, key);
  this->SetValAt(index, new_page_id);
  this->IncreaseSize(1);
}


/*
 * Helper method to get/set the key associated with input "index"(a.k.a
 * array offset)
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::KeyAt(int index) const -> KeyType {
  BUSTUB_ASSERT(index >= 0, "keyAt(int index): index必须大于等于0.");
  KeyType key{array_[index].first};
  return key;
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::SetKeyAt(int index, const KeyType &key) {
  BUSTUB_ASSERT(index >= 0, "SetKeyAt(int index, const KeyType &key): index必须大于等于0.");
  array_[index].first = key;
}

// 补充: 在index处设置新值
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::SetValAt(int index, ValueType val) {
  BUSTUB_ASSERT(index >= 0, "SetKeyAt(int index, const KeyType &key): index必须大于等于0.");
  array_[index].second = val;
}

/* // 补充
 * Helper method to search the index associated with input "value"
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::ValueIndex(const ValueType &value) const -> int{
  throw Exception("B_PLUS_TREE_INTERNAL_PAGE_TYPE::ValueIndex(const ValueType &value):not complete");
};

/*
 * Helper method to get the value associated with input "index"(a.k.a array
 * offset)
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::ValueAt(int index) const -> ValueType {
  BUSTUB_ASSERT(index >= 0, "ValueAt(int index): index必须大于等于0.");
  return array_[index].second; 
}

// valuetype for internalNode should be page id_t
template class BPlusTreeInternalPage<GenericKey<4>, page_id_t, GenericComparator<4>>;
template class BPlusTreeInternalPage<GenericKey<8>, page_id_t, GenericComparator<8>>;
template class BPlusTreeInternalPage<GenericKey<16>, page_id_t, GenericComparator<16>>;
template class BPlusTreeInternalPage<GenericKey<32>, page_id_t, GenericComparator<32>>;
template class BPlusTreeInternalPage<GenericKey<64>, page_id_t, GenericComparator<64>>;
}  // namespace bustub
