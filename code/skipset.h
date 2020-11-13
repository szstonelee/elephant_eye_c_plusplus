// some code from https://stackoverflow.com/questions/31580869/skip-lists-are-they-really-performing-as-good-as-pugh-paper-claim

#pragma once

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
  SkipSet();
  ~SkipSet() noexcept;
  SkipSet(const SkipSet&) = delete;
  SkipSet& operator=(const SkipSet&) = delete;

  bool empty() const;
  bool insert(const T& value);
  bool erase(const T& value);
  Iterator begin() const;
  Iterator end() const;
  Iterator find(const T& value) const;
  bool contains(const T& value) const;

private:
  Node* create_node(const int level, const T& new_value) const;
  Node* create_node(const int level, T&& new_value) const;
  void destroy_node(Node* node) const noexcept;
  int random_height() const;
  
private:
  Node* head_;
  int height_;
  int count_;

  const int kMaxHeight = 32;
  const float kProbability = 0.5;
};


} // namespace sss

