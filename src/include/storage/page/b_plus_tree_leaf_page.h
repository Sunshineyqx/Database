//===----------------------------------------------------------------------===//
//
//                         CMU-DB Project (15-445/645)
//                         ***DO NO SHARE PUBLICLY***
//
// Identification: src/include/page/b_plus_tree_leaf_page.h
//
// Copyright (c) 2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "common/config.h"
#include "storage/page/b_plus_tree_page.h"

namespace bustub {

#define B_PLUS_TREE_LEAF_PAGE_TYPE BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>
#define LEAF_PAGE_HEADER_SIZE 16
#define LEAF_PAGE_SIZE ((BUSTUB_PAGE_SIZE - LEAF_PAGE_HEADER_SIZE) / sizeof(MappingType))

/**
 * Store indexed key and record id(record id = page id combined with slot id,
 * see include/common/rid.h for detailed implementation) together within leaf
 * page. Only support unique key.
 *
 * NOTE: 
 * 1. Even though Leaf Pages and Internal Pages contain the same type of key, they may have different value types. 
 *    Thus, the max_size can be different.
 * 2. Leaf pages have the same restrictions on the number of key/value pairs as Internal pages, 
 *    and should follow the same operations for merging, splitting, and redistributing keys.
 * 3. Each B+Tree leaf/internal page corresponds to the content (i.e., the data_ part) of a memory page fetched by the buffer pool.
 *    Every time you read or write a leaf or internal page, you must first fetch the page from the buffer pool (using its unique page_id), 
 *    reinterpret cast it to either a leaf or an internal page, and unpin the page after reading or writing it.
 *
 * Leaf page format (keys are stored in order):
 *  ----------------------------------------------------------------------
 * | HEADER | KEY(1) + RID(1) | KEY(2) + RID(2) | ... | KEY(n) + RID(n)
 *  ----------------------------------------------------------------------
 *
 *  Header format (size in byte, 16 bytes in total):
 *  ---------------------------------------------------------------------
 * | PageType (4) | CurrentSize (4) | MaxSize (4) |
 *  ---------------------------------------------------------------------
 *  -----------------------------------------------
 * |  NextPageId (4)
 *  -----------------------------------------------
 */
INDEX_TEMPLATE_ARGUMENTS
class BPlusTreeLeafPage : public BPlusTreePage {
 public:
  // Delete all constructor / destructor to ensure memory safety
  BPlusTreeLeafPage() = delete;
  BPlusTreeLeafPage(const BPlusTreeLeafPage &other) = delete;

  /**
   * After creating a new leaf page from buffer pool, must call initialize
   * method to set default values
   * @param max_size Max size of the leaf node
   */
  void Init(int max_size = LEAF_PAGE_SIZE);

  /*
  * 补充
  */
  auto SplitTo(B_PLUS_TREE_LEAF_PAGE_TYPE* new_page, page_id_t new_page_id) -> void;
  auto LookUpIndex(const KeyType &key, KeyComparator &cmp) const -> int ; // 在节点内根据key寻找key==K的节点的Index
  auto LookUpIfExist(const KeyType& key, KeyComparator &cmp) const -> bool; // 在节点内根据key寻找是否存在该key
  auto LookUpV(const KeyType& key, KeyComparator &cmp) const  -> ValueType; // 在节点内根据key寻找key==K的节点的val
  auto InsertKV(const KeyType& key, const ValueType& value, KeyComparator &cmp) -> bool; // 插入kv, 必须保证不会满

  // helper methods
  auto GetNextPageId() const -> page_id_t;
  void SetNextPageId(page_id_t next_page_id);
  auto KeyAt(int index) const -> KeyType;
  auto ValAt(int index) const -> ValueType;
  auto SetKeyAt(int index, const KeyType& key) -> void;
  auto SetValAt(int index, const ValueType& val) -> void;

  /**
   * @brief for test only return a string representing all keys in
   * this leaf page formatted as "(key1,key2,key3,...)"
   *
   * @return std::string
   */
  auto ToString() const -> std::string {
    std::string kstr = "(";
    bool first = true;

    for (int i = 0; i < GetSize(); i++) {
      KeyType key = KeyAt(i);
      if (first) {
        first = false;
      } else {
        kstr.append(",");
      }

      kstr.append(std::to_string(key.ToString()));
    }
    kstr.append(")");

    return kstr;
  }

 private:
  page_id_t next_page_id_;
  // Flexible array member for page data.
  MappingType array_[0];
};
}  // namespace bustub
