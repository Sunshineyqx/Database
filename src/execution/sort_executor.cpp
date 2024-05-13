#include "execution/executors/sort_executor.h"
#include "common/rid.h"

namespace bustub {

SortExecutor::SortExecutor(ExecutorContext *exec_ctx, const SortPlanNode *plan,
                           std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void SortExecutor::Init() {
  child_executor_->Init();
  ordered_pairs_.clear();

  Tuple tuple;
  RID rid;
  while (child_executor_->Next(&tuple, &rid)) {
    ordered_pairs_.emplace_back(tuple, rid);
  }

  auto &order_bys = plan_->GetOrderBy();
  auto &schema = plan_->OutputSchema();
  auto sort_by = [&order_bys, &schema](std::pair<Tuple, RID> &t1, std::pair<Tuple, RID> &t2) -> bool {
    for (const auto &order_by : order_bys) {
      Value val1 = order_by.second->Evaluate(&t1.first, schema);
      Value val2 = order_by.second->Evaluate(&t2.first, schema);
      /*
      std::cout << "order_by.second: " << order_by.second->ToString() << std::endl;
      std::cout << "val1: " << val1.ToString() << std::endl;
      std::cout << "val2: " << val2.ToString() << std::endl << std::endl;
      */
      if (val1.CompareEquals(val2) == CmpBool::CmpTrue) {  // continue if they equal in this val
        continue;
      }

      return order_by.first == OrderByType::DEFAULT || order_by.first == OrderByType::ASC
                 ? val1.CompareLessThan(val2) == CmpBool::CmpTrue
                 : val1.CompareGreaterThan(val2) == CmpBool::CmpTrue;
    }
    return false;  // return false if they equal in every val (namely themself)
  };

  std::sort(ordered_pairs_.begin(), ordered_pairs_.end(), sort_by);
  cur_iterator_ = ordered_pairs_.begin();
}

auto SortExecutor::Next(Tuple *tuple, RID *rid) -> bool {
    if(cur_iterator_ == ordered_pairs_.end()){
        return false;
    }
    auto& res = *cur_iterator_;
    cur_iterator_++;
    *tuple = res.first;
    *rid = res.second;
    return true;
}

}  // namespace bustub
