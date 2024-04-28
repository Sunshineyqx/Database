//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// update_executor.cpp
//
// Identification: src/execution/update_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include <memory>
#include "catalog/catalog.h"
#include "common/config.h"
#include "concurrency/lock_manager.h"
#include "type/type_id.h"
#include "type/value.h"

#include "execution/executors/update_executor.h"

namespace bustub {

UpdateExecutor::UpdateExecutor(ExecutorContext *exec_ctx, const UpdatePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void UpdateExecutor::Init() {
  auto catalog = exec_ctx_->GetCatalog();
  table_info_ = catalog->GetTable(plan_->TableOid());
  indexes_info_ = catalog->GetTableIndexes(table_info_->name_);
  child_executor_->Init();
  has_finished_ = false;
}

auto UpdateExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if (has_finished_) {
    return false;
  }

  int num_inserted = 0;
  while (child_executor_->Next(tuple, rid)) {
    // 先删除 
    TupleMeta tuple_meta = {INVALID_TXN_ID, INVALID_TXN_ID, true};
    table_info_->table_->UpdateTupleMeta(tuple_meta, *rid);

    // 插入
    std::vector<Value> values;
    values.reserve(plan_->target_expressions_.size());
    for (const auto &expression : plan_->target_expressions_) {
      values.push_back(expression->Evaluate(tuple, table_info_->schema_));
      // std::cout<<values.back().ToString() << " ";
    }
    Tuple new_tuple{values, &table_info_->schema_};
    tuple_meta.is_deleted_ = false;
    std::optional<RID> res = table_info_->table_->InsertTuple(tuple_meta, new_tuple, exec_ctx_->GetLockManager(), exec_ctx_->GetTransaction(), plan_->TableOid());
    if (!res.has_value()) {
      return false;
    }
    num_inserted++;

    // 修改索引
    for (auto &index : indexes_info_) {
      // 删除旧的索引
      Tuple removed_key = tuple->KeyFromTuple(table_info_->schema_, index->key_schema_, index->index_->GetKeyAttrs());
      index->index_->DeleteEntry(removed_key, *rid, exec_ctx_->GetTransaction());
      // 增加新索引
      Tuple new_key = new_tuple.KeyFromTuple(table_info_->schema_, index->key_schema_, index->index_->GetKeyAttrs());
      if(!index->index_->InsertEntry(new_key, res.value(), exec_ctx_->GetTransaction())){
        return false;
      }
    }
  }
  std::vector<Value> value = {{TypeId::INTEGER, num_inserted}};
  Tuple res_tuple{value, &this->GetOutputSchema()};
  *tuple = res_tuple;
  has_finished_ = true;
  return true;
}

}  // namespace bustub
