#include "primer/trie.h"
#include <cstddef>
#include <memory>
#include <string_view>
#include "common/exception.h"
#include "primer/trie_store.h"

namespace bustub {

template <class T>
auto Trie::Get(std::string_view key) const -> const T * {
  //空树
  if(root_ == nullptr){
     return nullptr;
  }

  std::shared_ptr<const TrieNode> node = root_;
  //查找key中每一个字符
  for(auto& c : key){
    if(node->children_.find(c) == node->children_.end()){
      return nullptr;
    }
    //继续迭代查找
    node = node->children_.at(c);
  }
  //找到了...使用dynamic_cast验证是不是终端节点
  auto res = dynamic_cast<const TrieNodeWithValue<T> *>(node.get());
  if(res){
    return res->value_.get();
  }
  return nullptr;
  // You should walk through the trie to find the node corresponding to the key. If the node doesn't exist, return
  // nullptr. After you find the node, you should use `dynamic_cast` to cast it to `const TrieNodeWithValue<T> *`. If
  // dynamic_cast returns `nullptr`, it means the type of the value is mismatched, and you should return nullptr.
  // Otherwise, return the value.
}

template <class T>
auto Trie::Put(std::string_view key, T value) const -> Trie {
  // Note that `T` might be a non-copyable type. Always use `std::move` when creating `shared_ptr` on that value.
  Trie new_trie;
  //空串
  if(key.empty()){
    new_trie.root_ = std::make_shared<TrieNodeWithValue<T>>(root_->Clone()->children_, std::make_shared<T>(std::move(value)));
    return new_trie;
  }
 
  //空树
  if(root_ == nullptr){
      auto cur_node = std::make_shared<TrieNode>();
      auto pre_node = cur_node;
      new_trie.root_ = cur_node;
      for(auto& k : key){
      auto next_node = std::make_shared<TrieNode>();
      cur_node->children_[k] = next_node;
      pre_node = cur_node;
      cur_node = next_node;
    }
    auto last_char = key[key.size() - 1];
    auto last_node = std::make_shared<TrieNodeWithValue<T>>(std::make_shared<T>(std::move(value)));
    pre_node->children_[last_char] = last_node;
    return new_trie;
  }

  //正常情况
  auto cur_node = std::shared_ptr<TrieNode>(root_->Clone());
  auto pre_node = cur_node;
  new_trie.root_ = cur_node;
  for(auto& k : key){
    auto& children = cur_node->children_;
    if(children.find(k) != children.end()){ //找到就clone()
      auto next_node = std::shared_ptr<TrieNode>(children[k]->Clone());
      cur_node->children_[k] = next_node;
      pre_node = cur_node;
      cur_node = next_node;
    }
    else{
      auto next_node = std::make_shared<TrieNode>();
      cur_node->children_[k] = next_node;
      pre_node = cur_node;
      cur_node = next_node;
    }
  }
  auto last_char = key[key.size()- 1];
  pre_node->children_[last_char] = std::make_shared<TrieNodeWithValue<T>>(cur_node->children_, std::make_shared<T>(std::move(value)));
  return new_trie;
  // You should walk through the trie and create new nodes if necessary. If the node corresponding to the key already
  // exists, you should create a new `TrieNodeWithValue`.
}

auto Trie::Remove(std::string_view key) const -> Trie {
  throw NotImplementedException("Trie::Remove is not implemented.");

  // You should walk through the trie and remove nodes if necessary. If the node doesn't contain a value any more,
  // you should convert it to `TrieNode`. If a node doesn't have children any more, you should remove it.
}

// Below are explicit instantiation of template functions.
//
// Generally people would write the implementation of template classes and functions in the header file. However, we
// separate the implementation into a .cpp file to make things clearer. In order to make the compiler know the
// implementation of the template functions, we need to explicitly instantiate them here, so that they can be picked up
// by the linker.

template auto Trie::Put(std::string_view key, uint32_t value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const uint32_t *;

template auto Trie::Put(std::string_view key, uint64_t value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const uint64_t *;

template auto Trie::Put(std::string_view key, std::string value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const std::string *;

// If your solution cannot compile for non-copy tests, you can remove the below lines to get partial score.

using Integer = std::unique_ptr<uint32_t>;

template auto Trie::Put(std::string_view key, Integer value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const Integer *;

template auto Trie::Put(std::string_view key, MoveBlocked value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const MoveBlocked *;

}  // namespace bustub
