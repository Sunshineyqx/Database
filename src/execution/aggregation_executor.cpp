//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// aggregation_executor.cpp
//
// Identification: src/execution/aggregation_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include <memory>
#include <vector>
#include "type/type.h"

#include "execution/executors/aggregation_executor.h"

namespace bustub {

AggregationExecutor::AggregationExecutor(ExecutorContext *exec_ctx, const AggregationPlanNode *plan,
                                         std::unique_ptr<AbstractExecutor> &&child)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      child_(std::move(child)),
      aht_(plan->GetAggregates(), plan->GetAggregateTypes()),
      aht_iterator_(aht_.Begin()),
      num_of_tuples_(0) {}

void AggregationExecutor::Init() {
  child_->Init();
  aht_.Clear();
  num_of_tuples_ = 0;
  Tuple tuple;
  RID rid;
  while (child_->Next(&tuple, &rid)) {
    aht_.InsertCombine(MakeAggregateKey(&tuple), MakeAggregateValue(&tuple));
    num_of_tuples_++;
  }
  aht_iterator_ = aht_.Begin();
}

auto AggregationExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  // 空表搭配 group by 不需要有任何输出
  if (num_of_tuples_ == 0 && plan_->group_bys_.empty()) {  // empty table
    Tuple output_tuple(aht_.GenerateInitialAggregateValue().aggregates_, &plan_->OutputSchema());
    *tuple = output_tuple;
    *rid = output_tuple.GetRid();
    --num_of_tuples_;  // 防止下次再次进入相同的逻辑
    return true;
  }
  if (aht_iterator_ == aht_.End()) {
    return false;
  }
  std::vector<Value> values = aht_iterator_.Key().group_bys_;
  values.insert(values.end(), aht_iterator_.Val().aggregates_.begin(), aht_iterator_.Val().aggregates_.end());
  Tuple output_tuple(values, &plan_->OutputSchema());

  *tuple = output_tuple;
  *rid = output_tuple.GetRid();
  ++aht_iterator_;
  return true;
}

auto AggregationExecutor::GetChildExecutor() const -> const AbstractExecutor * { return child_.get(); }

}  // namespace bustub
