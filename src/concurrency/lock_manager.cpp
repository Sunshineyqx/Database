//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lock_manager.cpp
//
// Identification: src/concurrency/lock_manager.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "concurrency/lock_manager.h"
#include <memory>
#include <mutex>

#include "common/config.h"
#include "common/macros.h"
#include "concurrency/transaction.h"
#include "concurrency/transaction_manager.h"

namespace bustub {

auto LockManager::LockTable(Transaction *txn, LockMode lock_mode, const table_oid_t &oid) -> bool {
  // 检查事务的状态是否合理
  BUSTUB_ASSERT(txn != nullptr, "UnlockTable: txn is nullptr");
  BUSTUB_ASSERT(txn->GetState() != TransactionState::ABORTED, "UnlockTable: txn is aborted");
  BUSTUB_ASSERT(txn->GetState() != TransactionState::COMMITTED, "UnlockTable: txn is committed");

  // 检查lock_mode与txn的隔离级别、状态的兼容性
  switch (txn->GetIsolationLevel()) {
    case bustub::IsolationLevel::REPEATABLE_READ: {
      if (txn->GetState() == bustub::TransactionState::SHRINKING) {
        ThrowAbort(txn, AbortReason::LOCK_ON_SHRINKING);
      }
      break;
    }
    case bustub::IsolationLevel::READ_COMMITTED: {
      if (txn->GetState() == bustub::TransactionState::SHRINKING && lock_mode != LockMode::SHARED &&
          lock_mode != LockMode::INTENTION_SHARED) {
        ThrowAbort(txn, AbortReason::LOCK_ON_SHRINKING);
      }
    } break;
    case bustub::IsolationLevel::READ_UNCOMMITTED: {
      if (lock_mode != LockMode::EXCLUSIVE && lock_mode != LockMode::INTENTION_EXCLUSIVE) {
        ThrowAbort(txn, AbortReason::LOCK_SHARED_ON_READ_UNCOMMITTED);
      }
      if (txn->GetState() != TransactionState::GROWING) {
        ThrowAbort(txn, AbortReason::LOCK_ON_SHRINKING);
      }
      break;
    }
    default:
      break;
  }

  // get请求队列
  table_lock_map_latch_.lock();
  if (table_lock_map_.find(oid) == table_lock_map_.end()) {
    table_lock_map_[oid] = std::make_shared<LockRequestQueue>();
  }
  auto &request_queue = table_lock_map_[oid];
  std::unique_lock<std::mutex> que_lock(request_queue->latch_);
  table_lock_map_latch_.unlock();

  std::shared_ptr<LockRequest> new_request = nullptr;
  // 检查是否事务已有相应的锁 / 可升级的锁
  for (auto it = request_queue->request_queue_.begin(); it != request_queue->request_queue_.end(); ++it) {
    auto cur_request = *it;
    if (cur_request->txn_id_ == txn->GetTransactionId()) {
      if (cur_request->lock_mode_ == lock_mode) {  // 该事务有相同的锁，返回true
        return true;
      }
      /** 有锁，但不同...可能需要升级...但当前队列只能有一个升级的请求 */
      if (request_queue->upgrading_ != INVALID_TXN_ID) {
        ThrowAbort(txn, AbortReason::UPGRADE_CONFLICT);
      }
      LockMode old_lock_mode = cur_request->lock_mode_;
      // 判断lock_mode与事务持有锁是否兼容
      if (!CanLockUpgrade(old_lock_mode, lock_mode)) {
        ThrowAbort(txn, AbortReason::INCOMPATIBLE_UPGRADE);
      }
      // 可以升级, 先删掉原始事务的请求，清理事务的set, 将新请求添加到等待请求的最前面
      request_queue->request_queue_.erase(it);
      DeleteTxnLockTable(txn, old_lock_mode, oid);
      request_queue->upgrading_ = txn->GetTransactionId();
      new_request = std::make_shared<LockRequest>(txn->GetTransactionId(), lock_mode, oid);
      auto insert_pos = request_queue->request_queue_.begin();
      while (insert_pos != request_queue->request_queue_.end() && (*insert_pos)->granted_) {
        ++insert_pos;
      }
      request_queue->request_queue_.insert(insert_pos, new_request);
      // process
      while (true) {
        if (GrantAllowed(txn, request_queue, lock_mode)) {
          new_request->granted_ = true;
          request_queue->upgrading_ = INVALID_TXN_ID;
          // 其实在这里可以释放队列锁了吧...
          InsertTxnLockTable(txn, lock_mode, oid);
          return true;
        }
        request_queue->cv_.wait(que_lock);
        if (txn->GetState() == TransactionState::ABORTED) {  // 死锁检测可能会让事务abort并唤醒
          request_queue->request_queue_.remove(new_request);
          request_queue->upgrading_ = INVALID_TXN_ID;
          // que_lock.unlock();
          request_queue->cv_.notify_all();
          return false;
        }
      }
    }
  }

  // 不是一个升级的请求，创建新的锁请求并添加到队列末尾
  if (new_request == nullptr) {
    new_request = std::make_shared<LockRequest>(txn->GetTransactionId(), lock_mode, oid);
    request_queue->request_queue_.push_back(new_request);
  }
  // process
  while (true) {
    if (GrantAllowed(txn, request_queue, lock_mode)) {
      new_request->granted_ = true;
      InsertTxnLockTable(txn, lock_mode, oid);
      return true;
    }
    request_queue->cv_.wait(que_lock);
    if (txn->GetState() == TransactionState::ABORTED) {  // 死锁检测可能会让事务abort并唤醒
      request_queue->request_queue_.remove(new_request);
      // que_lock.unlock();
      request_queue->cv_.notify_all();
      return false;
    }
  }
}

auto LockManager::UnlockTable(Transaction *txn, const table_oid_t &oid) -> bool {
  BUSTUB_ASSERT(txn != nullptr, "UnlockTable: txn is nullptr");
  BUSTUB_ASSERT(txn->GetState() != TransactionState::COMMITTED, "UnlockTable: txn is committed");

  // 检查事务是否持有该表锁
  LockMode lock_mode;
  bool found = false;
  auto lock_sets = {txn->GetIntentionSharedTableLockSet(), txn->GetSharedTableLockSet(),
                    txn->GetIntentionExclusiveTableLockSet(), txn->GetSharedIntentionExclusiveTableLockSet(),
                    txn->GetExclusiveTableLockSet()};
  for (const auto &lock_set : lock_sets) {
    if (lock_set->find(oid) != lock_set->end()) {
      found = true;
      if (lock_set == txn->GetIntentionSharedTableLockSet()) {
        lock_mode = LockMode::INTENTION_SHARED;
      } else if (lock_set == txn->GetSharedTableLockSet()) {
        lock_mode = LockMode::SHARED;
      } else if (lock_set == txn->GetIntentionExclusiveTableLockSet()) {
        lock_mode = LockMode::INTENTION_EXCLUSIVE;
      } else if (lock_set == txn->GetSharedIntentionExclusiveTableLockSet()) {
        lock_mode = LockMode::SHARED_INTENTION_EXCLUSIVE;
      } else if (lock_set == txn->GetExclusiveTableLockSet()) {
        lock_mode = LockMode::EXCLUSIVE;
      }
      break;
    }
  }
  if (!found) {
    ThrowAbort(txn, AbortReason::ATTEMPTED_UNLOCK_BUT_NO_LOCK_HELD);
  }

  // 不能在事务拥有该表行锁的情况下解除表锁
  const auto &s_row_set = txn->GetSharedRowLockSet();
  const auto &x_row_set = txn->GetExclusiveRowLockSet();
  if (!((s_row_set->find(oid) == s_row_set->end() || s_row_set->at(oid).empty()) &&
        (x_row_set->find(oid) == x_row_set->end() || x_row_set->at(oid).empty()))) {
    ThrowAbort(txn, AbortReason::TABLE_UNLOCKED_BEFORE_UNLOCKING_ROWS);
  }

  // 更新事务的状态为 SHRINKING
  switch (txn->GetIsolationLevel()) {
    case IsolationLevel::REPEATABLE_READ:
      if (lock_mode == LockMode::SHARED || lock_mode == LockMode::EXCLUSIVE) {
        txn->SetState(TransactionState::SHRINKING);
      }
      break;
    case IsolationLevel::READ_COMMITTED:
      if (lock_mode == LockMode::EXCLUSIVE) {
        txn->SetState(TransactionState::SHRINKING);
      }
      break;
    case IsolationLevel::READ_UNCOMMITTED:
      if (lock_mode == LockMode::EXCLUSIVE) {
        txn->SetState(TransactionState::SHRINKING);
      } else if (lock_mode == LockMode::SHARED) {
        ThrowAbort(txn, AbortReason::LOCK_SHARED_ON_READ_UNCOMMITTED);
      }
      break;
    default:
      break;
  }

  // 更新事务的 table_lock_set
  for (const auto &lock_set : lock_sets) {
    if (lock_set->find(oid) != lock_set->end()) {
      lock_set->erase(oid);
    }
  }

  // 上锁
  std::unique_lock<std::mutex> map_lock(table_lock_map_latch_);
  auto it = table_lock_map_.find(oid);
  if (it == table_lock_map_.end()) {
    ThrowAbort(txn, AbortReason::ATTEMPTED_UNLOCK_BUT_NO_LOCK_HELD);
  }
  auto &request_queue = it->second;
  std::unique_lock<std::mutex> queue_lock(request_queue->latch_);
  map_lock.unlock();

  // 移除请求队列中的该事务请求，并唤醒等待该资源的事务
  for (auto it = request_queue->request_queue_.begin(); it != request_queue->request_queue_.end(); ++it) {
    if ((*it)->txn_id_ == txn->GetTransactionId()) {
      request_queue->request_queue_.erase(it);
      break;
    }
  }
  queue_lock.unlock();
  request_queue->cv_.notify_all();
  return true;
}

auto LockManager::LockRow(Transaction *txn, LockMode lock_mode, const table_oid_t &oid, const RID &rid) -> bool {
  // 检查事务的状态是否合理
  BUSTUB_ASSERT(txn != nullptr, "UnlockRow: txn is nullptr");
  BUSTUB_ASSERT(txn->GetState() != TransactionState::ABORTED, "UnlockRow: txn is aborted");
  BUSTUB_ASSERT(txn->GetState() != TransactionState::COMMITTED, "UnlockRow: txn is committed");

  // 行锁只能为S/X
  if (lock_mode != LockMode::SHARED && lock_mode != LockMode::EXCLUSIVE) {
    ThrowAbort(txn, AbortReason::ATTEMPTED_INTENTION_LOCK_ON_ROW);
  }

  // 判断有无对应的表锁
  if (!CheckAppropriateLockOnTable(txn, oid, lock_mode)) {
    ThrowAbort(txn, AbortReason::TABLE_LOCK_NOT_PRESENT);
  }

  // 检查lock_mode与txn的隔离级别、状态的兼容性
  switch (txn->GetIsolationLevel()) {
    case bustub::IsolationLevel::REPEATABLE_READ: {
      if (txn->GetState() == bustub::TransactionState::SHRINKING) {
        ThrowAbort(txn, AbortReason::LOCK_ON_SHRINKING);
      }
      break;
    }
    case bustub::IsolationLevel::READ_COMMITTED: {
      if (txn->GetState() == bustub::TransactionState::SHRINKING && lock_mode != LockMode::SHARED &&
          lock_mode != LockMode::INTENTION_SHARED) {
        ThrowAbort(txn, AbortReason::LOCK_ON_SHRINKING);
      }
    } break;
    case bustub::IsolationLevel::READ_UNCOMMITTED: {
      if (lock_mode != LockMode::EXCLUSIVE && lock_mode != LockMode::INTENTION_EXCLUSIVE) {
        ThrowAbort(txn, AbortReason::LOCK_SHARED_ON_READ_UNCOMMITTED);
      }
      if (txn->GetState() != TransactionState::GROWING) {
        ThrowAbort(txn, AbortReason::LOCK_ON_SHRINKING);
      }
      break;
    }
    default:
      break;
  }

  // get请求队列
  row_lock_map_latch_.lock();
  if (row_lock_map_.find(rid) == row_lock_map_.end()) {
    row_lock_map_[rid] = std::make_shared<LockRequestQueue>();
  }
  auto &request_queue = row_lock_map_[rid];
  std::unique_lock<std::mutex> que_lock(request_queue->latch_);
  row_lock_map_latch_.unlock();

  std::shared_ptr<LockRequest> new_request = nullptr;
  // 检查是否事务已有相应的锁 / 可升级的锁
  for (auto it = request_queue->request_queue_.begin(); it != request_queue->request_queue_.end(); ++it) {
    auto cur_request = *it;
    if (cur_request->txn_id_ == txn->GetTransactionId()) {
      if (cur_request->lock_mode_ == lock_mode) {  // 该事务有相同的锁，返回true
        return true;
      }
      /** 有锁，但不同...可能需要升级...但当前队列只能有一个升级的请求 */
      if (request_queue->upgrading_ != INVALID_TXN_ID) {
        ThrowAbort(txn, AbortReason::UPGRADE_CONFLICT);
      }
      LockMode old_lock_mode = cur_request->lock_mode_;
      // 判断lock_mode与事务持有锁是否兼容
      if (!CanLockUpgrade(old_lock_mode, lock_mode)) {
        ThrowAbort(txn, AbortReason::INCOMPATIBLE_UPGRADE);
      }
      // 可以升级, 先删掉原始事务的请求，清理事务的set, 将新请求添加到等待请求的最前面
      request_queue->request_queue_.erase(it);
      DeleteTxnLockRow(txn, old_lock_mode, oid, rid);
      request_queue->upgrading_ = txn->GetTransactionId();
      new_request = std::make_shared<LockRequest>(txn->GetTransactionId(), lock_mode, oid);
      auto insert_pos = request_queue->request_queue_.begin();
      while (insert_pos != request_queue->request_queue_.end() && (*insert_pos)->granted_) {
        ++insert_pos;
      }
      request_queue->request_queue_.insert(insert_pos, new_request);
      // process
      while (true) {
        if (GrantAllowed(txn, request_queue, lock_mode)) {
          new_request->granted_ = true;
          request_queue->upgrading_ = INVALID_TXN_ID;
          InsertTxnLockRow(txn, lock_mode, oid, rid);
          return true;
        }
        request_queue->cv_.wait(que_lock);
        if (txn->GetState() == TransactionState::ABORTED) {  // 死锁检测可能会让事务abort并唤醒
          request_queue->request_queue_.remove(new_request);
          request_queue->upgrading_ = INVALID_TXN_ID;
          que_lock.unlock();
          request_queue->cv_.notify_all();
          return false;
        }
      }
    }
  }

  // 不是一个升级的请求，创建新的锁请求并添加到队列末尾
  if (new_request == nullptr) {
    new_request = std::make_shared<LockRequest>(txn->GetTransactionId(), lock_mode, oid);
    request_queue->request_queue_.push_back(new_request);
  }
  // process
  while (true) {
    if (GrantAllowed(txn, request_queue, lock_mode)) {
      new_request->granted_ = true;
      InsertTxnLockRow(txn, lock_mode, oid, rid);
      return true;
    }
    request_queue->cv_.wait(que_lock);
    if (txn->GetState() == TransactionState::ABORTED) {  // 死锁检测可能会让事务abort并唤醒
      request_queue->request_queue_.remove(new_request);
      que_lock.unlock();
      request_queue->cv_.notify_all();
      return false;
    }
  }
}

auto LockManager::UnlockRow(Transaction *txn, const table_oid_t &oid, const RID &rid, bool force) -> bool {
  BUSTUB_ASSERT(txn != nullptr, "UnlockRow: txn is nullptr");
  BUSTUB_ASSERT(txn->GetState() != TransactionState::COMMITTED, "UnlockRow: txn is committed");

  // 检查事务是否持有该行锁
  LockMode lock_mode;
  bool found = false;
  auto lock_set = txn->GetSharedRowLockSet();
  if (lock_set->find(oid) != lock_set->end() && lock_set->at(oid).find(rid) != lock_set->at(oid).end()) {
    found = true;
    lock_mode = LockMode::SHARED;
  }
  lock_set = txn->GetExclusiveRowLockSet();
  if (lock_set->find(oid) != lock_set->end() && lock_set->at(oid).find(rid) != lock_set->at(oid).end()) {
    found = true;
    lock_mode = LockMode::EXCLUSIVE;
  }
  if (!found) {
    ThrowAbort(txn, AbortReason::ATTEMPTED_UNLOCK_BUT_NO_LOCK_HELD);
  }

  // 更新事务的状态为 SHRINKING
  switch (txn->GetIsolationLevel()) {
    case IsolationLevel::REPEATABLE_READ:
      if (lock_mode == LockMode::SHARED || lock_mode == LockMode::EXCLUSIVE) {
        txn->SetState(TransactionState::SHRINKING);
      }
      break;
    case IsolationLevel::READ_COMMITTED:
      if (lock_mode == LockMode::EXCLUSIVE) {
        txn->SetState(TransactionState::SHRINKING);
      }
      break;
    case IsolationLevel::READ_UNCOMMITTED:
      if (lock_mode == LockMode::EXCLUSIVE) {
        txn->SetState(TransactionState::SHRINKING);
      } else if (lock_mode == LockMode::SHARED) {
        ThrowAbort(txn, AbortReason::LOCK_SHARED_ON_READ_UNCOMMITTED);
      }
      break;
    default:
      break;
  }

  // 更新事务的 row_lock_set
  lock_set = txn->GetSharedRowLockSet();
  if (lock_set->find(oid) != lock_set->end()) {
    lock_set->at(oid).erase(rid);
  }
  lock_set = txn->GetExclusiveRowLockSet();
  if (lock_set->find(oid) != lock_set->end()) {
    lock_set->at(oid).erase(rid);
  }

  // 上锁
  std::unique_lock<std::mutex> map_lock(row_lock_map_latch_);
  auto it = row_lock_map_.find(rid);
  if (it == row_lock_map_.end()) {
    ThrowAbort(txn, AbortReason::ATTEMPTED_UNLOCK_BUT_NO_LOCK_HELD);
  }
  auto &request_queue = it->second;
  std::unique_lock<std::mutex> queue_lock(request_queue->latch_);
  map_lock.unlock();

  // 移除请求队列中的该事务请求，并唤醒等待该资源的事务
  for (auto it = request_queue->request_queue_.begin(); it != request_queue->request_queue_.end(); ++it) {
    if ((*it)->txn_id_ == txn->GetTransactionId()) {
      request_queue->request_queue_.erase(it);
      break;
    }
  }
  queue_lock.unlock();
  request_queue->cv_.notify_all();
  return true;
}

void LockManager::UnlockAll() {
  // You probably want to unlock all table and txn locks here.
}

void LockManager::AddEdge(txn_id_t t1, txn_id_t t2) {}

void LockManager::RemoveEdge(txn_id_t t1, txn_id_t t2) {}

auto LockManager::HasCycle(txn_id_t *txn_id) -> bool { return false; }

auto LockManager::GetEdgeList() -> std::vector<std::pair<txn_id_t, txn_id_t>> {
  std::vector<std::pair<txn_id_t, txn_id_t>> edges(0);
  return edges;
}

void LockManager::RunCycleDetection() {
  while (enable_cycle_detection_) {
    std::this_thread::sleep_for(cycle_detection_interval);
    {  // TODO(students): detect deadlock
    }
  }
}

/** my help func */
void LockManager::ThrowAbort(Transaction *txn, AbortReason reason) {
  txn->SetState(TransactionState::ABORTED);
  throw TransactionAbortException(txn->GetTransactionId(), reason);
}

void LockManager::DeleteTxnLockTable(Transaction *txn, LockMode lock_mode, table_oid_t oid) {
  txn->LockTxn();
  if (lock_mode == LockMode::INTENTION_SHARED) {
    txn->GetIntentionSharedTableLockSet()->erase(oid);
  } else if (lock_mode == LockMode::SHARED) {
    txn->GetSharedTableLockSet()->erase(oid);
  } else if (lock_mode == LockMode::INTENTION_EXCLUSIVE) {
    txn->GetIntentionExclusiveTableLockSet()->erase(oid);
  } else if (lock_mode == LockMode::SHARED_INTENTION_EXCLUSIVE) {
    txn->GetSharedIntentionExclusiveTableLockSet()->erase(oid);
  } else {  // lock_mode == LockMode::EXCLUSIVE
    txn->GetExclusiveTableLockSet()->erase(oid);
  }
  txn->UnlockTxn();
}

void LockManager::InsertTxnLockTable(Transaction *txn, LockMode lock_mode, table_oid_t oid) {
  txn->LockTxn();
  if (lock_mode == LockMode::INTENTION_SHARED) {
    txn->GetIntentionSharedTableLockSet()->insert(oid);
  } else if (lock_mode == LockMode::SHARED) {
    txn->GetSharedTableLockSet()->insert(oid);
  } else if (lock_mode == LockMode::INTENTION_EXCLUSIVE) {
    txn->GetIntentionExclusiveTableLockSet()->insert(oid);
  } else if (lock_mode == LockMode::SHARED_INTENTION_EXCLUSIVE) {
    txn->GetSharedIntentionExclusiveTableLockSet()->insert(oid);
  } else {  // lock_mode == LockMode::EXCLUSIVE
    txn->GetExclusiveTableLockSet()->insert(oid);
  }
  txn->UnlockTxn();
}

auto LockManager::GrantAllowed(Transaction *txn, const std::shared_ptr<LockRequestQueue> &request_queue,
                               LockMode lock_mode) -> bool {
  for (const auto &req : request_queue->request_queue_) {
    // 如果当前请求不兼容之前的任何一个请求，则返回 false
    if (req->txn_id_ != txn->GetTransactionId() && !AreLocksCompatible(req->lock_mode_, lock_mode)) {
      return false;
    }
    // 如果遇到未授予的请求，则停止检查
    if (!req->granted_) {
      break;
    }
  }
  return true;
}

void LockManager::InsertTxnLockRow(Transaction *txn, LockMode lock_mode, const table_oid_t &oid, const RID &rid) {
  txn->LockTxn();
  if (lock_mode == LockMode::SHARED) {
    if (txn->GetSharedRowLockSet()->find(oid) == txn->GetSharedRowLockSet()->end()) {
      txn->GetSharedRowLockSet()->emplace(oid, std::unordered_set<RID>());
    }
    txn->GetSharedRowLockSet()->at(oid).insert(rid);
  } else if (lock_mode == LockMode::EXCLUSIVE) {
    if (txn->GetExclusiveRowLockSet()->find(oid) == txn->GetExclusiveRowLockSet()->end()) {
      txn->GetExclusiveRowLockSet()->emplace(oid, std::unordered_set<RID>());
    }
    txn->GetExclusiveRowLockSet()->at(oid).insert(rid);
  }
  txn->UnlockTxn();
}

void LockManager::DeleteTxnLockRow(Transaction *txn, LockMode lock_mode, const table_oid_t &oid, const RID &rid) {
  txn->LockTxn();
  if (lock_mode == LockMode::SHARED) {
    txn->GetSharedRowLockSet()->at(oid).erase(rid);
  } else if (lock_mode == LockMode::EXCLUSIVE) {
    txn->GetExclusiveRowLockSet()->at(oid).erase(rid);
  }
  txn->UnlockTxn();
}

/** 可选实现的辅助函数 */
auto LockManager::CanLockUpgrade(LockMode curr_lock_mode, LockMode requested_lock_mode) -> bool {
  switch (curr_lock_mode) {
    case LockMode::INTENTION_SHARED:
      return requested_lock_mode == LockMode::SHARED || requested_lock_mode == LockMode::EXCLUSIVE ||
             requested_lock_mode == LockMode::INTENTION_EXCLUSIVE ||
             requested_lock_mode == LockMode::SHARED_INTENTION_EXCLUSIVE;
    case LockMode::SHARED:
    case LockMode::INTENTION_EXCLUSIVE:
      return requested_lock_mode == LockMode::EXCLUSIVE || requested_lock_mode == LockMode::SHARED_INTENTION_EXCLUSIVE;
    case LockMode::SHARED_INTENTION_EXCLUSIVE:
      return requested_lock_mode == LockMode::EXCLUSIVE;
    default:
      return false;
  }
}

auto LockManager::AreLocksCompatible(LockMode l1, LockMode l2) -> bool {
  switch (l1) {
    case LockMode::INTENTION_SHARED: {
      if (l2 == LockMode::EXCLUSIVE) {
        return false;
      }
    } break;
    case LockMode::INTENTION_EXCLUSIVE: {
      if (l2 != LockMode::INTENTION_SHARED && l2 != LockMode::INTENTION_EXCLUSIVE) {
        return false;
      }
    } break;
    case LockMode::SHARED: {
      if (l2 != LockMode::SHARED && l2 != LockMode::INTENTION_SHARED) {
        return false;
      }
    } break;
    case LockMode::SHARED_INTENTION_EXCLUSIVE: {
      if (l2 != LockMode::INTENTION_SHARED) {
        return false;
      }
    } break;
    default:
      return false;  // EXCLUSIVE
  }
  return true;
}

auto LockManager::CheckAppropriateLockOnTable(Transaction *txn, const table_oid_t &oid, LockMode row_lock_mode)
    -> bool {
  if (row_lock_mode == LockMode::EXCLUSIVE) {  // at least IX
    if (!(txn->IsTableIntentionExclusiveLocked(oid) || txn->IsTableExclusiveLocked(oid) ||
          txn->IsTableSharedIntentionExclusiveLocked(oid))) {
      return false;
    }
  }
  if (row_lock_mode == LockMode::SHARED) {  // at least IS
    if (!(txn->IsTableIntentionExclusiveLocked(oid) || txn->IsTableExclusiveLocked(oid) ||
          txn->IsTableSharedIntentionExclusiveLocked(oid) || txn->IsTableSharedLocked(oid) ||
          txn->IsTableIntentionSharedLocked(oid))) {
      return false;
    }
  }
  return true;
}
}  // namespace bustub
