//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// delete_executor.cpp
//
// Identification: src/execution/delete_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>
#include <vector>
#include "catalog/catalog.h"
#include "common/config.h"
#include "storage/table/tuple.h"
#include "type/type_id.h"

#include "execution/executors/delete_executor.h"

namespace bustub {

DeleteExecutor::DeleteExecutor(ExecutorContext *exec_ctx, const DeletePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void DeleteExecutor::Init() {
  auto catalog = exec_ctx_->GetCatalog();
  table_info_ = catalog->GetTable(plan_->TableOid());
  indexes_info_ = catalog->GetTableIndexes(table_info_->name_);
  child_executor_->Init();
  has_finished_ = false;
}

auto DeleteExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if (has_finished_) {
    return false;
  }
  int num_deleted = 0;
  while (child_executor_->Next(tuple, rid)) {
    // 逻辑删除tuple
    TupleMeta delete_tuplemeta{INVALID_TXN_ID, INVALID_TXN_ID, true};
    table_info_->table_->UpdateTupleMeta(delete_tuplemeta, *rid);
    // 删除相关索引
    for (auto &index_info : indexes_info_) {
      Tuple delete_key =
          tuple->KeyFromTuple(table_info_->schema_, index_info->key_schema_, index_info->index_->GetKeyAttrs());
      index_info->index_->DeleteEntry(delete_key, *rid, exec_ctx_->GetTransaction());
    }
    num_deleted++;
  }
  // 返回
  std::vector<Value> values = {{TypeId::INTEGER, num_deleted}};
  Tuple res{values, &this->GetOutputSchema()};
  *tuple = res;
  has_finished_ = true;
  return true;
}

}  // namespace bustub
