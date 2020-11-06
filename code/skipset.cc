#include <random>

#include "skipset.h"

namespace sss { // sss is simple skip set or single-threaded skip set

template<class T>
SkipSet<T>::SkipSet(): head_(nullptr), level_(0), count_(0) {
    head_ = create_node(kMaxLevel, T());
}

template<class T>
SkipSet<T>::~SkipSet() noexcept {
  auto* node = head_;
  while (node)
  {
    auto* to_destroy = node;
    node = node->next[0];
    destroy_node(to_destroy);
  }
}

template<class T>
bool SkipSet<T>::empty() const {
  if (count_ > 0) 
    assert(level_ > 0);
  else 
    assert(level_ == 0);

  return count_ == 0;
}

template<class T>
bool SkipSet<T>::contains(const T& value) const {
  return find(value) != end();
}

template<class T>
bool SkipSet<T>::insert(const T& value) {
  Node* preds[kMaxLevel];
  std::memset(preds, 0, kMaxLevel*sizeof(Node*));

  auto* node = head_;
  for (int i = level_-1; i >= 0; i--) 
  {
    while (node->next[i] && node->next[i]->value < value) {
      node = node->next[i];
    }
    preds[i] = node; 
  }
    
  const auto* const find = node->next[0];
  if (find && find->value == value)
    return false;

  // need insert
  int lvl = random_level();
  assert(lvl > 0 && lvl <= kMaxLevel);
  if (lvl > level_) 
  {
    for (int i = level_; i < lvl; i++) {
      preds[i] = head_;
    }
    level_ = lvl;
  }

  auto* const new_node = create_node(lvl, value);
  for (int i = 0; i < lvl; i++) {
    new_node->next[i] = preds[i]->next[i];
    preds[i]->next[i] = new_node;
  }

  ++count_;
  return true;            
}

template<class T>
bool SkipSet<T>::erase(const T& value) {
  Node* preds[kMaxLevel];
  std::memset(preds, 0, sizeof(Node*)*(kMaxLevel));

  Node* node = head_;  
  for (int i = level_-1; i >= 0; i--) 
  {
    while (node->next[i] && node->next[i]->value < value) {
      node = node->next[i];
    }
    preds[i] = node; 
  }
        
  auto* const find = node->next[0];
  if (!(find && find->value == value))
    return false;

  // can be eraaed
  for (int i = 0; i < level_; i++) {
    if (preds[i]->next[i] != find)
      break;
    preds[i]->next[i] = find->next[i];
  }
  while (level_ > 0 && head_->next[level_] == nullptr)
    --level_;

  destroy_node(find);
  --count_;
  return true;
}

template<class T>
typename SkipSet<T>::Iterator SkipSet<T>::begin() const {
  return Iterator(head_->next[0]);
}

template<class T>
typename sss::SkipSet<T>::Iterator sss::SkipSet<T>::end() const {
  return Iterator(nullptr);
}

template<class T>
typename sss::SkipSet<T>::Iterator sss::SkipSet<T>::find(const T& value) const {
  const Node* node = head_;
  for (int i = level_-1; i >= 0; --i)
  {
    while (node->next[i] && node->next[i]->value < value) {
      node = node->next[i];
    }
  }
    
  const auto* const find = node->next[0];
  if (find && find->value == value) {
    return Iterator(find);
  } else {
    return end();
  }
}

template<class T>
typename SkipSet<T>::Node* SkipSet<T>::create_node(const int level, const T& new_value) const {
  auto copy = new_value;
  return create_node(level, std::move(copy));
}

template<class T>
typename SkipSet<T>::Node* SkipSet<T>::create_node(const int level, T&& new_value) const {
  assert(level > 0 && level <= kMaxLevel);

  void* node_mem = std::malloc(sizeof(Node)+(level-1)*sizeof(Node*));
  Node* new_node = static_cast<Node*>(node_mem);
  new (&new_node->value) T(new_value);
  for (int i = 0; i < level; ++i) {
    new_node->next[i] = nullptr;
  }

  return new_node;
}

template<class T>
void SkipSet<T>::destroy_node(Node* node) const noexcept {
  node->value.~T();
  std::free(node);
}

// return rand level in [1, kMaxLevel]
template<class T>
int SkipSet<T>::random_level() const {
  int lvl = 1;
  if (kProbability == 0.5) {
    while (rand() % 2 == 0 && lvl < kMaxLevel) {
      ++lvl;
    }
  } else {
    while ((static_cast<float>(rand()) / RAND_MAX) < kProbability && lvl < kMaxLevel) {
      ++lvl;
    }
  }
  return lvl;
}

} // namespace simple skip set

