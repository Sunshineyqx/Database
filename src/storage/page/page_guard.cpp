#include "storage/page/page_guard.h"
#include <cstddef>
#include "buffer/buffer_pool_manager.h"
// make page_guard_test -j8
// ./test/page_guard_test
namespace bustub {
// Basic
BasicPageGuard::BasicPageGuard(BasicPageGuard &&that) noexcept
    : bpm_(that.bpm_), page_(that.page_), is_dirty_(that.is_dirty_) {
  that.bpm_ = nullptr;
  that.page_ = nullptr;
  that.is_dirty_ = false;
}

void BasicPageGuard::Drop() {
  if (bpm_ != nullptr) {
    bpm_->UnpinPage(this->PageId(), this->is_dirty_);
  }
  this->bpm_ = nullptr;
  this->page_ = nullptr;
  this->is_dirty_ = false;
}

auto BasicPageGuard::operator=(BasicPageGuard &&that) noexcept -> BasicPageGuard & {
  if (&that == this) {
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

BasicPageGuard::~BasicPageGuard() { this->Drop(); };  // NOLINT

// Read
ReadPageGuard::ReadPageGuard(ReadPageGuard &&that) noexcept : guard_(std::move(that.guard_)) {}

auto ReadPageGuard::operator=(ReadPageGuard &&that) noexcept -> ReadPageGuard & {
  if (&that == this) {
    return *this;
  }
  this->Drop();
  this->guard_ = std::move(that.guard_);
  return *this;
}

void ReadPageGuard::Drop() {
  if (guard_.page_ != nullptr) {
    guard_.page_->RUnlatch();
  }
  this->guard_.Drop();
}

ReadPageGuard::~ReadPageGuard() { this->Drop(); }  // NOLINT

// Write
WritePageGuard::WritePageGuard(WritePageGuard &&that) noexcept : guard_(std::move(that.guard_)) {}

auto WritePageGuard::operator=(WritePageGuard &&that) noexcept -> WritePageGuard & {
  if (&that == this) {
    return *this;
  }
  this->Drop();
  this->guard_ = std::move(that.guard_);
  return *this;
}

void WritePageGuard::Drop() {
  if (guard_.page_ != nullptr) {
    guard_.page_->WUnlatch();
  }
  this->guard_.Drop();
}

WritePageGuard::~WritePageGuard() { this->Drop(); }  // NOLINT

}  // namespace bustub
