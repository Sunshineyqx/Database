//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// seq_scan_executor.cpp
//
// Identification: src/execution/seq_scan_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/seq_scan_executor.h"
#include <cstddef>
#include "storage/table/table_iterator.h"

namespace bustub {

SeqScanExecutor::SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan) : AbstractExecutor(exec_ctx), plan_(plan), iter_(nullptr) {}

SeqScanExecutor::~SeqScanExecutor(){
    delete iter_;
    iter_ = nullptr;
}

void SeqScanExecutor::Init() {
    std::cout << "SeqScanExecutor::Init()" << '\n';
    auto catalog = exec_ctx_->GetCatalog();
    auto table_info = catalog->GetTable(plan_->GetTableOid());
    // hash join 的时候, seqscan 作为 right_child 会被多次调用 Init()
    // 所以每次初始化都需要把之前的 TableIterator 内存释放掉
    if(iter_ != nullptr){
        delete iter_;
        iter_ = new TableIterator(table_info->table_->MakeIterator());  
    } else {
        iter_ = new TableIterator(table_info->table_->MakeIterator());  
    }
}

auto SeqScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
    while(true){
        if(iter_->IsEnd()){
            return false;
        }

        auto&& tuple_pair = iter_->GetTuple();
        if(!tuple_pair.first.is_deleted_){
            *tuple = std::move(tuple_pair.second);
            *rid = iter_->GetRID();
            ++(*iter_);
            return true;
        }
        ++(*iter_);
    }
}

}  // namespace bustub

