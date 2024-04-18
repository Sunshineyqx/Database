//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"
#include <cstddef>
#include "common/config.h"
#include "common/exception.h"
#include "common/macros.h"
#include "type/limits.h"

namespace bustub {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  latch_.lock();
  if (node_less_k_.empty() && node_more_k_.empty()) {
    latch_.unlock();
    return false;
  }
  // node_less_k_
  for (auto it = node_less_k_.begin(); it != node_less_k_.end(); ++it) {
    auto cur_frame_id = *it;
    if (node_store_[cur_frame_id].is_evictable_) {
      *frame_id = cur_frame_id;
      node_less_k_.erase(it);
      node_store_.erase(cur_frame_id);
      --curr_size_;
      latch_.unlock();
      return true;
    }
  }
  // node_more_k_
  frame_id_t id = -1;
  size_t k_distance = 0;
  auto remove_it = node_more_k_.begin();
  for (auto it = node_more_k_.begin(); it != node_more_k_.end(); ++it) {
    auto cur_frame_id = *it;
    auto &node = node_store_[cur_frame_id];
    if (node.is_evictable_) {
      if (node.GetKDistance() > k_distance) {
        id = cur_frame_id;
        remove_it = it;
        k_distance = node.GetKDistance();
      }
    }
  }
  if (id == -1) {
    latch_.unlock();
    return false;
  }
  *frame_id = id;
  node_more_k_.erase(remove_it);
  node_store_.erase(id);
  --curr_size_;
  latch_.unlock();
  return true;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id, [[maybe_unused]] AccessType access_type) {
  // 检查frame_id的有效性(
  BUSTUB_ASSERT(frame_id >= 0 && frame_id <= (int)replacer_size_, "Invalid frame id");

  latch_.lock();
  if (node_store_.count(frame_id) == 0U) {
    auto node = LRUKNode();
    node.k_ = k_;
    node.fid_ = frame_id;
    node.history_.push_back(current_timestamp_++);
    node_store_[frame_id] = std::move(node);
    node_less_k_.push_back(frame_id);
  } else {
    auto &node = node_store_[frame_id];
    auto pre_size = node.history_.size();
    node.SycModifyHistory(current_timestamp_++);
    if (pre_size + 1 == k_) {  // 从less移入more
      for (auto it = node_less_k_.begin(); it != node_less_k_.end(); it++) {
        if (*it == frame_id) {
          node_less_k_.erase(it);
          break;
        }
      }
      node_more_k_.push_back(frame_id);
    }
  }
  latch_.unlock();
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  latch_.lock();
  auto &node = node_store_[frame_id];
  if (!node.is_evictable_ && set_evictable) {
    ++curr_size_;
  } else if (node.is_evictable_ && !set_evictable) {
    --curr_size_;
  }
  node.is_evictable_ = set_evictable;
  latch_.unlock();
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  latch_.lock();
  if (node_store_.count(frame_id) == 0U) {
    return;
  }
  auto node = node_store_[frame_id];
  if (!node.is_evictable_) {
    latch_.unlock();
    throw Exception("not evictable...");
  }
  auto k = node.k_;
  node_store_.erase(frame_id);
  if (k == k_) {
    for (auto it = node_more_k_.begin(); it != node_more_k_.end(); it++) {
      if (*it == frame_id) {
        node_more_k_.erase(it);
        break;
      }
    }
  } else {
    for (auto it = node_less_k_.begin(); it != node_less_k_.end(); it++) {
      if (*it == frame_id) {
        node_less_k_.erase(it);
        break;
      }
    }
  }
  curr_size_--;
  latch_.unlock();
}

auto LRUKReplacer::Size() -> size_t {
  latch_.lock();
  auto size = curr_size_;
  latch_.unlock();
  return size;
}

}  // namespace bustub
