#include <random>

#include "skipset.h"

namespace sss { // sss is simple skip set or single-threaded skip set

template<class T>
SkipSet<T>::SkipSet(): head_(nullptr), height_(0), count_(0) {
    head_ = create_node(kMaxHeight, T());
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
    assert(height_ > 0);
  else 
    assert(height_ == 0);

  return count_ == 0;
}

template<class T>
bool SkipSet<T>::contains(const T& value) const {
  return find(value) != end();
}

template<class T>
bool SkipSet<T>::insert(const T& value) {
  Node* preds[kMaxHeight];
  std::memset(preds, 0, kMaxHeight*sizeof(Node*));

  auto* node = head_;
  for (int level = height_-1; level >= 0; level--) 
  {
    while (node->next[level] && node->next[level]->value < value) {
      node = node->next[level];
    }
    preds[level] = node; 
  }
    
  const auto* const find = node->next[0];
  if (find && find->value == value)
    return false;

  // need insert
  int new_height = random_height();
  assert(new_height > 0 && new_height <= kMaxHeight);
  if (new_height > height_) 
  {
    for (int level = height_; level < new_height; level++) {
      preds[level] = head_;
    }
    height_ = new_height;
  }

  auto* const new_node = create_node(new_height, value);
  for (int level = 0; level < new_height; level++) {
    new_node->next[level] = preds[level]->next[level];
    preds[level]->next[level] = new_node;
  }

  ++count_;
  return true;            
}

template<class T>
bool SkipSet<T>::erase(const T& value) {
  Node* preds[kMaxHeight];
  std::memset(preds, 0, sizeof(Node*)*(kMaxHeight));

  Node* node = head_;  
  for (int level = height_-1; level >= 0; level--) 
  {
    while (node->next[level] && node->next[level]->value < value) {
      node = node->next[level];
    }
    preds[level] = node; 
  }
        
  auto* const find = node->next[0];
  if (!(find && find->value == value))
    return false;

  // can be eraaed
  for (int level = 0; level < height_; level++) {
    if (preds[level]->next[level] != find)
      break;
    preds[level]->next[level] = find->next[level];
  }
  while (height_ > 0 && head_->next[height_] == nullptr)
    --height_;

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
  for (int level = height_-1; level >= 0; --level)
  {
    while (node->next[level] && node->next[level]->value < value) {
      node = node->next[level];
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
typename SkipSet<T>::Node* SkipSet<T>::create_node(const int height, T&& new_value) const {
  assert(height > 0 && height <= kMaxHeight);

  void* node_mem = std::malloc(sizeof(Node)+(height-1)*sizeof(Node*));
  Node* new_node = static_cast<Node*>(node_mem);
  new (&new_node->value) T(new_value);
  for (int level = 0; level < height; ++level) {
    new_node->next[level] = nullptr;
  }

  return new_node;
}

template<class T>
void SkipSet<T>::destroy_node(Node* node) const noexcept {
  node->value.~T();
  std::free(node);
}

// return rand level in [1, kMaxHeight]
template<class T>
int SkipSet<T>::random_height() const {
  int height = 1;
  if (kProbability == 0.5) {
    while (rand() % 2 == 0 && height < kMaxHeight) {
      ++height;
    }
  } else {
    while ((static_cast<float>(rand()) / RAND_MAX) < kProbability && height < kMaxHeight) {
      ++height;
    }
  }
  return height;
}

} // namespace simple skip set

