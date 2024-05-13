#include "execution/executors/topn_executor.h"
#include <cstddef>
#include <queue>
#include <vector>

namespace bustub {

TopNExecutor::TopNExecutor(ExecutorContext *exec_ctx, const TopNPlanNode *plan,
                           std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)), count_(0) {}

void TopNExecutor::Init() {
  count_ = 0;
  child_executor_->Init();
  vec_.clear();
  // heap Init
  auto &order_bys = plan_->GetOrderBy();
  auto &schema = child_executor_->GetOutputSchema();
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
  std::priority_queue<std::pair<Tuple, RID>, std::vector<std::pair<Tuple, RID>>, decltype(sort_by)> heap(sort_by);
  // next
  Tuple tuple;
  RID rid;
  size_t n = GetNumInHeap();
  while(child_executor_->Next(&tuple, &rid)){
    heap.emplace(tuple, rid);
    if(heap.size() > n){
        heap.pop();
    }
  }
  // std::cout << heap.size() << std::endl;
  // copy to my vec...
  while(!heap.empty()){
    auto pair = heap.top();
    heap.pop();
    // std::cout << pair.first.ToString(&plan_->OutputSchema()) << " " << pair.second.ToString() << std::endl;
    vec_.emplace_back(pair);
  }
  std::reverse(vec_.begin(), vec_.end());
}

auto TopNExecutor::Next(Tuple *tuple, RID *rid) -> bool {
    if(count_ >= vec_.size()){
        return false;
    }
    auto& pair = vec_.at(count_);
    count_++;
    *tuple = pair.first;
    *rid = pair.second;
    return true;
}

auto TopNExecutor::GetNumInHeap() -> size_t { return plan_->GetN(); };

}  // namespace bustub
