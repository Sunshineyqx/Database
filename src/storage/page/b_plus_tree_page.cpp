//===----------------------------------------------------------------------===//
//
//                         CMU-DB Project (15-445/645)
//                         ***DO NO SHARE PUBLICLY***
//
// Identification: src/page/b_plus_tree_page.cpp
//
// Copyright (c) 2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/page/b_plus_tree_page.h"
#include "common/exception.h"

namespace bustub {

/*
 * Helper methods to get/set page type
 * Page type enum class is defined in b_plus_tree_page.h
 */
auto BPlusTreePage::IsLeafPage() const -> bool { return page_type_ == IndexPageType::LEAF_PAGE; }
auto BPlusTreePage::IsInternalPage() const -> bool { return page_type_ == IndexPageType::INTERNAL_PAGE; }
void BPlusTreePage::SetPageType(IndexPageType page_type) { page_type_ = page_type; }
/*
 * Helper methods to get/set size (number of key/value pairs stored in that
 * page)
 */
auto BPlusTreePage::GetSize() const -> int { return size_; }
void BPlusTreePage::SetSize(int size) { size_ = size; }
void BPlusTreePage::IncreaseSize(int amount) { size_ += amount; }

/*
 * Helper methods to get/set max size (capacity) of the page
 */
auto BPlusTreePage::GetMaxSize() const -> int { return max_size_; }
void BPlusTreePage::SetMaxSize(int size) { max_size_ = size; }

/*
 * Helper method to get min page size
 * Generally, min page size == max page size / 2; // 需要区分
 */
// 大小小于minsize时需要合并或者借kv(均从兄弟节点)
auto BPlusTreePage::GetMinSize() const -> int {
  /*
  if (page_type_ == IndexPageType::INTERNAL_PAGE) {
    return (max_size_ + 1) / 2; // wait
  }
  */
  return max_size_ / 2;
}

// 补充: 判断页面执行某只操作是否安全
auto BPlusTreePage::IsSafe(int opFlag) const -> bool {
  if (opFlag == OpInsert) {
    return this->GetSize() < this->GetMaxSize() - 1;
  }
  if (opFlag == OpDelete) {
    return this->GetSize() > this->GetMinSize();
  }
  throw Exception("对错误的操作判断是否安全");
}

}  // namespace bustub
