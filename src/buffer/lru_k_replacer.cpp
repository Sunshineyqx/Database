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
// #include "common/logger.h"
#include "common/macros.h"
#include "type/limits.h"

// make lru_k_replacer_test -j8
// ./test/lru_k_replacer_test
namespace bustub {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  std::lock_guard<std::mutex> lock(latch_);
  frame_id_t victim = INT_MAX;
  // 没有可以驱逐的frame
  if (curr_size_ == 0) {
    *frame_id = victim;
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
      return true;
    }
  }
  // node_more_k_
  frame_id_t id = -1;
  size_t k_distance = std::numeric_limits<size_t>::max();
  auto remove_it = node_more_k_.begin();
  for (auto it = node_more_k_.begin(); it != node_more_k_.end(); ++it) {
    auto cur_frame_id = *it;
    auto &node = node_store_[cur_frame_id];
    if (node.is_evictable_) {
      if (node.GetKDistance() < k_distance) {
        id = cur_frame_id;
        remove_it = it;
        k_distance = node.GetKDistance();
      }
    }
  }
  if (id == -1) {
    *frame_id = victim;
    return false;
  }
  *frame_id = id;
  node_more_k_.erase(remove_it);
  node_store_.erase(id);
  --curr_size_;
  return true;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id, AccessType access_type) {
  // 检查frame_id的有效性(
  BUSTUB_ASSERT(frame_id >= 0 && frame_id <= (int)replacer_size_, "Invalid frame id");
  std::lock_guard<std::mutex> lock(latch_);

  if (node_store_.find(frame_id) == node_store_.end()) {
    node_store_[frame_id] = LRUKNode();
    auto &node = node_store_[frame_id];
    node.k_ = k_;
    node.fid_ = frame_id;
    node.history_.push_back(current_timestamp_++);
    node_less_k_.push_back(frame_id);
  } else {
    auto &node = node_store_[frame_id];
    auto pre_size = node.history_.size();
    node.SycModifyHistory(current_timestamp_++);
    if (pre_size + 1 == k_) {  // 从less移入more
      auto it = node_less_k_.begin();
      for (; it != node_less_k_.end(); it++) {
        if (*it == frame_id) {
          break;
        }
      }
      node_less_k_.erase(it);
      node_more_k_.push_back(frame_id);
    }
  }
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  // 检查frame_id的有效性(
  BUSTUB_ASSERT(frame_id >= 0 && frame_id <= (int)replacer_size_, "Invalid frame id");
  std::lock_guard<std::mutex> lock(latch_);

  if (node_store_.find(frame_id) == node_store_.end()) {
    return;
  }
  auto &node = node_store_[frame_id];
  if (!node.is_evictable_ && set_evictable) {
    ++curr_size_;
  } else if (node.is_evictable_ && !set_evictable) {
    --curr_size_;
  }
  node.is_evictable_ = set_evictable;
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  // 检查frame_id的有效性(
  BUSTUB_ASSERT(frame_id >= 0 && frame_id <= (int)replacer_size_, "Invalid frame id");
  std::lock_guard<std::mutex> lock(latch_);

  if (node_store_.count(frame_id) == 0U) {
    return;
  }
  auto &node = node_store_[frame_id];
  if (!node.is_evictable_) {
    throw Exception("not evictable...");
  }
  auto k = node.history_.size();
  node.history_.clear();
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
}

auto LRUKReplacer::Size() -> size_t {
  std::lock_guard<std::mutex> lock(latch_);
  return curr_size_;
}

}  // namespace bustub
