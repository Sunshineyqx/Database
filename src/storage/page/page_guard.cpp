#include "storage/page/page_guard.h"
#include "buffer/buffer_pool_manager.h"

namespace bustub {
// Basic
BasicPageGuard::BasicPageGuard(BasicPageGuard &&that) noexcept {
    if(&that == this){
        return;
    }
    this->bpm_ = that.bpm_;
    that.bpm_ = nullptr;
    this->page_ = that.page_;
    that.page_ = nullptr;
    this->is_dirty_ = that.is_dirty_;
    that.is_dirty_ = false;
}

void BasicPageGuard::Drop() {
    if(bpm_ != nullptr){
        bpm_->UnpinPage(this->PageId(), this->is_dirty_);
    }
    this->bpm_ = nullptr;
    this->page_ = nullptr;
    this->is_dirty_ = false;
}

auto BasicPageGuard::operator=(BasicPageGuard &&that) noexcept -> BasicPageGuard & {
    if(&that == this){
        return *this;
    }
    this->Drop();
    this->bpm_ = that.bpm_;
    that.bpm_ = nullptr;
    this->page_ = that.page_;
    that.page_ = nullptr;
    this->is_dirty_ = that.is_dirty_;
    that.is_dirty_ = false;
    return *this; 
}

BasicPageGuard::~BasicPageGuard() {
    this->Drop();
};  // NOLINT

// Read
ReadPageGuard::ReadPageGuard(ReadPageGuard &&that) noexcept :guard_(std::move(that.guard_)){};

auto ReadPageGuard::operator=(ReadPageGuard &&that) noexcept -> ReadPageGuard & {
    this->guard_ = std::move(that.guard_);
    return *this; 
}

void ReadPageGuard::Drop() {
    if(guard_.bpm_ != nullptr){
        guard_.bpm_->UnpinPage(guard_.PageId(), guard_.is_dirty_);
    }
    if(guard_.page_ != nullptr){
        guard_.page_->RUnlatch();
    }
    guard_.bpm_ = nullptr;
    guard_.page_ = nullptr;
    guard_.is_dirty_ = false;
}

ReadPageGuard::~ReadPageGuard() {
    this->guard_.Drop();
}  // NOLINT

// Write
WritePageGuard::WritePageGuard(WritePageGuard &&that) noexcept : guard_(std::move(that.guard_)){};

auto WritePageGuard::operator=(WritePageGuard &&that) noexcept -> WritePageGuard & {
    if(&that == this){
        return *this;
    }
    this->guard_ = std::move(that.guard_);
    return *this; 
}

void WritePageGuard::Drop() {
    if(guard_.bpm_ != nullptr){
        guard_.bpm_->UnpinPage(guard_.PageId(), guard_.is_dirty_);
    }
    if(guard_.page_ != nullptr){
        guard_.page_->WUnlatch();
    }
    guard_.bpm_ = nullptr;
    guard_.page_ = nullptr;
    guard_.is_dirty_ = false;
}

WritePageGuard::~WritePageGuard() {
    this->Drop();
}  // NOLINT

}  // namespace bustub
