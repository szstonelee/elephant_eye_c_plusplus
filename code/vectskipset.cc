
#include <vector>
#include <iostream>

namespace sss {

template<class T>
class VectSkipSet {
private:
  struct Node {
    std::vector<T> keys;
    int iMin;
    int iMax;
    Node* next[1];
  };

public:
  VectSkipSet() : head_(nullptr), level_(0), count_(0) {
    head_ = create_node(kMaxLevel, T());
  }

  ~VectSkipSet() noexcept {
    auto* node = head_;
    while (node) {
      auto* to_destroy = node;
      node = node->next[0];

      to_destroy->keys.clear();
      destroy_node(to_destroy);
    }
  }

  bool insert(const T& key) {
    Node* preds[kMaxLevel];
    std::memset(preds, 0, kMaxLevel*sizeof(Node*));

    auto* curr = head_;
    for (int i = level_-1; i >= 0; --i) {
      while(curr->next[i] && curr->next[i]->node_min_key() < key) {
        curr = curr->next[i];
      }
      preds[i] = curr;
    }

    auto* no_less = curr->next[0];
    if (no_less && no_less->node_min_key() == key)
      return false;   // next node, i,e., no_less, has the key

    // now no_less is absolutely bigger than the key. NOTE: no_less could be nullptr

    if (curr != head_ && exist_key(curr, key))
      return false;   // key in the vector keys of curr node
    
    if (is_full(curr) && is_full(no_less)) {
      // need insert new node
      auto in_new_node_key = key;
      if (curr && curr->node_max_key() > key) {
        in_new_node_key = curr->node_max_key();
        update_imax(curr);
      }
      insert_new_node(preds, std::move(in_new_node_key));
    } else {
      // just insert the key to the non-full node
      if (!is_full(curr)) {
        // prefer to insert key to curr first for debug purpose
        insert_any_key(key, curr);
      } else {
        assert(!is_full(no_less));
        insert_min_key(key, no_less);
      }
    }

    ++count_;
    return true;
  }

  void insert_new_node(Node* preds[], T&& key) {
    int lvl = random_level();
    if (lvl > level_) {
      for (int i = level_; i < lvl; i++) {
        preds[i] = head_;
      }
      level_ = lvl;
    }

    auto* const new_node = create_node(lvl, key);
    for (int i = 0; i < lvl; i++) {
      new_node->next[i] = preds[i]->next[i];
      preds[i]->next[i] = new_node;
    }
  }

  // guarantee the key is the smallest one
  void insert_min_key(const T& key, Node* node) {
    assert(node && node != head_ && node->keys.size() < kCapacity);
    assert(key < node->node_min_key());

    node->iMin = node->keys.size();
    node->keys.push_back(key);
  }

  // guarantee the key is distinct and greater than the min key of the node 
  void insert_any_key(const T& key, Node* node) {
    assert(node && node != head_ && node->keys.size() < kCapacity);
    assert(key > node->node_min_key());
    assert(!exist_key(node, key));

    if (key > node->node_max_key())
      node->iMax = node->keys.size();
    
    node->keys.push_back(key);
  }

  // NOTE: if node is the head_ or node is nullptr, it means the node can not be inserted new keys
  bool is_full(Node* node) {
    if (node == head_ || node == nullptr) {
      return true;
    } else {
      return node->keys.size() == kCapacity;
    }
  }

/*
  // insert the key from the start_node until the end, i.e., each node must meet the capacity
  // if every nodes including the start_node and the following ones are full capacity, 
  // i.e, all overflow, return false and set the overflow, which is the last overflow of the biggest key
  // otherwise, return true. The value of overflow is useless.
  bool insert_key_until_end(const T& key, Node* start_node, T& overflow) {
    assert(start_node && start_node != head_);

    auto* node = start_node;
    auto new_key = key;
    while (node) {
      if (try_insert_key(node, new_key, overflow) {
        return true;
      } else {
        node = node->next[0];
        new_key = overflow;
      }
    }
    return false;
  }

  // insert the key which should be the smallest key in the node
  // if under capacity, return true, update the iMin
  // if reach capacity, return false, the max key of the node will be popped to the overflow, and update the iMax 
  bool try_insert_key(Node* node, const T& key, T& overflow) {
    assert(key < node_min_key(node) && node->keys.size() <= kCapacity);

    if (node->keys.size() == kCapacity) {
      // overflow, pop the max key
      overflow = node_max_key(node);
      node->keys[node->iMax] = key;
      node->iMin = node->iMax;
      update_imax(node);
      return false;
    } else {
      // under capacity, can insert the key in the node
      node->keys.push_back(key);
      node->iMin = node->keys.size() - 1;
      return true;
    }
  }

  // O(n) to update imax of the node, but for vector, it is quick
  void update_imax(Node* node) {
    int new_imax = 0;
    for (int i = 1, sz = node->keys.size(); i < sz; ++i) {
      assert(node->keys[i] != node->keys[new_imax]);
      if (node->keys[i] > node->keys[new_imax]) {
        new_imax = i;
      }
    }
    assert(new_imax != node->imax);
    node->imax = new_imax;
  }
*/

  void test_create_node(T v) {
    constexpr int level = 3;
    auto* node = create_node(level, v);

    std::cout << "size of keys = " << node->keys.size() << '\n';
    std::cout << "print keys: ";
    for (auto key : node->keys) {
      std::cout << key << ", ";
    }
    std::cout << '\n';
    std::cout << "iMin = " << node->iMin << ", iMax = " << node->iMax << '\n';

    std::cout << "print pointers: ";
    for (int i = 0; i < level; ++i) {
      std::cout << node->next[i] << ", ";
    }
    std::cout << '\n';
  }

  void test_destroy_node(T v) {
    auto* node = create_node(3, v);
    node->keys.clear();
    destroy_node(node);
  }

private:
  bool exist_key(Node* node, const T& to_find) const {
    assert(node && node != head_);
    for (const auto& key : node->keys) {
      if (key == to_find) return true;
    }
    return false;
  }

  T& node_min_key(Node* node) const {
    assert(!node->keys.empty());
    assert(node->iMin >= 0 && node->iMin < node->keys.size());
    return node->keys[node->iMin];
  }

  T& node_max_key(Node* node) const {
    assert(!node->keys.empty());
    assert(node->iMax >= 0 && node->iMax < node->keys.size());
    return node->keys[node->iMax];
  }

  Node* create_node(const int level, const T& new_value) const {
    auto copy = new_value;
    return create_node(level, std::move(copy));
  }

  Node* create_node(const int level, T&& new_key) const {
    assert(level > 0 && level <= kMaxLevel);

    void* new_mem = std::malloc(sizeof(Node) + (level-1)*sizeof(Node*));
    Node* new_node = static_cast<Node*>(new_mem);
 
    new (&new_node->keys) std::vector<T>();
    new_node->keys.push_back(new_key);
    new_node->iMin = 0;
    new_node->iMax = 0;
    for (int i = 0; i < level; ++i) {
      new_node->next[i] = nullptr;
    }

    return new_node;
  }

  void destroy_node(Node* node) const noexcept {
    assert(node->keys.empty());

    node->keys.~vector();
    std::free(node);
  }

  // return rand level in [1, kMaxLevel]
  int random_level() const {
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

  Node* head_;
  int level_;
  int count_;

  const int kMaxLevel = 32;
  const float kProbability = 0.5;
  const int kCapacity = 128;
};

} // namespace sss

int main() {
  sss::VectSkipSet<int> vss;

  vss.test_create_node(100);
  vss.test_destroy_node(200);

  return 0;
}