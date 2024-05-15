//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// insert_executor.cpp
//
// Identification: src/execution/insert_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>
#include <optional>
#include "common/config.h"
#include "storage/table/tuple.h"

#include "execution/executors/insert_executor.h"

namespace bustub {

InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void InsertExecutor::Init() {
  auto catalog = exec_ctx_->GetCatalog();
  table_info_ = catalog->GetTable(plan_->TableOid());
  index_info_ = catalog->GetTableIndexes(table_info_->name_);
  child_executor_->Init();
  has_finished_ = false;
}
/**
 * Yield the number of rows inserted into the table.
 * @param[out] tuple The integer tuple indicating the number of rows inserted into the table
 * @param[out] rid The next tuple RID produced by the insert (ignore, not used)
 * @return `true` if a tuple was produced, `false` if there are no more tuples
 *
 * NOTE: InsertExecutor::Next() does not use the `rid` out-parameter.
 * NOTE: InsertExecutor::Next() returns true with number of inserted rows produced only once.
 */
auto InsertExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if (has_finished_) {
    return false;
  }

  int num_inserted = 0;
  while (child_executor_->Next(tuple, rid)) {
    TupleMeta tuple_meta = {INVALID_TXN_ID, INVALID_TXN_ID, false};
    std::optional<RID> opt = table_info_->table_->InsertTuple(tuple_meta, *tuple);
    if (!opt.has_value()) {
      return false;
    }
    ++num_inserted;
    *rid = opt.value();

    for (auto &index_info : index_info_) {
      auto key = tuple->KeyFromTuple(table_info_->schema_, index_info->key_schema_, index_info->index_->GetKeyAttrs());
      if (!index_info->index_->InsertEntry(key, *rid, exec_ctx_->GetTransaction())) {
        return false;
      }
    }
  }
  // 最后的 tuple 应该包含插入的 tuple 数量的信息
  std::vector<Value> values{{TypeId::INTEGER, num_inserted}};
  Tuple output_tuple(values, &GetOutputSchema());
  *tuple = output_tuple;

  has_finished_ = true;
  return true;
}

}  // namespace bustub
