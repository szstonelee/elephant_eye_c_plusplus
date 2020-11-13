// reference <The Art of Multiprocessor Programming>

#pragma once

#include <iostream>
#include <vector>
#include <mutex>

#include "atomic_flag_reference.h"

namespace sss {

template<class T>
class LockFreeSetLinkList {

private:

struct Node {
  T key;
  FlagReference<Node> next;

  Node(const T& k) : key(k), next(nullptr, false) {}
};

public:
  LockFreeSetLinkList() : head_(new Node(T())), tail_(new Node(T())), size_(0) {
    head_->next.set_ref(tail_);
  }

  // when dtor(), it needs the app to guarantee no thread moidifying the internal data structure,
  // e.g. calling add() or remove() or find()
  // because it could violate the integrity, like
  // 1. delete the node for multi times 
  // 2. can miss some just added nodes which leads to memory leakage
  // it also make those nodes being freed but concurrent thread try to visit them, i.e. dangling potiner
  ~LockFreeSetLinkList() noexcept {
    delete_all_nodes();

    head_->key.~T();
    tail_->key.~T();
    delete head_;
    delete tail_;
  }

  LockFreeSetLinkList(const LockFreeSetLinkList&) = delete;
  LockFreeSetLinkList& operator=(const LockFreeSetLinkList& rhs) = delete;

// because find() is the base private member function, we show here even find() is a private member function

private:
  // find() will locate the key at [pred, curr] from scope [head_, tail_] where 
  // 1. curr is less than or equal to the key,
  // 2. pred -> curr
  // 3. because the nodes in linked list is sorted and distinct, curr > pred
  // 4. neither pred nor curr is not marked as deleted from the eye of the thread
  // 
  // NOTE: tail_ is bigger than any key, so at last curr could be tail_
  // 
  // In the traverse path, if find() found any node marked as deleted, i.e., logically deleted, 
  // it will unlink the node, i.e., phisically delete
  // when phisically delete the node, it could be success or fail
  // if fail, it means another thread could just phisically delete at the same time, so retry agin from [head_, tail_]
  // if success, it uses atomic operatation to unlink, so it is thread-safe and can go on
  // 
  // in theory, when fail and retry from head_ again for enough times, find() will success in the end, though it could
  // 1. fail many times if unlucky, i.e. too many concurrent threads are deleting too many nodes
  // 2. is not wait-free because of 1
  // 
  // for debug reason, we set a threashhold for retry times, check kMaxTryCount
  // 
  // find() guarantee that [pred, curr] is not logically deleted from the eye of the caller thread
  // but in concurrent environment, other threads could logically mark pred or curr to be deleted at the same time
  std::tuple<Node*, Node*> find(const T& key) {
    int try_fail_cnt = 0;
    Node* pred;
    Node* curr;

    while (try_fail_cnt <= kMaxTryCount) {  // if kMaxTryCount == INT_MAX, it is like while (true)
      if (traverse_with_unlink(key, pred, curr)) {
        return {pred, curr};
      } else {
        ++try_fail_cnt;
      }
    }

    std::cerr << "too many failure when unlink in find(), fail times = " << try_fail_cnt << '\n';
    exit(-1);
  } 

  // From head_ we traverse with unlink the logically delete nodes
  // After locating the [pred, curr], return true.
  // If unlink failed, return false 
  bool traverse_with_unlink(const T& key, Node*& pred, Node*& curr) {
    pred = head_;
    curr = pred->next.get_ref();

    while (curr != tail_) {
      auto [succ, curr_logic_delete] = curr->next.get();

      if (curr_logic_delete) {
        if (!try_unlink(pred, curr, succ))
          return false;

        // curr has been unlinked successfully, 
        // so curr needs to be the next one and test logical deleted again
        curr = succ;   

      } else {
        if (curr == tail_ || curr->key >= key) 
          break;
        
        pred = curr;
        curr = succ;
      }
    }

    return true;
  }

  // try to phisically delete the node curr, i.e. unlink.
  // before unlink, curr is logically deleted (and could has been unlinked at the same time by another thread), 
  //                i.e. flag is true, which is guaranteed by the caller 
  // pred is the node before curr, succ is the node after curr from the caller, NOTE: it may be an illusion
  // use atomic compare_and_set, i.e., CAS, to try to unlink
  // if fail, it means one of the scenarios happens
  // 1. another concurrent thread unlink in the same time, so curr has been unlinked, i.e., pred not -> curr
  // 2. some nodes has been added in [pred, curr], 
  //    e.g. remove() or find() call to here with concurrent add() in [pred, curr] by another thread
  //         NOTE: the timing of another thread calling add() successfully needs to 
  //               be earlier than the one of the thread() call set_flag() in remove()
  // 3. pred has been logically or physically deleted
  // 
  // when fail, we return false, and the caller will try loop again from the head_, 
  // and it will prevent 1-3 happen repeatly
  //     for 1, curr is unlinked, so can not locate the same curr again
  //     for 2, the pred will be changed
  //     for 3, the loop, i.e., find(), will remove pred first
  // 
  // NOTE1: succ could be logically deleted concurrently when we make pred point to succ, but it is OK with the integrity.
  //   e.g. in remove(), before we call set_flag(), 
  //        succ is logically deleted but not physicall deleted concurrently by another thread.
  //        then in remove(), we get succ by calling curr->next.get_ref(); then call in to try_unlink()
  //        if we can continue, unlink will be sucessful for curr. So pred->succ, but now succ is logically deleted
  //        it is OK for integrity. check integrity: pred->succ(logically deleted) and curr is unlinked
  //        but succ can not be physically deleted. check NOTE 2
  // 
  // NOTE2: curr must point to succ, i.e., no new nodes can be added between [curr, succ], or unlink curr -> succ
  //        because curr is logically deleted first, then algorithm get the succ from curr->next.
  //        it will guaratee that 
  //        1. make add() to fail to insert nodes between [pred, curr]
  //        2. make remove() to fail to remove succ 
  bool try_unlink(Node* const pred, Node* const curr, Node* const succ) noexcept {
    assert(curr->next.get_flag());  // must be in logically delete. NOTE: all algorithm guarantee 0->1 for flag, no reverse
    
    const bool unlink_success = pred->next.compare_and_set(curr, succ, false, false);

    if (unlink_success) {
      // TODO: gc
      // because only one thread can call CAS successfully, it is safe to recycle the node
      // we can add it to gc queue for which 
      // the thread of guarbage collector will delete it later and safely
      // we can not delete curr right now, because some concurrent thread maybe visit it at the same time 

      // here we try use mutex for a queue to save the unlinked node
      // mutex is not lock free, but new, delete is not lock-free too
      // considering the unlink operation is less than other, the possibility of contention would be low
      // we will reclaim the memory in lock-free-set-link-list dtor(), so memory consumption need to be considered
      // another un-safe but effiecient way is tag a timestamp to unlinked node 
      // and recycle the nodes which last a enough safe long time (like seconds or minutes)

      std::lock_guard<std::mutex> guard(gc_mutex_);
      gc_nodes_.push_back(curr);      

      assert(size_.load() > 0);
      --size_;  
    }

    return unlink_success; 
  }

public:
  // For add(), we first call find() to locate [pred, curr]
  // if curr is the key, return false, i.e. we can not add duplicated key in the set
  // 
  // otherwise, we can try to add the key between [pred, curr]
  // For the concurrent situations, we need compare and set, i.e. CAS, to guarantee
  // 1. pred -> curr
  // 2. pred and curr is not mark as deleted
  // 
  // if CAS failed, it means one of the following senarios happens
  // 1. at the same time, another thread has added a same key node between [pred, curr]
  // 2. pred has been mark as deleted
  // 3. pred not point to curr, one or more nodes have been added between [pred, curr], it could be the same key as 1 
  // 4. pred not point to curr, because curr has been unlinked, phisically removed
  // 
  // NOTE1: curr could be marked as deleted, i.e., logically deleted
  // but we can go on for CAS, there are two possibilities after the CAS
  // 1. another thread unlinked curr successfully by its CAS in find(), so here CAS in add() will fail
  // 2. the add() thread calls CAS successfully, so pred->new_node->curr finishes, which will make another thread CAS in find() fail.
  //    So in 2, it guarantees that when pred->new_node->curr finished, 
  //    the concurrent thread of find() can not make pred -> curr.next successfully 
  // 
  // NOTE2: pred could be physically deleted, unlink from the list
  //        in this situation, pred' flag must be true because the algorithm guarantee logically deletion happen before physically deletion
  //        it will make CAS in add() fail so we can repeat try from the head_ by find()
  //
  // if CAS failed, we will repeat to try again  
  bool add(const T& key) {
    int try_fail_cnt = 0;

    while (try_fail_cnt <= kMaxTryCount) {  // if kMaxTryCount == INT_MAX, it is like while (true)
      auto [pred, curr] = find(key);

      if (curr->key == key)
        return false;

      if (try_add(key, pred, curr)) {
        ++size_;
        return true;
      } else {
        ++try_fail_cnt;
      }
    }

    std::cerr << "too many failure when try_add in add(), fail times = " << try_fail_cnt << '\n';
    exit(-1);
  }

  // For remove(), we first call find() to locate [pred, curr]
  // if curr is not the key, return false, i.e., we must find the key first
  // 
  // otherwise, we will try to use CAS to unlink curr
  // 
  // if CAS failed, it means one of the scenarios happens
  // 1. curr has been unlinked by another concurrent thread, it could be another thread call remove() or find()
  // 2. pred not point to curr, because some nodes added before curr, this must happend before curr->next.set_flag(true);
  // 3. pred not point to curr, because pred is logically deleted, some concurrent thread call remove but locate for pred will make it happen
  // 4. pred not point to curr, because pred is unlinked. 
  // 
  // if CAS fail, we will retry by starting by find() which start from head_
  bool remove(const T& key) {
    int try_fail_cnt = 0;

    while (try_fail_cnt <= kMaxTryCount) {  // if kMaxTryCount == INT_MAX, it is like while (true)
      auto [pred, curr] = find(key);

      if (curr->key != key)
        return false;
    
      curr->next.set_flag(true);    // flag curr to be logically deleted
      auto* succ = curr->next.get_ref();

      if (try_unlink(pred, curr, succ)) {
        return true;
      } else {
        ++try_fail_cnt;
      }
    }

    std::cerr << "too many failure when try_unlink in remove(), fail times = " << try_fail_cnt << '\n';
    exit(-1);
  }

  // contains traverse like find() but do no unlink work
  // this way it guaratees wait-free buecause no CAS
  bool contains(const T& key) const {
    auto* curr = head_->next.get_ref();

    while (curr != tail_) {
      if (curr->key >= key) 
        break;

      curr = curr->next.get_ref();
    }

    return curr != tail_ && curr->key == key && !curr->next.get_flag();   // curr can not be mark as deleted
  }

  int size() {
    return size_;
  }

  void debug_print_whole_nodes() {
    const auto* n = head_->next.get_ref();
    while (n != tail_) {
      auto flag = n->next.get_flag();
      std::cout << n->key << "(" << (flag?"+":"-") << ")->";    // + means positive or deleted
      n = n->next.get_ref();
    }
    std::cout << "(nullptr of tail)\n";
  }

  // nth start from 1
  void debug_set_flag(const int nth, const bool flag) {
    auto* n = head_->next.get_ref();
    int cnt = 0;
    while (n != tail_) {
      ++cnt;
      if (cnt == nth) {
        n->next.set_flag(flag);
      }
      n = n->next.get_ref();
    }
  }

  bool debug_find(const T& key) {
    auto [pred, curr] = find(key);
    if (curr != tail_ && curr->key == key) {
      return true;
    } else {
      return false;
    }    
  }

  void debug_print() {
    std::cout << "Lock free set link list debug report. NOTE: if concurrent, the following data would look strange!\n";
    std::cout << "size = " << size_ << '\n';
    debug_print_whole_nodes();
  }

private:
  bool try_add(const T& key, Node* const pred, Node* const curr) {
    assert(curr == tail_ || curr->key > key);
    assert(pred == head_ || head_->key < key);

    auto* new_node = new Node(key);

    if (try_link(new_node, pred, curr)) {
      return true;
    } else {
      delete new_node;
      return false;
    }
  }

  bool try_link(Node* const new_node, Node* const pred, Node* const curr) {
    assert(new_node->next.get_flag() == false);

    new_node->next.set_ref(curr);

    const bool success = pred->next.compare_and_set(curr, new_node, false, false);
    return success;
  }

  void gc_clear() {
    std::lock_guard<std::mutex> guard(gc_mutex_);
    for (const auto node : gc_nodes_) {
      delete node;
    }
  }

  void delete_all_nodes() {
    gc_clear();

    auto* curr = head_->next.get_ref();
    while (curr != tail_) {
      const auto next_ref = curr->next.get_ref();
      delete curr;  
      curr = next_ref;
    }    
  }

  Node* head_;  // sentinel virtual pointer, which key is less than any nodes
  Node* tail_;  // sentinel virtual pointer, which key is greater than any nodes
  std::atomic<int> size_;  // only count nodes exclude unlink nodes, i.e., if logically deleted, they will be counted into size_
  const int kMaxTryCount = INT_MAX; // if you want disable the threashhold, set it to be INT_MAX

  std::mutex gc_mutex_;
  std::vector<Node*> gc_nodes_; 
};

} // namespace sss
