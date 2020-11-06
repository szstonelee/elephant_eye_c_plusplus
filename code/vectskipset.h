#pragma once

namespace sss { // simple skip set or single-threaded skip set

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
  // during the time of using this iterator, vector skip list should be not changed
  class ImmuIter {   
  public:
    explicit ImmuIter(const Node* node, const T& key);
    bool operator==(const ImmuIter& it) const;
    bool operator!=(const ImmuIter& it) const;
    void operator++();
    T operator*() const;
    bool end() const;

  private:
    const Node* curr_;  
    int index_;
    std::vector<T> sort_keys_; 
  };

public:
  VectSkipSet();
  ~VectSkipSet() noexcept;
  VectSkipSet& operator=(const VectSkipSet&) = delete;

  bool empty() const;
  bool contains(const T& key) const;
  bool insert(const T& key);
  bool erase(const T& key);
  int count() const;

  ImmuIter find_immutation(const T& key) const;

private:
  bool is_single_key_node(Node* node) const;
  std::tuple<Node*, Node*> locate_curr_and_no_less(const T& key, Node* preds[]) const;
  bool exist_in_curr_or_no_less(const T& key, const Node* const curr, const Node* const no_less) const;
  void insert_new_node(Node* preds[], T&& key);
  void delete_node(Node* preds[], Node* to_delete);
  void insert_min_key(T&& key, Node* const node) const;
  void insert_any_key(const T& key, Node* const node) const;
  void delete_key_from_node(const T& key, Node* node) const;
  bool is_full(const Node* const node) const;
  void update_imax(Node* const node) const;
  void update_imin_imax(Node* const node) const;
  bool exist_key(const Node* const node, const T& to_find) const;
  T node_min_key(const Node* const node) const;
  T node_max_key(const Node* const node) const;
  Node* create_node(const int level, const T& key) const;
  Node* create_node(const int level, T&& first_key) const;
  void destroy_node(Node* node) const noexcept;
  int random_level() const;


public:
  void test_print_node(Node* node);
  void test_print_whole_nodes();
  void test_insert(const std::vector<T>& keys);
  bool test_key_in_vector(const T& key, const std::vector<T>& keys);
  void test_erase(const std::vector<T>& insert_keys, const std::vector<T>& delete_keys);
  void test_create_node(T v);
  void test_destroy_node(T v);


private:
  Node* head_;
  int level_;
  int count_;

  const int kMaxLevel = 32;
  const float kProbability = 0.5;
  const int kCapacity = 64;
};

} // namespace sss