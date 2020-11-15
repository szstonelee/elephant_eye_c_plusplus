// some code from https://stackoverflow.com/questions/31580869/skip-lists-are-they-really-performing-as-good-as-pugh-paper-claim

#pragma once

#include <random>
#include <vector>
#include <unordered_set>

namespace sss { // sss is simple skip set or single-threaded skip set

template <class T>
class ASkipSet
{
private:
  struct Node
  {
    T key;
    // NOTE: we will allocate memory in place after value for contigoous layout, 
    // check create_node() & destroy_node()
    Node* next[1];   
  };

  class Iterator {
  public:
    explicit Iterator(const Node* n) : curr_(n) {}

    void operator++() {
      assert(curr_);
      curr_ = curr_->next[0];
    }

    bool operator==(const Iterator& it) const {
      return curr_ == it.curr_;
    }

    bool operator!=(const Iterator& it) const {
      return !(*this == it);
    }

    T operator*() const {
      return curr_->key;
    }

  private:
    const Node* curr_;
  };

public:
  ASkipSet() : head_(nullptr), height_(0), count_(0), modify_count_(0), threshold_(0), ascope_(0), search_index_(-1LL) {
    head_ = create_node(kMaxHeight, T());
    std::srand(std::time(0));
    search_keys_ = std::vector<T>(kSearchKeySize);
  }

  ~ASkipSet() noexcept {
    auto* node = head_->next[0];
    while (node)
    {
      auto* to_destroy = node;
      node = node->next[0];
      destroy_node(to_destroy);
    }
    // NOTE: destory head without calling destroy_node is for debug purpose
    head_->key.~T();
    std::free(head_);
  }

  ASkipSet(const ASkipSet&) = delete;
  ASkipSet& operator=(const ASkipSet&) = delete;

  bool empty() const {
    if (count_ > 0) 
      assert(height_ > 0);
    else 
      assert(height_ == 0);

    return count_ == 0;
  }

  bool insert(const T& key) {
    if (!insert_internal(key))
      return false;

    ++count_;
    check_adjust_trigger(key);
    return true;
  }

  bool erase(const T& key) {
    if (!erase_internal(key))
      return false;

    --count_;
    check_adjust_trigger(key);
    return true;
  }

  Iterator begin() const {
    return Iterator(head_->next[0]);
  }

  Iterator end() const {
    return Iterator(nullptr);
  }

  Iterator find(const T& key) {
    auto* const find = locate_node(key);
    if (find && find->key == key) {
      ++search_index_;
      search_keys_[search_index_%kSearchKeySize] = key;
      return Iterator(find);
    } else {
      return end();
    }
  }

  bool contains(const T& key) {
   return find(key) != end();   
  }

  // if threashold <= 0, disable trigger adjust
  // if ascope <= 0, it means all nodes except head_ will be reconstruted
  void set_threashold(const int threshold, const int ascope) {
    threshold_ = threshold;
    ascope_ = ascope;
  }

private:
  Node* locate_node(const T& key) const {
    const Node* node = head_;
    for (int level = height_-1; level >= 0; --level) {
      while (node->next[level] && node->next[level]->key < key) {
        node = node->next[level];
      }
    }
    return node->next[0];
  }

  bool insert_internal(const T& key) {
    Node* preds[kMaxHeight];
    std::memset(preds, 0, kMaxHeight*sizeof(Node*));

    auto* node = head_;
    for (int level = height_-1; level >= 0; level--) 
    {
      while (node->next[level] && node->next[level]->key < key) {
        node = node->next[level];
      }
      preds[level] = node; 
    }
      
    const auto* const find = node->next[0];
    if (find && find->key == key)
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

    auto* const new_node = create_node(new_height, key);
    for (int level = 0; level < new_height; level++) {
      new_node->next[level] = preds[level]->next[level];
      preds[level]->next[level] = new_node;
    }

    return true;            
  }

  bool erase_internal(const T& key) {
    Node* preds[kMaxHeight];
    std::memset(preds, 0, sizeof(Node*)*(kMaxHeight));

    Node* node = head_;  
    for (int level = height_-1; level >= 0; level--) 
    {
      while (node->next[level] && node->next[level]->key < key) {
        node = node->next[level];
      }
      preds[level] = node; 
    }
        
    auto* const find = node->next[0];
    if (!(find && find->key == key))
      return false;

    // can be eraaed
    for (int level = 0; level < height_; level++) {
      if (preds[level]->next[level] != find)
        break;
      preds[level]->next[level] = find->next[level];
    }
    while (height_ > 0 && head_->next[height_-1] == nullptr)
      --height_;

    destroy_node(find);

    return true;  
  }

  Node* create_node(const int height, const T& new_key) const {
    auto copy = new_key;
    return create_node(height, std::move(copy));
  }

  Node* create_node(const int height, T&& new_key)  const {
    assert(height > 0 && height <= kMaxHeight);

    void* node_mem = std::malloc(sizeof(Node)+(height-1)*sizeof(Node*));
    assert(node_mem);
    
    Node* new_node = static_cast<Node*>(node_mem);
    assert(node_mem == reinterpret_cast<void*>(new_node));

    new (&new_node->key) T(new_key);
    for (int level = 0; level < height; ++level) {
      new_node->next[level] = nullptr;
    }

    return new_node;
  }

  void destroy_node(Node* node) const noexcept {
    assert(node != head_);    // avoid wrong destroy head_, head_ will be freed in dtor()
    
    node->key.~T();
    std::free(node);
  }

  int random_height() const {
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

  T get_adjust_start_key(const T& key) {
    assert(count_ > 0);

    T start_key;
    if (search_index_ == -1) {
      start_key = key;
    } else {
      // if we have scan history keys, use the saved scan keys
      assert(search_index_ >= 0);
      start_key = search_keys_[search_index_%kSearchKeySize]; // use the up-to-date scan key

      if (search_index_ >= kSearchKeySize)
        ++search_index_;  // next one if search keys is full
      else
        search_index_ = 0;  // from the first one if search key is not full
    }
    return start_key;
  }

  void check_adjust_trigger(const T& key) {
    if (threshold_ <= 0) return;  // if disable, do nothing

    ++modify_count_;
    if (modify_count_ < 0) modify_count_ = 0;

    if (modify_count_ < threshold_) return;
    if (count_ == 0) return;

    // do adjust
    modify_count_ = 0;

    // first choose the start key
    auto start_key = get_adjust_start_key(key);

    adjust_memory(start_key);
  }

  // from the start_key, NOTE: start_key may not exist, reconstru keys by memory allocation for contiguous purpose
  void adjust_memory(const T& start_key) {
    assert(count_ > 0);

    Node* node = locate_node(start_key);  // node is node with the key or the first bigger than the key 
    const int total = ascope_ <= 0 ? count_ : std::min(count_, ascope_);
    assert(total > 0);
    std::vector<T> all_keys;
    all_keys.reserve(total);
    for (int i = 0; i < total; ++i) {
      if (node == nullptr)
        node = head_->next[0];
      all_keys.push_back(node->key);
      node = node->next[0];
    }
   
    // batch erase for memory reallocation, otherwise, 
    // if one erase followed by one insert, the memory allocation is not contiguous
    for (const auto& key : all_keys) {
      bool erase_res = erase_internal(key);
      assert(erase_res);
    }

    for (const auto& key : all_keys) {
      bool insert_res = insert_internal(key);
      assert(insert_res);
    }  
  }

private:
  Node* head_;
  int height_;
  int count_;

  int modify_count_;
  int threshold_;   // how many modify trigger the memory adjust
  int ascope_;      // the highest number of keys to be adjusted, if <= 0, it means ervery key

  std::vector<T> search_keys_;
  long long search_index_;

  const int kMaxHeight = 32;
  const float kProbability = 0.5;
  const int kSearchKeySize = 64;
};


} // namespace sss

