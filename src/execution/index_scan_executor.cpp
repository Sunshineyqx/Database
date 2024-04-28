//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// index_scan_executor.cpp
//
// Identification: src/execution/index_scan_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include "execution/executors/index_scan_executor.h"
#include "type/value.h"

namespace bustub {
IndexScanExecutor::IndexScanExecutor(ExecutorContext *exec_ctx, const IndexScanPlanNode *plan)
    : AbstractExecutor(exec_ctx), plan_(plan){}

void IndexScanExecutor::Init() {
    auto catalog = exec_ctx_->GetCatalog();
    auto index_info = catalog->GetIndex(plan_->GetIndexOid());
    table_info_ = catalog->GetTable(index_info->table_name_);
    index_ = dynamic_cast<BPlusTreeIndexForTwoIntegerColumn *>(index_info->index_.get());
    it_ = index_->GetBeginIterator();
    end_ = index_->GetEndIterator();
}

auto IndexScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
    while(true){
        if(it_ == end_){
            return false;
        }

        RID cur_rid = (*it_).second;
        auto tuple_pair = table_info_->table_->GetTuple(cur_rid);
        if(!tuple_pair.first.is_deleted_){
            *rid = cur_rid;
            *tuple = tuple_pair.second;
            ++it_;
            return true;
        }
        ++it_;
    }
}

}  // namespace bustub
