//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager.h"
#include <cstddef>

#include "common/config.h"
#include "common/exception.h"
#include "common/macros.h"
#include "storage/page/page.h"
#include "storage/page/page_guard.h"
// make buffer_pool_manager_test -j8
// ./test/buffer_pool_manager_test
namespace bustub {
// make buffer_pool_manager_test -j8
// ./test/buffer_pool_manager_test
BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, size_t replacer_k,
                                     LogManager *log_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager), log_manager_(log_manager) {
  // we allocate a consecutive memory space for the buffer pool
  pages_ = new Page[pool_size_];
  replacer_ = std::make_unique<LRUKReplacer>(pool_size, replacer_k);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManager::~BufferPoolManager() { delete[] pages_; }

auto BufferPoolManager::NewPage(page_id_t *page_id) -> Page * {
  std::lock_guard<decltype(latch_)> lock(latch_);
  int my_frame_id = -1;
  // 尝试选择frame
  if (!free_list_.empty()) {
    my_frame_id = free_list_.front();
    free_list_.pop_front();
  } else {  // freelist中无空闲，从replacer中淘汰
    auto flag = replacer_->Evict(&my_frame_id);
    if (!flag) {  // replacer可淘汰页面(所有页面都被pin)
      return nullptr;
    }
    // 重置旧页框对应的内容
    auto &page = pages_[my_frame_id];
    if (page.IsDirty()) {
      disk_manager_->WritePage(page.GetPageId(), page.GetData());
      page.is_dirty_ = false;
    }
    page_table_.erase(page.page_id_);  // forget...
    page.ResetMemory();
    page.pin_count_ = 0;
    DeallocatePage(page.GetPageId());
    page.page_id_ = INVALID_PAGE_ID;
  }

  // 成功得到某个空页框，设置其元信息
  auto id = AllocatePage();
  *page_id = id;
  auto &page = pages_[my_frame_id];
  page.page_id_ = id;
  page.pin_count_ = 1;
  page.is_dirty_ = false;
  replacer_->RecordAccess(my_frame_id);
  replacer_->SetEvictable(my_frame_id, false);
  page_table_[id] = my_frame_id;
  return pages_ + my_frame_id;
}

auto BufferPoolManager::FetchPage(page_id_t page_id, AccessType access_type) -> Page * {
  std::lock_guard<std::mutex> lock(latch_);
  // nullptr: 页面不在缓冲池需要从disk读取，但是所有的页框都在使用且没有可以淘汰的页框(all pined)
  frame_id_t cur_frame_id = -1;
  if (page_table_.count(page_id) == 0U) {
    if (!free_list_.empty()) {
      cur_frame_id = free_list_.front();
      free_list_.pop_front();
    } else {
      auto flag = replacer_->Evict(&cur_frame_id);
      if (!flag) {
        return nullptr;
      }
      // 逐出了某个页， 重置数据
      auto &page = pages_[cur_frame_id];
      if (page.IsDirty()) {
        disk_manager_->WritePage(page.GetPageId(), page.GetData());
        page.is_dirty_ = false;
      }
      page_table_.erase(page.page_id_);
      page.ResetMemory();
      page.pin_count_ = 0;
      DeallocatePage(page.GetPageId());
      page.page_id_ = INVALID_PAGE_ID;
    }
    // 获得可用空页框
    auto &page = pages_[cur_frame_id];
    page.page_id_ = page_id;
    page.pin_count_ = 1;
    disk_manager_->ReadPage(page_id, page.data_);
    replacer_->RecordAccess(cur_frame_id);
    replacer_->SetEvictable(cur_frame_id, false);
    page_table_[page_id] = cur_frame_id;
    return pages_ + cur_frame_id;
  }
  // 页面在缓冲池中
  cur_frame_id = page_table_[page_id];
  pages_[cur_frame_id].pin_count_++;
  replacer_->RecordAccess(cur_frame_id);
  replacer_->SetEvictable(cur_frame_id, false);
  return pages_ + cur_frame_id;
}

auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty, AccessType access_type) -> bool {
  std::lock_guard<decltype(latch_)> lock(latch_);
  // false: page不在缓冲池/引用计数已经为0
  if (page_table_.count(page_id) == 0U || pages_[page_table_[page_id]].GetPinCount() <= 0) {
    return false;
  }

  auto &page = pages_[page_table_[page_id]];
  page.pin_count_--;
  if (!page.IsDirty() && is_dirty) {
    page.is_dirty_ = is_dirty;
  }
  if (page.GetPinCount() == 0) {
    replacer_->SetEvictable(page_table_[page_id], true);
  }
  return true;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  std::lock_guard<decltype(latch_)> lock(latch_);
  // 检查page_id的有效性
  if (page_id == INVALID_PAGE_ID || page_table_.count(page_id) == 0U) {
    return false;
  }

  // 无论脏位如何，都要flush
  auto &page = pages_[page_table_[page_id]];
  disk_manager_->WritePage(page_id, page.GetData());
  page.is_dirty_ = false;
  return true;
}

void BufferPoolManager::FlushAllPages() {
  for (auto &page : page_table_) {
    FlushPage(page.first);
  }
}

auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool {
  std::lock_guard<decltype(latch_)> lock(latch_);
  // 如果不在缓冲池里，返回true
  if (page_table_.count(page_id) == 0U) {
    return true;
  }

  // 如果被pin(不能被删除), 返回false
  if (pages_[page_table_[page_id]].pin_count_ > 0) {
    return false;
  }

  auto cur_frame_id = page_table_[page_id];
  // 从page_table中清除
  page_table_.erase(page_id);
  // 从LRU-K中删除
  replacer_->Remove(cur_frame_id);
  // 添加回freelist
  free_list_.push_back(cur_frame_id);
  // reset page info
  auto &page = pages_[cur_frame_id];
  page.ResetMemory();
  page.page_id_ = INVALID_PAGE_ID;
  page.is_dirty_ = false;
  page.pin_count_ = 0;

  DeallocatePage(page_id);  // more
  return true;
}

auto BufferPoolManager::AllocatePage() -> page_id_t { return next_page_id_++; }

auto BufferPoolManager::FetchPageBasic(page_id_t page_id) -> BasicPageGuard { return {this, this->FetchPage(page_id)}; }

auto BufferPoolManager::FetchPageRead(page_id_t page_id) -> ReadPageGuard {
  auto page = FetchPage(page_id);
  page->RLatch();
  return {this, page};
}

auto BufferPoolManager::FetchPageWrite(page_id_t page_id) -> WritePageGuard {
  auto page = FetchPage(page_id);
  page->WLatch();
  return {this, page};
}

auto BufferPoolManager::NewPageGuarded(page_id_t *page_id) -> BasicPageGuard { return {this, this->NewPage(page_id)}; }

}  // namespace bustub
