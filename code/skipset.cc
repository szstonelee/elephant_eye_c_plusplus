// some code from https://stackoverflow.com/questions/31580869/skip-lists-are-they-really-performing-as-good-as-pugh-paper-claim

#include <iostream>
#include <random>
#include <algorithm>
#include <vector>
#include <cassert>
#include <string>
#include <set>

namespace sss { // sss is simple skip set or single-threaded skip set

template <class T>
class SkipSet
{
private:
  struct Node
  {
    T value;
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

  private:
    const Node* curr_;
  };

public:
  SkipSet(): head_(nullptr), level_(0), count_(0) {
    head_ = create_node(kMaxLevel, T());
  }

  ~SkipSet() noexcept {
    auto* node = head_;
    while (node)
    {
      auto* to_destroy = node;
      node = node->next[0];
      destroy_node(to_destroy);
    }
  }

  SkipSet& operator=(const SkipSet&) = delete; 

  bool empty() const {
    if (count_ > 0) 
      assert(level_ > 0);
    else 
      assert(level_ == 0);

    return count_ == 0;
  }

  bool contains(const T& value) const {
    return find(value) != end();
  }

  bool insert(const T& value) {
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

  bool erase(const T& value) {
    Node* preds[kMaxLevel];
    memset(preds, 0, sizeof(Node*)*(kMaxLevel));

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

  Iterator begin() const {
    return Iterator(head_->next[0]);
  }

  Iterator end() const {
    return Iterator(nullptr);
  }

  Iterator find(const T& value) const {
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

private:
  Node* create_node(const int level, const T& new_value) const {
    auto copy = new_value;
    return create_node(level, std::move(copy));
  }

  Node* create_node(const int level, T&& new_value) const {
    assert(level > 0 && level <= kMaxLevel);

    void* node_mem = std::malloc(sizeof(Node)+(level-1)*sizeof(Node*));
    Node* new_node = static_cast<Node*>(node_mem);
    new (&new_node->value) T(new_value);
    for (int i = 0; i < level; ++i) {
      new_node->next[i] = nullptr;
    }

    return new_node;
  }

  void destroy_node(Node* node) const noexcept {
    node->value.~T();
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
};

} // namespace simple skip set

static double sys_time() {
  return static_cast<double>(clock()) / CLOCKS_PER_SEC;
}

template <class T>
bool contains(const std::set<T>& cont, const T& val) {
  return cont.find(val) != cont.end();
}

template <class T>
bool contains(const sss::SkipSet<T>& cont, const T& val) {
  return cont.contains(val);
}

template <class Set, class T>
void benchmark(int num, const std::vector<T>& elements, 
              const std::vector<T>& search_elements) {
  const double start_insert = sys_time();
  Set element_set;
  for (int j=0; j < num; ++j)
    element_set.insert(elements[j]);
  std::cout << "-- Inserted " << num << " elements in " << (sys_time() - start_insert) << " secs\n";

  const double start_search = sys_time();
  int num_found = 0;
  for (int j=0; j < num; ++j)
  {
    if (contains(element_set, search_elements[j]))
      ++num_found;
  }
  std::cout << "-- Found " << num_found << " elements in " << (sys_time() - start_search) << " secs\n";

  const double start_erase = sys_time();
  int num_erased = 0;
  for (int j=0; j < num; ++j)
  {
    if (element_set.erase(search_elements[j]))
      ++num_erased;
  }
  std::cout << "-- Erased " << num_found << " elements in " << (sys_time() - start_erase) << " secs\n";
}

// crud : create, replace, update, delete
void bench_random_crud() {
  std::random_device rd;
  std::mt19937 g(rd());

  const int num_elements = 200000;
  std::vector<int> elements(num_elements);
  for (int j=0; j < num_elements; ++j)
    elements[j] = j;
  std::shuffle(elements.begin(), elements.end(), g);

  std::vector<int> search_elements = elements;
  std::shuffle(search_elements.begin(), search_elements.end(), g);

  using Set1 = std::set<int>;
  using Set2 = sss::SkipSet<int>;

  for (int j=0; j < 2; ++j)
  {
    std::cout << "std::set\n";
    benchmark<Set1>(num_elements, elements, search_elements);
    std::cout << '\n';

    std::cout << "SkipSet\n";
    benchmark<Set2>(num_elements, elements, search_elements);
    std::cout << '\n';
  }
}

void scan_in_random(const int set_sz, const int scope,
                    const std::vector<int>& starts) {
  std::vector<int> elements(set_sz);
  for (int i = 0; i < set_sz; ++i)
    elements[i] = i;

  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(elements.begin(), elements.end(), g);

  sss::SkipSet<int> ss;
  for (const auto element : elements) 
    ss.insert(element);

  int num_total = 0;
  const double start_time = sys_time();
  for (const auto start : starts) {
    auto it = ss.find(start);
    for (int i = 0; i < scope; ++i) {
      if (it == ss.end()) break;

      ++it;
      ++num_total;
    }
  }
  std::cout << "-- Scan in random memory layout, total " << num_total << " in " << (sys_time() - start_time) << " secs\n";
}

void scan_in_contiguous(const int set_sz, const int scope,
                       const std::vector<int>& starts) {
  sss::SkipSet<int> ss;
  for (int i = 0; i < set_sz; ++i)
    ss.insert(i);

  int num_total = 0;
  const double start_time = sys_time();
  for (const auto start : starts) {
    auto it = ss.find(start);
    for (int i = 0; i < scope; ++i) {
      if (it == ss.end()) break;

      ++it;
      ++num_total;
    }
  }
  std::cout << "-- Scan in contigous memory layout, total " << num_total << " in " << (sys_time() - start_time) << " secs\n";
}

void bench_range_scan() {
  constexpr int set_sz = 1 << 23;   // 8 Million
  constexpr int num_rand = 1000;    
  std::vector<int> scan_starts;
  scan_starts.reserve(num_rand);
  for (int i = 0; i < num_rand; ++i) {
    scan_starts.push_back(std::rand()%set_sz);
  }

  constexpr int scope = 1 << 16;    // 64 K
  scan_in_random(set_sz, scope, scan_starts);
  scan_in_contiguous(set_sz, scope, scan_starts);
}

int main()
{
  // bench_random_crud();

  bench_range_scan();

  return 0;
}