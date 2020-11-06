
#include <vector>
#include <iostream>
#include <random>
#include <algorithm>
#include <cassert>

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

  VectSkipSet& operator=(const VectSkipSet&) = delete;

  bool empty() const {
    if (count_ > 0) 
      assert(level_ > 0);
    else 
      assert(level_ == 0);

    return count_ == 0;
  }

  bool contains(const T& key) const {
    const Node* curr = head_;
    for (int i = level_-1; i >= 0; --i)
    {
      while (curr->next[i] && node_min_key(curr->next[i]) < key) {
        curr = curr->next[i];
      }
    }
    
    const auto* const no_less = curr->next[0];
    if (no_less && node_min_key(no_less) == key) {
      return true;
    } else {
      // exclude no_less, now check curr
      if (curr == head_) {
        return false;
      } else {
        return exist_key(curr, key);
      }
    }
  }

  bool insert(const T& key) {
    Node* preds[kMaxLevel];
    auto [curr, no_less] = locate_curr_and_no_less(key, preds);

    if (exist_in_curr_or_no_less(key, curr, no_less))
      return false;

    // now the key is distinct, in the scope [curr, no_less]
    if (is_full(curr) && is_full(no_less)) {
      // need a new node
      auto new_node_key = key;
      if (curr && node_max_key(curr) > new_node_key) {
        std::swap(new_node_key, curr->keys[curr->iMax]);
        update_imax(curr);
      }
      insert_new_node(preds, std::move(new_node_key));
    } else {
      // no need to insert a new ndoe, insert the key to either curr or no_less
      if (!is_full(curr)) {
        insert_any_key(key, curr);
      } else {
        assert(!is_full(no_less));
        auto no_less_key = key;
        if (curr != head_ && node_max_key(curr) > no_less_key) {
          std::swap(no_less_key, curr->keys[curr->iMax]);
          update_imax(curr);
        }
        insert_min_key(std::move(no_less_key), no_less);
      }
    }

    ++count_;
    return true;
  }

  bool erase(const T& key) {
    Node* preds[kMaxLevel];
    auto [curr, no_less] = locate_curr_and_no_less(key, preds);

    if (!exist_in_curr_or_no_less(key, curr, no_less))
      return false;

    // now key in either curr or no_less, we need to delete it
    if (no_less && key == node_min_key(no_less)) {
      // key in no_less node
      if (is_single_key_node(no_less)) {
        delete_node(preds, no_less);
      } else {
        delete_key_from_node(key, no_less);
      }
    } else {
      // key in curr node
      assert(exist_key(curr, key));
      if (is_single_key_node(curr)) {
        delete_node(preds, curr);
      } else {
        delete_key_from_node(key, curr);
      }
    }

    --count_;
    return true;
  }

  int count() const {
    return count_;
  }

  void test_print_node(Node* node) {
    std::cout << "key size = " << node->keys.size() << ": (";
    for (const auto& key : node->keys) {
      std::cout << key << ", ";
    }
    std::cout << " )";
  }

  void test_print_whole_nodes() {
    int index = 0;
    auto* node = head_;
    while (node) {
      if (node == head_) {
        std::cout << "head node: \n";
      } else {
        std::cout << "node " << index << ": " << "min_key = " << node_min_key(node) << ", max_key = " << node_max_key(node) << ": ";
        test_print_node(node);
        std::cout << '\n';
      }
      node = node->next[0];
      ++index;
    }
  }

  void test_insert(const std::vector<T>& keys) {
    for (int i = 0, sz = keys.size(); i < sz; ++i) {
      insert(keys[i]);
      std::cout << "insert i = " << i << ", count = " << count() <<  ", level = " << level_ << '\n';
      test_print_whole_nodes();
    }
  }

  bool test_key_in_vector(const T& key, const std::vector<T>& keys) {
    for (const auto& k : keys) {
      if (k == key) return true;
    }
    return false;
  }

  // each key in delete keys must be distinct 
  void test_erase(const std::vector<T>& insert_keys, const std::vector<T>& delete_keys) {
    for (const auto& key : insert_keys) {
      assert(insert(key));
    }

    for (const auto& key : delete_keys) {
      if (test_key_in_vector(key, insert_keys)) {
        assert(erase(key));
      } else {
        assert(!erase(key));
      }
      std::cout << "after delete key = " << key << ", whole nodes are like \n";
      test_print_whole_nodes();
    }
  }

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
  bool is_single_key_node(Node* node) const {
    assert(node && node != head_);
    return node->keys.size() == 1;
  }

  // iterate skip list's levels from top to bottom, locate the exact [curr, no_less] scope in level 0
  // where no_less's min key is equal or bigger than the key
  // if no_less is nullptr, it means the node with the virtual absolute max key
  // preds will store the previous nodes for each level
  std::tuple<Node*, Node*> locate_curr_and_no_less(const T& key, Node* preds[]) const {
    std::memset(preds, 0, kMaxLevel*sizeof(Node*));

    auto* curr = head_;
    for (int i = level_-1; i >= 0; --i) {
      while(curr->next[i] && node_min_key(curr->next[i]) < key) {
        curr = curr->next[i];
      }
      preds[i] = curr;
    }

    auto* no_less = curr->next[0];

    return {curr, no_less};
  }

  bool exist_in_curr_or_no_less(const T& key, const Node* const curr, const Node* const no_less) const {
    if (no_less && node_min_key(no_less) == key)
      return true;   // key in the no_less node

    if (curr != head_ && exist_key(curr, key))
      return true;   // key in the curr node

    return false;
  }

  void insert_new_node(Node* preds[], T&& key) {
    const int lvl = random_level();
    if (lvl > level_) {
      for (int i = level_; i < lvl; i++) {
        preds[i] = head_;
      }
      level_ = lvl;
    }

    auto* const new_node = create_node(lvl, std::forward<T>(key));
    for (int i = 0; i < lvl; i++) {
      new_node->next[i] = preds[i]->next[i];
      preds[i]->next[i] = new_node;
    }
  }

  void delete_node(Node* preds[], Node* to_delete) {
    assert(to_delete && to_delete != head_);

    for (int i = 0; i < level_; i++) {
      if (preds[i]->next[i] != to_delete)
        break;
      preds[i]->next[i] = to_delete->next[i];
    }
    while (level_ > 0 && head_->next[level_] == nullptr)
      --level_;
  }

  // guarantee the key is distinct and less than the min key of the node
  void insert_min_key(T&& key, Node* const node) const {
    assert(node && node != head_ && node->keys.size() < kCapacity);
    assert(!exist_key(node, key) && key < node_min_key(node));

    node->iMin = node->keys.size();
    node->keys.push_back(key);
  }

  // guarantee the key is distinct and greater than the min key of the node 
  void insert_any_key(const T& key, Node* const node) const {
    assert(node && node != head_ && node->keys.size() < kCapacity);
    assert(!exist_key(node, key) && key > node_min_key(node));

    if (key > node_max_key(node))
      node->iMax = node->keys.size();
    
    node->keys.push_back(key);
  }

  void delete_key_from_node(const T& key, Node* node) const {
    assert(node && node != head_ && node->keys.size() > 1);

    int index = -1;
    for (int i = 0, sz = node->keys.size(); i < sz; ++i) {
      if (key == node->keys[i]) {
        index = i;
        break;
      }
    }
    assert(index != -1);

    node->keys.erase(node->keys.begin()+index);

    if (index != node->iMin && index != node->iMax) {
      if (node->iMin > index) 
        --node->iMin;
      if (node->iMax > index)
        --node->iMax;
    } else {
      // the deleted key is the min or max key, we need update iMin and/or iMax
      update_imin_imax(node);
    }
  }

  // If node is the head_ or node is nullptr, it means the node can not accept new keys, 
  // so return true. Otherwise, check the capacity of the node
  bool is_full(const Node* const node) const {
    if (node == head_ || node == nullptr) {
      return true;
    } else {
      assert(node->keys.size() <= kCapacity);
      return node->keys.size() == kCapacity;
    }
  }

  // update imax of the node after the max key has been changed
  // O(N) for vector but quick enough
  void update_imax(Node* const node) const {
    assert(node && node != head_ && !node->keys.empty());

    int new_imax = 0;
    for (int i = 1, sz = node->keys.size(); i < sz; ++i) {
      assert(node->keys[i] != node->keys[new_imax]);
      if (node->keys[i] > node->keys[new_imax]) {
        new_imax = i;
      }
    }
    if (node->iMax != new_imax) // NOTE: new_imax could be same with the previous iMax
      node->iMax = new_imax;  // less write for better performance
  }

  void update_imin_imax(Node* const node) const {
    assert(node && node != head_ && !node->keys.empty());

    int new_imin = 0;
    int new_imax = 0;
    for (int i = 1, sz = node->keys.size(); i < sz; ++i) {
      assert(node->keys[i] != node->keys[new_imin]);
      assert(node->keys[i] != node->keys[new_imax]);

      if (node->keys[i] > node->keys[new_imax]) {
        new_imax = i;
      }
      if (node->keys[i] < node->keys[new_imin]) {
        new_imin = i;
      }
    }
    if (node->iMin != new_imin)
      node->iMin = new_imin;
    if (node->iMax != new_imax)
      node->iMax = new_imax;
  }

  // O(N) for vector but quick enough
  bool exist_key(const Node* const node, const T& to_find) const {
    assert(node && node != head_);

    for (const auto& key : node->keys) {
      if (key == to_find) return true;
    }
    return false;
  }

  T node_min_key(const Node* const node) const {
    assert(!node->keys.empty());
    assert(node->iMin >= 0 && node->iMin < node->keys.size());
    return node->keys[node->iMin];
  }

  T node_max_key(const Node* const node) const {
    assert(!node->keys.empty());
    assert(node->iMax >= 0 && node->iMax < node->keys.size());
    return node->keys[node->iMax];
  }

  Node* create_node(const int level, const T& key) const {
    auto copy = key;
    return create_node(level, std::move(copy));
  }

  Node* create_node(const int level, T&& first_key) const {
    assert(level > 0 && level <= kMaxLevel);

    void* new_mem = std::malloc(sizeof(Node) + (level-1)*sizeof(Node*));
    Node* const new_node = static_cast<Node*>(new_mem);
 
    new (&new_node->keys) std::vector<T>();
    new_node->keys.push_back(first_key);
    new_node->iMin = 0;
    new_node->iMax = 0;
    for (int i = 0; i < level; ++i) {
      new_node->next[i] = nullptr;
    }

    return new_node;
  }

  // for debug, the node needs to empty all keys before destroy 
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
  const int kCapacity = 16;
};

} // namespace sss

void test_shuffle() {
  std::srand(time(0));

  std::random_device rd;
  std::mt19937 g(rd());

  constexpr int num_elements = 2 << 20;
  std::vector<int> elements(num_elements);
  std::vector<int> to_deletes;
  to_deletes.reserve(num_elements/2);
  std::vector<int> to_searchs;
  to_searchs.reserve(num_elements/2);
  for (int j=0; j < num_elements; ++j) {
    elements[j] = j;
    if (rand()%2 == 0) {
      to_deletes.push_back(j);
    } else {
      to_searchs.push_back(j);
    }
  }
  std::shuffle(elements.begin(), elements.end(), g);

  sss::VectSkipSet<int> vss;
  for (const auto element : elements) {
    auto res = vss.insert(element);
    assert(res);
  }

  for (const auto item : to_deletes) {
    auto res = vss.erase(item);
    assert(res);
  }

  for (const auto item : to_searchs) {
    auto res = vss.contains(item);
    assert(res);
  }

  std::cout << "vss count = " << vss.count() << ", to_searchs size = " << to_searchs.size() << '\n';
}

int main() {
  // sss::VectSkipSet<int> vss;

  // std::vector<int> keys = {4, 2, 19, 7, 14, 3, 8, 5, 6, 9, 10, 11, 1, 12};
  // vss.test_insert(keys);

  // std::vector<int> insert_keys = {1, 2, 3, 4, 5, 6, 7};
  // std::vector<int> delete_keys = {4, 1, 5, 6};
  // vss.test_erase(insert_keys, delete_keys);

  test_shuffle();
  return 0;
}