//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_join_executor.cpp
//
// Identification: src/execution/hash_join_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/hash_join_executor.h"
#include <cstddef>
#include <cstdint>
#include "binder/table_ref/bound_join_ref.h"
#include "common/config.h"
#include "common/rid.h"
#include "type/type.h"
#include "type/value_factory.h"
namespace bustub {

HashJoinExecutor::HashJoinExecutor(ExecutorContext *exec_ctx, const HashJoinPlanNode *plan,
                                   std::unique_ptr<AbstractExecutor> &&left_child,
                                   std::unique_ptr<AbstractExecutor> &&right_child)
    : AbstractExecutor(exec_ctx),plan_(plan), left_executor_(std::move(left_child)), right_executor_(std::move(right_child)) {
  if (plan->GetJoinType() != JoinType::LEFT && plan->GetJoinType() != JoinType::INNER) {
    // Note for 2023 Spring: You ONLY need to implement left join and inner join.
    throw bustub::NotImplementedException(fmt::format("join type {} not supported", plan->GetJoinType()));
  }
}

void HashJoinExecutor::Init() {
  // 方便left_join, 对右边/内表进行hash
  left_executor_->Init();
  right_executor_->Init();
  ht_.clear();
  is_begin_ = true;

  Tuple tuple;
  RID rid;
  while (right_executor_->Next(&tuple, &rid)) {
    JoinKey join_key = MakeJoinKey(tuple, plan_->RightJoinKeyExpressions(), plan_->GetRightPlan());
    InsertJoinKey(join_key, tuple);
  }
}

auto HashJoinExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  auto &left_plan_schema = plan_->GetLeftPlan()->OutputSchema();
  auto &right_plan_schema = plan_->GetRightPlan()->OutputSchema();

  while (true) {
    if (is_begin_) {
      is_begin_ = false;
      const auto &status = left_executor_->Next(&left_tuple_, rid);
      if (!status) {
        return false;
      }
      JoinKey left_join_key = MakeJoinKey(left_tuple_, plan_->LeftJoinKeyExpressions(), plan_->GetLeftPlan());
      range_pair_it_ = ht_.equal_range(left_join_key);
      cur_it_ = range_pair_it_.first;
      bucket_finished_ = (cur_it_ == range_pair_it_.second);
    }
    // 没有对应的kv
    if (bucket_finished_) {
      is_begin_ = true;
      if (plan_->join_type_ == JoinType::LEFT) {
        OutputTuple(left_plan_schema, right_plan_schema, nullptr, tuple, false);  // left join
        return true;
      }
      continue;
    }

    Tuple right_tuple = cur_it_->second;
    cur_it_++;
    OutputTuple(left_plan_schema, right_plan_schema, &right_tuple, tuple, true);
    if (cur_it_ == range_pair_it_.second) {
      is_begin_ = true;
    }
    return true;
  }
}
auto HashJoinExecutor::OutputTuple(const Schema &left_table_schema, const Schema &right_table_schema, Tuple *right_tuple,
                                   Tuple *tuple, bool matched)->void{
  std::vector<Value> values;
  values.reserve(GetOutputSchema().GetColumnCount());
  for(uint32_t i = 0; i < left_table_schema.GetColumnCount(); i++){
    values.emplace_back(left_tuple_.GetValue(&left_table_schema, i));
  }

  if(!matched){
     for (uint32_t i = 0; i < right_table_schema.GetColumnCount(); ++i) {
      auto type_id = right_table_schema.GetColumn(i).GetType();
      values.emplace_back(ValueFactory::GetNullValueByType(type_id));
    }
  } else {
    for (uint32_t i = 0; i < right_table_schema.GetColumnCount(); ++i) {
      values.emplace_back(right_tuple->GetValue(&right_table_schema, i));
    }
  }
  *tuple = {values, &plan_->OutputSchema()};
}
}  // namespace bustub
