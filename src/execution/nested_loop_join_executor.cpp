//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// nested_loop_join_executor.cpp
//
// Identification: src/execution/nested_loop_join_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/nested_loop_join_executor.h"
#include <cstdint>
#include "binder/table_ref/bound_join_ref.h"
#include "catalog/schema.h"
#include "common/exception.h"
#include "execution/executor_context.h"
#include "type/type.h"
#include "type/value_factory.h"

namespace bustub {

NestedLoopJoinExecutor::NestedLoopJoinExecutor(ExecutorContext *exec_ctx, const NestedLoopJoinPlanNode *plan,
                                               std::unique_ptr<AbstractExecutor> &&left_executor,
                                               std::unique_ptr<AbstractExecutor> &&right_executor)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      left_executor_(std::move(left_executor)),
      right_executor_(std::move(right_executor)),
      joined_(false) {
  if (plan->GetJoinType() != JoinType::LEFT && plan->GetJoinType() != JoinType::INNER) {
    // Note for 2023 Spring: You ONLY need to implement left join and inner join.
    throw bustub::NotImplementedException(fmt::format("join type {} not supported", plan->GetJoinType()));
  }
}

void NestedLoopJoinExecutor::Init() {
  left_executor_->Init();
  right_executor_->Init();
  RID rid;
  left_executor_->Next(&left_tuple_, &rid);
  joined_ = false;
}

auto NestedLoopJoinExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  auto &predicate_expr = plan_->Predicate();
  auto &left_plan_schema = plan_->GetLeftPlan()->OutputSchema();
  auto &right_plan_schema = plan_->GetRightPlan()->OutputSchema();

  while (true) {
    while (right_executor_->Next(&right_tuple_, rid)) {
      auto value = predicate_expr->EvaluateJoin(&left_tuple_, left_plan_schema, &right_tuple_, right_plan_schema);
      if (!value.IsNull() && value.GetAs<bool>()) {
        OutputTuple(left_plan_schema, right_plan_schema, tuple, true);
        return true;
      }
    }

    if (plan_->GetJoinType() == JoinType::LEFT && !joined_) {
      OutputTuple(left_plan_schema, right_plan_schema, tuple, false);
      right_executor_->Init();
      return true;
    }

    if (!left_executor_->Next(&left_tuple_, rid)) {
      return false;
    }
    right_executor_->Init();
    joined_ = false;
  }
}

auto NestedLoopJoinExecutor::OutputTuple(const Schema &left_plan_schema, const Schema &right_plan_schema, Tuple *tuple,
                                         bool isMatched) -> void {
  std::vector<Value> values;
  values.reserve(plan_->OutputSchema().GetColumnCount());
  for (uint32_t i = 0; i < left_plan_schema.GetColumnCount(); i++) {
    values.emplace_back(left_tuple_.GetValue(&left_plan_schema, i));
  }

  if (!isMatched) {
    for (uint32_t i = 0; i < right_plan_schema.GetColumnCount(); i++) {
      auto type_id = right_plan_schema.GetColumn(i).GetType();
      values.emplace_back(ValueFactory::GetNullValueByType(type_id));
    }
  } else {
    for (uint32_t i = 0; i < right_plan_schema.GetColumnCount(); i++) {
      values.emplace_back(right_tuple_.GetValue(&right_plan_schema, i));
    }
  }
  joined_ = true;
  *tuple = Tuple(values, &plan_->OutputSchema());
}
}  // namespace bustub
