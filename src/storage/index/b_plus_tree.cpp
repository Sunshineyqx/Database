#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

#include "common/config.h"
#include "common/exception.h"
#include "common/logger.h"
#include "common/macros.h"
#include "common/rid.h"
#include "storage/index/b_plus_tree.h"
#include "storage/index/index_iterator.h"
#include "storage/page/b_plus_tree_header_page.h"
#include "storage/page/b_plus_tree_internal_page.h"
#include "storage/page/b_plus_tree_leaf_page.h"
#include "storage/page/b_plus_tree_page.h"
#include "storage/page/page_guard.h"
#include "type/value.h"
// make b_plus_tree_insert_test -j8
// ./test/b_plus_tree_insert_test

// make b_plus_tree_delete_test -j8
// ./test/b_plus_tree_delete_test

// make b_plus_tree_sequential_scale_test -j8
// ./test/b_plus_tree_sequential_scale_test

// make b_plus_tree_concurrent_test -j8
// ./test/b_plus_tree_concurrent_test
namespace bustub {

INDEX_TEMPLATE_ARGUMENTS
BPLUSTREE_TYPE::BPlusTree(std::string name, page_id_t header_page_id, BufferPoolManager *buffer_pool_manager,
                          const KeyComparator &comparator, int leaf_max_size, int internal_max_size)
    : index_name_(std::move(name)),
      bpm_(buffer_pool_manager),
      comparator_(std::move(comparator)),
      leaf_max_size_(leaf_max_size),
      internal_max_size_(internal_max_size),
      header_page_id_(header_page_id) {
  WritePageGuard guard = bpm_->FetchPageWrite(header_page_id_);
  auto root_page = guard.AsMut<BPlusTreeHeaderPage>();
  root_page->root_page_id_ = INVALID_PAGE_ID;
}

/*
 * Helper function to decide whether current b+tree is empty
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::IsEmpty() const -> bool {
  if (this->header_page_id_ == INVALID_PAGE_ID) {
    throw Exception("BPLUSTREE_TYPE::IsEmpty(): Invalid header_page_id_");
  }
  ReadPageGuard header_readguard = bpm_->FetchPageRead(this->header_page_id_);
  auto *header_page = header_readguard.As<BPlusTreeHeaderPage>();
  return header_page->root_page_id_ == INVALID_PAGE_ID;
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::IsEmpty(Context &ctx) const -> bool { return ctx.root_page_id_ == INVALID_PAGE_ID; }

// ------my-------
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::UpdateRootPageId(Context &ctx, page_id_t new_root_id) -> void {
  if (!ctx.header_page_.has_value()) {
    throw Exception("UpdateRootPageId(): ctx的header_page_为空");
  }
  ctx.root_page_id_ = new_root_id;
  auto header_page = ctx.header_page_.value().AsMut<BPlusTreeHeaderPage>();
  header_page->root_page_id_ = new_root_id;
}

/*****************************************************************************
 * SEARCH
 *****************************************************************************/
/*
 * my helper function: 从根节点开始，根据key找到对应的叶子节点。
 *
 */

// my
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::FindLeafPage(Context &ctx, const KeyType &key, int opFlag,
                                  std::unordered_map<page_id_t, int> *page_id_to_index) -> page_id_t {
  if (IsEmpty(ctx)) {
    return INVALID_PAGE_ID;
  }
  // OpFind
  if (opFlag == OpFind) {
    page_id_t page_id = ctx.root_page_id_;
    ReadPageGuard pageguard = bpm_->FetchPageRead(page_id);
    auto page = pageguard.As<BPlusTreePage>();
    if(ctx.header_page_.has_value()){
      ctx.header_page_.value().Drop();
      ctx.header_page_ = std::nullopt;
    }
    while (!page->IsLeafPage()) {
      auto internal_page = reinterpret_cast<const InternalPage *>(page);
      auto next_page_id = internal_page->LookUpV(key, comparator_);
      page_id = next_page_id;
      pageguard = bpm_->FetchPageRead(next_page_id);
      page = pageguard.As<BPlusTreePage>();
    }
    return page_id;
  }
  // OpInsert
  if (opFlag == OpInsert) {
    page_id_t page_id = ctx.root_page_id_;
    WritePageGuard pageguard = bpm_->FetchPageWrite(page_id);
    auto page = pageguard.As<BPlusTreePage>();
    ctx.write_set_.push_back(std::move(pageguard));
    while (!page->IsLeafPage()) {
      auto internal_page = reinterpret_cast<const InternalPage *>(page);
      auto next_page_id = internal_page->LookUpV(key, comparator_);
      page_id = next_page_id;
      pageguard = bpm_->FetchPageWrite(next_page_id);
      page = pageguard.As<BPlusTreePage>();
      if (page->IsSafe(opFlag)) {
        if (ctx.header_page_.has_value()) {
          ctx.header_page_.value().Drop();
          ctx.header_page_ = std::nullopt;
        }
        ctx.write_set_.clear();
      }
      ctx.write_set_.push_back(std::move(pageguard));
    }
    return page_id;
  }
  // OpLeftMost
  if (opFlag == OpLeftMost) {
    page_id_t page_id = ctx.root_page_id_;
    ReadPageGuard pageguard = bpm_->FetchPageRead(page_id);
    auto page = pageguard.As<BPlusTreePage>();
    if(ctx.header_page_.has_value()){
      ctx.header_page_.value().Drop();
      ctx.header_page_ = std::nullopt;
    }
    while (!page->IsLeafPage()) {
      auto internal_page = reinterpret_cast<const InternalPage *>(page);
      auto next_page_id = internal_page->ValueAt(0);  // here
      page_id = next_page_id;
      pageguard = bpm_->FetchPageRead(next_page_id);
      page = pageguard.As<BPlusTreePage>();
    }
    return page_id;
  }
  // OpDelete
  if (opFlag == OpDelete) {
    page_id_t page_id = ctx.root_page_id_;
    WritePageGuard pageguard = bpm_->FetchPageWrite(page_id);
    auto page = pageguard.As<BPlusTreePage>();
    ctx.write_set_.push_back(std::move(pageguard));
    while (!page->IsLeafPage()) {
      auto internal_page = reinterpret_cast<const InternalPage *>(page);
      auto index = internal_page->LookUpEqualLessIndex(key, comparator_);
      if (index == -1) {
        return INVALID_PAGE_ID;
      }
      auto next_page_id = internal_page->ValueAt(index);
      page_id = next_page_id;
      pageguard = bpm_->FetchPageWrite(next_page_id);
      page = pageguard.As<BPlusTreePage>();
      if (page->IsSafe(opFlag)) {
        if (ctx.header_page_.has_value()) {
          ctx.header_page_.value().Drop();
          ctx.header_page_ = std::nullopt;
        }
        ctx.write_set_.clear();
      }
      ctx.write_set_.push_back(std::move(pageguard));
      (*page_id_to_index)[page_id] = index;  // For remove
    }
    return page_id;
  }
  throw Exception("BPLUSTREE_TYPE::FindLeafPage(): 传入了错误的OpFlag");
}

// 仅仅为了符合2022 test的接口。。。
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::FindLeafPage(const KeyType &key, bool leftMost) -> B_PLUS_TREE_LEAF_PAGE_TYPE * {
  int flag = (leftMost) ? OpType::OpLeftMost : OpType::OpFind;
  Context ctx;
  ctx.root_page_id_ = this->GetRootPageId();
  ctx.header_page_ = bpm_->FetchPageWrite(header_page_id_);
  auto leaf_page_id = FindLeafPage(ctx, key, flag, nullptr);
  auto *leaf_page_ptr = bpm_->FetchPage(leaf_page_id);
  return reinterpret_cast<B_PLUS_TREE_LEAF_PAGE_TYPE *>(leaf_page_ptr->GetData());
}

/*
 * Return the only value that associated with input key
 * This method is used for point query
 * @return : true means key exists
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetValue(const KeyType &key, std::vector<ValueType> *result, Transaction *txn) -> bool {
  // empty
  if (IsEmpty()) {
    return false;
  }
  // 0. ctx初始化
  Context ctx;
  ctx.root_page_id_ = this->GetRootPageId();
  ctx.header_page_ = bpm_->FetchPageWrite(header_page_id_);
  // 1. 找到key对应的叶子节点
  page_id_t leaf_page_id = FindLeafPage(ctx, key, OpType::OpFind, nullptr);
  if (leaf_page_id == INVALID_PAGE_ID) {
    return false;
  }
  auto leaf_page = bpm_->FetchPageRead(leaf_page_id);
  auto leaf = leaf_page.As<LeafPage>();
  // 2. 从叶子节点内部查找对应的kv
  auto exist = leaf->LookUpIfExist(key, comparator_);
  if (!exist) {
    return false;
  }
  auto ret = leaf->LookUpV(key, comparator_);
  // 3. 填充结果
  result->push_back(ret);
  return true;
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/

// 目前树为空，创建新的BPlusTree,并插入kv
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::CreateNewTree(Context &ctx, const KeyType &key, const ValueType &value) -> void {
  if (!IsEmpty(ctx)) {
    return;
  }
  // 获取新的页面作为root且为叶子节点
  page_id_t new_page_id;
  bpm_->NewPageGuarded(&new_page_id);
  WritePageGuard new_writeguard = bpm_->FetchPageWrite(new_page_id);
  auto new_root = new_writeguard.AsMut<LeafPage>();
  // 新页面初始化
  new_root->Init(leaf_max_size_);
  // 更新ctx以及b+tree的root_page_id
  UpdateRootPageId(ctx, new_page_id);
  // 向root插入kv
  new_root->InsertKV(key, value, comparator_);
}

// 针对内部节点，将old_page的kv划分给new_page，需要小心new_page第一个kv
// 其只有在将key递归传给上层时有用，其余时候都是无效
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Split(InternalPage *old_page) -> page_id_t {
  BUSTUB_ASSERT(old_page->GetSize() >= 2, "BPLUSTREE_TYPE::Split(): kv太少...");
  page_id_t new_page_id;
  bpm_->NewPageGuarded(&new_page_id);
  auto new_writeguard = bpm_->FetchPageWrite(new_page_id);
  auto new_page = new_writeguard.AsMut<InternalPage>();
  new_page->Init(internal_max_size_);
  old_page->SplitTo(new_page);  // wait!
  return new_page_id;
}

// 针对叶子节点，将old_page的kv划分给新的page，需要修改next_page_id
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Split(LeafPage *old_page) -> page_id_t {
  page_id_t new_page_id;
  bpm_->NewPageGuarded(&new_page_id);
  auto new_writeguard = bpm_->FetchPageWrite(new_page_id);
  auto new_page = new_writeguard.AsMut<LeafPage>();
  new_page->Init(leaf_max_size_);
  old_page->SplitTo(new_page, new_page_id);
  return new_page_id;
}

// 向父节点插入kv
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::InsertIntoParent(Context &ctx, WritePageGuard &&old_guard, const KeyType &key,
                                      WritePageGuard &&new_guard) -> void {
  // auto old_page = old_guard.AsMut<BPlusTreePage>();
  // auto new_page = new_guard.AsMut<BPlusTreePage>();
  // 根节点分裂了, 需要创建并设置新的root
  if (ctx.IsRootPage(old_guard.PageId())) {
    page_id_t new_root_id;
    bpm_->NewPageGuarded(&new_root_id);
    WritePageGuard new_root_writeguard = bpm_->FetchPageWrite(new_root_id);
    auto new_root_page = new_root_writeguard.AsMut<InternalPage>();
    // 新页面要初始化
    new_root_page->Init(internal_max_size_);
    // 更新父页面root_page
    new_root_page->InsertAsRoot(old_guard.PageId(), key, new_guard.PageId());
    // 更新b+树的根节点id
    UpdateRootPageId(ctx, new_root_id);
    return;
  }
  // 不是根节点分裂,从ctx获取并更新父节点
  WritePageGuard parent_pageguard = std::move(ctx.write_set_.back());
  ctx.write_set_.pop_back();
  auto parent_page = parent_pageguard.AsMut<InternalPage>();
  // 父节点插入是否安全
  if(parent_page->IsSafe(OpInsert)){
    parent_page->InsertKVAfter(key, new_guard.PageId(), comparator_); 
    return;
  }
  // 不安全: 先插入在分裂
  parent_page->InsertKVAfter(key, new_guard.PageId(), comparator_);  // here
  page_id_t new_parent_page_id = Split(parent_page);
  WritePageGuard new_parent_pageguard = bpm_->FetchPageWrite(new_parent_page_id);
  auto new_parent_page = new_parent_pageguard.AsMut<InternalPage>();
  // 内部节点的第一个key只有在这时有效~
  InsertIntoParent(ctx, std::move(parent_pageguard), new_parent_page->KeyAt(0), std::move(new_parent_pageguard));
}

// 树不为空， 寻找叶子节点并插入kv，递归地插入父节点
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::InsertIntoLeaf(Context &ctx, const KeyType &key, const ValueType &val) -> bool {
  // 先找到leaf page
  // 没有对应的leaf
  page_id_t leaf_page_id = FindLeafPage(ctx, key, OpType::OpInsert, nullptr);
  if (leaf_page_id == INVALID_PAGE_ID) {
    return false;
  }

  WritePageGuard leaf_write = std::move(ctx.write_set_.back());
  ctx.write_set_.pop_back();
  auto *leaf_page = leaf_write.AsMut<LeafPage>();
  // 叶子节点很安全
  if(leaf_page->IsSafe(OpInsert)){
    bool success = leaf_page->InsertKV(key, val, comparator_);
    return success;
  }
  // 不安全: 先插入,再分裂...
  BUSTUB_ASSERT(leaf_page->GetSize() < leaf_page->GetMaxSize(), "InsertIntoLeaf(): 叶子节点已满，无法插入");
  // 插入同时判断key是否已经存在
  bool success = leaf_page->InsertKV(key, val, comparator_);
  if (!success) {
    return false;
  }
  // 分裂
  page_id_t new_page_id = Split(leaf_page);
  WritePageGuard new_write = bpm_->FetchPageWrite(new_page_id);
  auto new_page = new_write.AsMut<LeafPage>();
  InsertIntoParent(ctx, std::move(leaf_write), new_page->KeyAt(0), std::move(new_write));
  return true;
}

/*
 * Insert constant key & value pair into b+ tree
 * if current tree is empty, start new tree, update root page id and insert
 * entry, otherwise insert into leaf page.
 * @return: since we only support unique key, if user try to insert duplicate
 * keys return false, otherwise return true.
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Insert(const KeyType &key, const ValueType &value, Transaction *txn) -> bool {
  Context ctx;
  ctx.root_page_id_ = this->GetRootPageId();
  ctx.header_page_ = bpm_->FetchPageWrite(header_page_id_);
  // 空
  if (IsEmpty(ctx)) {
    CreateNewTree(ctx, key, value);
    return true;
  }
  // 不空
  // insert
  return InsertIntoLeaf(ctx, key, value);
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * Delete key & value pair associated with input key
 * If current tree is empty, return immediately.
 * If not, User needs to first find the right leaf page as deletion target, then
 * delete entry from leaf page. Remember to deal with redistribute or merge if
 * necessary.
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Remove(const KeyType &key, Transaction *txn) {
  // ctx初始化
  Context ctx;
  ctx.root_page_id_ = this->GetRootPageId();
  ctx.header_page_ = bpm_->FetchPageWrite(header_page_id_);

  // 寻找叶子节点。没找到(空树)
  std::unordered_map<page_id_t, int> page_id_to_index;
  page_id_t leaf_page_id = FindLeafPage(ctx, key, OpType::OpDelete, &page_id_to_index);
  if (leaf_page_id == INVALID_PAGE_ID) {
    return;
  }
  // 找到了
  RemoveLeafEntry(ctx, key, page_id_to_index);
}

// 删除叶子节点的kv
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::RemoveLeafEntry(Context &ctx, const KeyType &key,
                                     std::unordered_map<page_id_t, int> &page_id_to_index) -> void {
  // 获取叶子节点
  WritePageGuard cur_leaf_writeguard = std::move(ctx.write_set_.back());
  ctx.write_set_.pop_back();

  page_id_t cur_leaf_id = cur_leaf_writeguard.PageId();
  auto *cur_leaf_page = cur_leaf_writeguard.AsMut<LeafPage>();

  // 从叶子节点删除指定的kv
  bool success = cur_leaf_page->DeleteKV(key, comparator_);  // wait
  if (!success) {                                            // 删除失败(节点内没有指定的key)
    return;
  }

  // 叶子节点是根节点并且现在是空的: 更新root_page_id为无效
  if (cur_leaf_id == ctx.root_page_id_ && cur_leaf_page->GetSize() == 0) {
    if (!ctx.header_page_.has_value()) {
      throw Exception("ctx.head_page_ is nullopt");
    }
    WritePageGuard &header_writeguard = ctx.header_page_.value();
    auto *header_page = header_writeguard.AsMut<BPlusTreeHeaderPage>();
    header_page->root_page_id_ = INVALID_PAGE_ID;
    ctx.Clear();
    return;
  }

  // 叶子节点是根节点并且不为空: safe
  if (cur_leaf_id == ctx.root_page_id_) {
    ctx.Clear();
    return;
  }

  /* 叶子节点不是根节点 */
  // 叶子节点的数目足够，直接返回
  if (cur_leaf_page->GetSize() >= cur_leaf_page->GetMinSize()) {
    ctx.Clear();
    return;
  }

  // 获取父节点以及key在父节点中的index
  WritePageGuard &parent_writeguard = ctx.write_set_.back();
  auto *parent_page = parent_writeguard.AsMut<InternalPage>();
  int key_index_in_parent = page_id_to_index[cur_leaf_id];

  // 获取兄弟节点
  page_id_t sibling_page_id = -1;
  int size = parent_page->GetSize();
  bool is_last_kv = (key_index_in_parent == size - 1);
  /*
  if(is_last_kv && key_index_in_parent -1 <= 0){
    std::cout << "here";
  }
  */
  if (is_last_kv) {  // 叶子节点是父节点最后一个kv指向的节点，要找左兄弟
    sibling_page_id = parent_page->ValueAt(key_index_in_parent - 1);
  } else {  // 叶子节点不是父节点最后指向的节点，默认找右兄弟
    sibling_page_id = parent_page->ValueAt(key_index_in_parent + 1);
  }

  // 一致化处理逻辑
  LeafPage *left_page = nullptr;
  LeafPage *right_page = nullptr;
  KeyType up_key;
  bool redistribute_to_right = true;
  if (is_last_kv) {
    cur_leaf_writeguard.Drop(); // 先Drop, 确保我们先获取左边节点的锁，在获取右边。(和叶子节点遍历的顺序一致，避免死锁)
    WritePageGuard sibling_writeguard = bpm_->FetchPageWrite(sibling_page_id);
    left_page = sibling_writeguard.AsMut<LeafPage>();
    cur_leaf_writeguard = bpm_->FetchPageWrite(cur_leaf_id);
    right_page = cur_leaf_writeguard.AsMut<LeafPage>();
    up_key = parent_page->KeyAt(key_index_in_parent);
  } else {
    left_page = cur_leaf_page;
    WritePageGuard sibling_writeguard = bpm_->FetchPageWrite(sibling_page_id);
    right_page = sibling_writeguard.AsMut<LeafPage>();
    up_key = parent_page->KeyAt(key_index_in_parent + 1);
    redistribute_to_right = false;
  }

  // 是否合并
  int left_size = left_page->GetSize();
  int right_size = right_page->GetSize();
  if (left_size + right_size < left_page->GetMaxSize()) {
    left_page->Merge(right_page);  // wait
    left_page->SetNextPageId(right_page->GetNextPageId());
    // 需要从bpm_中删除右页面么? wait
    RemoveInternalEntry(ctx, up_key, page_id_to_index);  // wait
    return;
  }

  // 重新分配一个kv
  if (redistribute_to_right) {  // left_page ===> right_page
    right_page->ShiftData(1);
    right_page->SetKeyAt(0, left_page->KeyAt(left_size - 1));
    right_page->SetValAt(0, left_page->ValAt(left_size - 1));
    left_page->IncreaseSize(-1);
    parent_page->SetKeyAt(key_index_in_parent, right_page->KeyAt(0));
  } else {  // left_page <=== right_page
    left_page->SetKeyAt(left_size, right_page->KeyAt(0));
    left_page->SetValAt(left_size, right_page->ValAt(0));
    left_page->IncreaseSize(1);
    right_page->ShiftData(-1);
    parent_page->SetKeyAt(key_index_in_parent + 1, right_page->KeyAt(0));
  }
}

// 递归删除内部节点的kv
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::RemoveInternalEntry(Context &ctx, const KeyType &key,
                                         std::unordered_map<page_id_t, int> &page_id_to_index) -> void {
  // 获取内部节点
  WritePageGuard cur_internal_writeguard = std::move(ctx.write_set_.back());
  ctx.write_set_.pop_back();

  page_id_t cur_internal_id = cur_internal_writeguard.PageId();
  auto *cur_internal_page = cur_internal_writeguard.AsMut<InternalPage>();

  // 从叶子节点删除指定的kv
  bool success = cur_internal_page->DeleteKV(key, comparator_);
  if (!success) {  // 删除失败(节点内没有指定的key)
    return;
  }

  // 内部节点节点是根节点并且现在只有一个无效kv: 更新root_page_id为无效
  if (cur_internal_id == ctx.root_page_id_ && cur_internal_page->GetSize() == 1) {
    if (!ctx.header_page_.has_value()) {
      throw Exception("ctx.head_page_ is nullopt");
    }
    WritePageGuard &header_writeguard = ctx.header_page_.value();
    auto *header_page = header_writeguard.AsMut<BPlusTreeHeaderPage>();
    header_page->root_page_id_ = cur_internal_page->ValueAt(0);
    ctx.root_page_id_ = header_page->root_page_id_;
    ctx.Clear();
    return;
  }

  // 内部节点是根节点并且不为空: safe
  if (cur_internal_id == ctx.root_page_id_) {
    ctx.Clear();
    return;
  }

  /* 内部节点不是根节点 */
  // 内部节点的kv数足够，直接返回
  if (cur_internal_page->GetSize() >= cur_internal_page->GetMinSize()) {
    return;
  }

  // 获取父节点以及key在父节点中的index
  int key_index_in_parent = page_id_to_index[cur_internal_id];
  WritePageGuard &parent_writeguard = ctx.write_set_.back();
  auto *parent_page = parent_writeguard.AsMut<InternalPage>();

  // 获取兄弟节点
  page_id_t sibling_page_id = -1;
  bool is_last_kv = (key_index_in_parent == parent_page->GetSize() - 1);
  if (is_last_kv) {  // 叶子节点是父节点最后一个kv指向的节点，要找左兄弟
    sibling_page_id = parent_page->ValueAt(key_index_in_parent - 1);
  } else {  // 叶子节点不是父节点最后指向的节点，默认找右兄弟
    sibling_page_id = parent_page->ValueAt(key_index_in_parent + 1);
  }
  WritePageGuard sibling_writeguard = bpm_->FetchPageWrite(sibling_page_id);

  // 一致化处理逻辑
  InternalPage *left_page = nullptr;
  InternalPage *right_page = nullptr;
  KeyType up_key;
  bool redistribute_to_right = true;
  if (is_last_kv) {
    left_page = sibling_writeguard.AsMut<InternalPage>();
    right_page = cur_internal_page;
    up_key = parent_page->KeyAt(key_index_in_parent);
  } else {
    left_page = cur_internal_page;
    right_page = sibling_writeguard.AsMut<InternalPage>();
    up_key = parent_page->KeyAt(key_index_in_parent + 1);
    redistribute_to_right = false;
  }

  // 合并right_page ===> left_page
  int left_size = left_page->GetSize();
  int right_size = right_page->GetSize();
  if (left_size + right_size < left_page->GetMaxSize()) {  // wait:
    right_page->SetKeyAt(0, up_key);
    left_page->Merge(right_page);  // wait
    // 需要从bpm_中删除右页面么? wait
    RemoveInternalEntry(ctx, up_key, page_id_to_index);  // wait
    return;
  }

  // 重新分配一个kv(第一个kv此时也可视为有效~)
  if (redistribute_to_right) {  // left_page ===> right_page
    right_page->ShiftData(1);
    right_page->SetKeyAt(0, left_page->KeyAt(left_size - 1));
    right_page->SetValAt(0, left_page->ValueAt(left_size - 1));
    left_page->IncreaseSize(-1);
    parent_page->SetKeyAt(key_index_in_parent, right_page->KeyAt(0));
  } else {  // left_page <=== right_page
    left_page->SetKeyAt(left_size, right_page->KeyAt(0));
    left_page->SetValAt(left_size, right_page->ValueAt(0));
    left_page->IncreaseSize(1);
    right_page->ShiftData(-1);
    parent_page->SetKeyAt(key_index_in_parent + 1, right_page->KeyAt(0));
  }
}

/*****************************************************************************
 * INDEX ITERATOR
 *****************************************************************************/
/*
 * Input parameter is void, find the leftmost leaf page first, then construct
 * index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin() -> INDEXITERATOR_TYPE {
  // 不必要的ctx...
  Context ctx;
  ctx.root_page_id_ = this->GetRootPageId();
  ctx.header_page_ = bpm_->FetchPageWrite(header_page_id_);
  page_id_t left_leaf_id = FindLeafPage(ctx, KeyType(), OpLeftMost, nullptr);
  return INDEXITERATOR_TYPE(bpm_, left_leaf_id, 0);
}

/*
 * Input parameter is low key, find the leaf page that contains the input key
 * first, then construct index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS auto BPLUSTREE_TYPE::Begin(const KeyType &key) -> INDEXITERATOR_TYPE {
  Context ctx;
  ctx.root_page_id_ = this->GetRootPageId();
  ctx.header_page_ = bpm_->FetchPageWrite(header_page_id_);
  page_id_t leaf_id = FindLeafPage(ctx, key, OpFind, nullptr);
  ReadPageGuard leaf_readguard = bpm_->FetchPageRead(leaf_id);
  auto *leaf = leaf_readguard.As<B_PLUS_TREE_LEAF_PAGE_TYPE>();
  int index = leaf->LookUpIndex(key, comparator_);
  return INDEXITERATOR_TYPE(bpm_, leaf_id, index);
}

/*
 * Input parameter is void, construct an index iterator representing the end
 * of the key/value pair in the leaf node
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::End() -> INDEXITERATOR_TYPE {
  // 构造一个尾迭代器
  return INDEXITERATOR_TYPE(bpm_, INVALID_PAGE_ID, -1);
}

/**
 * @return Page id of the root of this tree
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetRootPageId() -> page_id_t {
  if (this->header_page_id_ == INVALID_PAGE_ID) {
    return INVALID_PAGE_ID;
  }
  ReadPageGuard header_readguard = bpm_->FetchPageRead(this->header_page_id_);
  auto header_page = header_readguard.As<BPlusTreeHeaderPage>();
  return header_page->root_page_id_;
}

/*****************************************************************************
 * UTILITIES AND DEBUG
 *****************************************************************************/

/*
 * This method is used for test only
 * Read data from file and insert one by one
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::InsertFromFile(const std::string &file_name, Transaction *txn) {
  int64_t key;
  std::ifstream input(file_name);
  while (input) {
    input >> key;

    KeyType index_key;
    index_key.SetFromInteger(key);
    RID rid(key);
    Insert(index_key, rid, txn);
  }
}
/*
 * This method is used for test only
 * Read data from file and remove one by one
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::RemoveFromFile(const std::string &file_name, Transaction *txn) {
  int64_t key;
  std::ifstream input(file_name);
  while (input) {
    input >> key;
    KeyType index_key;
    index_key.SetFromInteger(key);
    Remove(index_key, txn);
  }
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Print(BufferPoolManager *bpm) {
  auto root_page_id = GetRootPageId();
  auto guard = bpm->FetchPageBasic(root_page_id);
  PrintTree(guard.PageId(), guard.template As<BPlusTreePage>());
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::PrintTree(page_id_t page_id, const BPlusTreePage *page) {
  if (page->IsLeafPage()) {
    auto *leaf = reinterpret_cast<const LeafPage *>(page);
    std::cout << "Leaf Page: " << page_id << "\tNext: " << leaf->GetNextPageId() << std::endl;

    // Print the contents of the leaf page.
    std::cout << "Contents: ";
    for (int i = 0; i < leaf->GetSize(); i++) {
      std::cout << leaf->KeyAt(i) << ", " << leaf->ValAt(i);  // my add
      if ((i + 1) < leaf->GetSize()) {
        std::cout << ", ";
      }
    }
    std::cout << '\n';
    std::cout << '\n';

  } else {
    auto *internal = reinterpret_cast<const InternalPage *>(page);
    std::cout << "Internal Page: " << page_id << '\n';

    // Print the contents of the internal page.
    std::cout << "Contents: ";
    for (int i = 0; i < internal->GetSize(); i++) {
      std::cout << internal->KeyAt(i) << ": " << internal->ValueAt(i);
      if ((i + 1) < internal->GetSize()) {
        std::cout << ", ";
      }
    }
    std::cout << '\n';
    std::cout << '\n';
    for (int i = 0; i < internal->GetSize(); i++) {
      auto guard = bpm_->FetchPageBasic(internal->ValueAt(i));
      PrintTree(guard.PageId(), guard.template As<BPlusTreePage>());
    }
  }
}

/**
 * This method is used for debug only, You don't need to modify
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Draw(BufferPoolManager *bpm, const std::string &outf) {
  if (IsEmpty()) {
    LOG_WARN("Drawing an empty tree");
    return;
  }

  std::ofstream out(outf);
  out << "digraph G {" << '\n';
  auto root_page_id = GetRootPageId();
  auto guard = bpm->FetchPageBasic(root_page_id);
  ToGraph(guard.PageId(), guard.template As<BPlusTreePage>(), out);
  out << "}" << '\n';
  out.close();
}

/**
 * This method is used for debug only, You don't need to modify
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::ToGraph(page_id_t page_id, const BPlusTreePage *page, std::ofstream &out) {
  std::string leaf_prefix("LEAF_");
  std::string internal_prefix("INT_");
  if (page->IsLeafPage()) {
    auto *leaf = reinterpret_cast<const LeafPage *>(page);
    // Print node name
    out << leaf_prefix << page_id;
    // Print node properties
    out << "[shape=plain color=green ";
    // Print data of the node
    out << "label=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
    // Print data
    out << "<TR><TD COLSPAN=\"" << leaf->GetSize() << "\">P=" << page_id << "</TD></TR>\n";
    out << "<TR><TD COLSPAN=\"" << leaf->GetSize() << "\">"
        << "max_size=" << leaf->GetMaxSize() << ",min_size=" << leaf->GetMinSize() << ",size=" << leaf->GetSize()
        << "</TD></TR>\n";
    out << "<TR>";
    for (int i = 0; i < leaf->GetSize(); i++) {
      out << "<TD>" << leaf->KeyAt(i) << "</TD>\n";
    }
    out << "</TR>";
    // Print table end
    out << "</TABLE>>];\n";
    // Print Leaf node link if there is a next page
    if (leaf->GetNextPageId() != INVALID_PAGE_ID) {
      out << leaf_prefix << page_id << " -> " << leaf_prefix << leaf->GetNextPageId() << ";\n";
      out << "{rank=same " << leaf_prefix << page_id << " " << leaf_prefix << leaf->GetNextPageId() << "};\n";
    }
  } else {
    auto *inner = reinterpret_cast<const InternalPage *>(page);
    // Print node name
    out << internal_prefix << page_id;
    // Print node properties
    out << "[shape=plain color=pink ";  // why not?
    // Print data of the node
    out << "label=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
    // Print data
    out << "<TR><TD COLSPAN=\"" << inner->GetSize() << "\">P=" << page_id << "</TD></TR>\n";
    out << "<TR><TD COLSPAN=\"" << inner->GetSize() << "\">"
        << "max_size=" << inner->GetMaxSize() << ",min_size=" << inner->GetMinSize() << ",size=" << inner->GetSize()
        << "</TD></TR>\n";
    out << "<TR>";
    for (int i = 0; i < inner->GetSize(); i++) {
      out << "<TD PORT=\"p" << inner->ValueAt(i) << "\">";
      if (i > 0) {
        out << inner->KeyAt(i);
      } else {
        out << " ";
      }
      out << "</TD>\n";
    }
    out << "</TR>";
    // Print table end
    out << "</TABLE>>];\n";
    // Print leaves
    for (int i = 0; i < inner->GetSize(); i++) {
      auto child_guard = bpm_->FetchPageBasic(inner->ValueAt(i));
      auto child_page = child_guard.template As<BPlusTreePage>();
      ToGraph(child_guard.PageId(), child_page, out);
      if (i > 0) {
        auto sibling_guard = bpm_->FetchPageBasic(inner->ValueAt(i - 1));
        auto sibling_page = sibling_guard.template As<BPlusTreePage>();
        if (!sibling_page->IsLeafPage() && !child_page->IsLeafPage()) {
          out << "{rank=same " << internal_prefix << sibling_guard.PageId() << " " << internal_prefix
              << child_guard.PageId() << "};\n";
        }
      }
      out << internal_prefix << page_id << ":p" << child_guard.PageId() << " -> ";
      if (child_page->IsLeafPage()) {
        out << leaf_prefix << child_guard.PageId() << ";\n";
      } else {
        out << internal_prefix << child_guard.PageId() << ";\n";
      }
    }
  }
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::DrawBPlusTree() -> std::string {
  if (IsEmpty()) {
    return "()";
  }

  PrintableBPlusTree p_root = ToPrintableBPlusTree(GetRootPageId());
  std::ostringstream out_buf;
  p_root.Print(out_buf);

  return out_buf.str();
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::ToPrintableBPlusTree(page_id_t root_id) -> PrintableBPlusTree {
  auto root_page_guard = bpm_->FetchPageBasic(root_id);
  auto root_page = root_page_guard.template As<BPlusTreePage>();
  PrintableBPlusTree proot;

  if (root_page->IsLeafPage()) {
    auto leaf_page = root_page_guard.template As<LeafPage>();
    proot.keys_ = leaf_page->ToString();
    proot.size_ = proot.keys_.size() + 4;  // 4 more spaces for indent

    return proot;
  }

  // draw internal page
  auto internal_page = root_page_guard.template As<InternalPage>();
  proot.keys_ = internal_page->ToString();
  proot.size_ = 0;
  for (int i = 0; i < internal_page->GetSize(); i++) {
    page_id_t child_id = internal_page->ValueAt(i);
    PrintableBPlusTree child_node = ToPrintableBPlusTree(child_id);
    proot.size_ += child_node.size_;
    proot.children_.push_back(child_node);
  }

  return proot;
}

template class BPlusTree<GenericKey<4>, RID, GenericComparator<4>>;

template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>>;

template class BPlusTree<GenericKey<16>, RID, GenericComparator<16>>;

template class BPlusTree<GenericKey<32>, RID, GenericComparator<32>>;

template class BPlusTree<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
