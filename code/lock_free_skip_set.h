#pragma once

#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>

#include "atomic_flag_reference.h"

namespace sss {

template <class T>
class LockFreeSkipSet {

private:
  // we could optimize struct Node.nexts with contigous memory right here, 
  // we use std::vector for simplicity, and vector uses a pointer to contigous memory layout
  struct Node {
    T key;
    std::vector<sss::FlagReference<Node>> nexts; 
  
    Node(const T& k, const int height) : key(k), 
                                         nexts(height, FlagReference<Node>(nullptr, false)) {
      assert(height > 0);
    }
  };

public:
  class Iterator {
  public:
    Iterator(Node* node) : curr_(node) {
      assert(node != nullptr);
    }
    ~Iterator() noexcept = default;
    Iterator(const Iterator& copy) = default;
    Iterator& operator=(const Iterator& rhs) = default;

    Iterator& operator++() {
      bool finish = false;
      while (!finish) {
        curr_ = curr_->nexts[0].get_ref();
        if (!curr_->nexts[0].get_flag())
          finish = true;
      }
      return *this;
    }

    T operator*() const {
      return curr_->key;
    }

    bool operator==(const Iterator& rhs) const {
      return curr_ == rhs.curr_;
    }

    bool operator!=(const Iterator& rhs) const {
      return !((*this) == rhs);
    }
    
    private:
      Node* curr_;  
    };

public:
  LockFreeSkipSet() : size_(0) {
    head_ = new Node(T(), kMaxHeight);
    tail_ = new Node(T(), kMaxHeight);

    for (int level = 0; level < kMaxHeight; ++level) {
      head_->nexts[level].set_ref(tail_);  // link head to tail in ctor()
    }

    std::srand(std::time(0));
  }

  ~LockFreeSkipSet() noexcept {
    delete head_;
    delete tail_;
  }

  LockFreeSkipSet(const LockFreeSkipSet&) = delete;
  LockFreeSkipSet& operator=(const LockFreeSkipSet&) = delete;

  int size() const {
    assert(size_.load() >= 0);
    return size_.load();
  }

  bool empty() const {
    return size() == 0;
  }

// read find() first even it is a private function
private:    
  // From top, i.e. level = KMaxLevl-1, to bottom, i.e., level = 0, travere each level in constant steps.
  // For each level, start from curr, which is in the next level of pred->nexts. 
  // Traverse the whol level to get [pred, curr], where curr is the node which key is less than or equal to the key and pred->curr
  // In the traverse path, if we meet any curr which is logically deleted, unlink it, i.e., physically deletion.
  // 
  // unlink using CAS could fail, if fail, restart again from the top level which is kMaxHeight-1, 
  // the reaseons for unlink failure are one of
  // 1. The to-be-unlinked-curr has been unlinked by another concurrent thread. 
  //    So restart will aovid that it happens again.
  // 2. The pred has been deleted by other thread, either logically or physically. 
  //    So restart will aovid the unlinked pred or unlink pred first.
  // 3. pred not->curr, because new nodes could be added by other threads which make pred->new_added_node. 
  //    So restart will avoid that pred->curr happens.
  // 
  // In theory, restart again will eventually success, but for debug reason, we set a failure count threshold
  // 
  // If success, save all preds and currs for each level to preds and succs as output 
  // so it guarantees that 
  // 1. each preds[i] is larger than the key
  // 2. each succs[i] is less than or greater than the key
  // 3. preds[i]->succs[i] at the time of visit them (but could be violated after that time)
  // 
  // preds[i], succs[i] could be logicall deleted or physically after exit or before exit of find()
  // 
  // return true if the key is found, false if the key is not found and gurarantee that 
  // 1. The check node for the key is from level0. 
  // 2. At least at one point of time in find(), the level0 node is not deleted, logically or physically
  bool find(const T& key, Node* preds[], Node* succs[]) {
    int try_fail_cnt = 0;

    while (try_fail_cnt <= kMaxTryCount) {  // if kMaxTryCount == INT_MAX, it is while (true)
      Node* pred = head_;   // restart point including first time
      bool restart = false;

      for (int level = kMaxHeight-1; level >= 0; --level) {
        Node* curr = pred->nexts[level].get_ref();  // pred->curr in level
        if (!traverse_with_unlink(level, key, pred, curr)) {
          ++try_fail_cnt;
          restart = true;
          break;    // restart again
        }
        preds[level] = pred;
        succs[level] = curr;
      }

      if (!restart)
        return succs[0] != tail_ && succs[0]->key == key;
    }

    std::cerr << "too many failure when find(), fail times = " << try_fail_cnt << '\n';
    exit(-1);
  }

// the public interfaces
public:
  // For add() interface, 
  // first, call find() to get all preds and succs and check whether the key is already there.
  // If the key already exists, return false.
  // 
  // Otherwise we can try to add a new_node and try to link it to the level0.
  // 
  // If level0 success, it means the new_node in the set, and we can go on for other higher levels.
  // Linking for other levels are absolutelly successful even we use CAS. Check link_other_levels().
  // 
  // If level0 fails, one of the following scenarios exists:
  //   1. the same key in another new node has benn linked in level0 at the same time by another concurrent thread
  //   2. pred in level 0 could be deleted, logically or physically, by another thread
  //   3. pred0->succ0 has been inserted other nodes by other threads, so pred0 not-> succ0
  // we do not know which one is the reason, so just repeat find() again to exclude reason 1
  bool add(const T& key) {
    int try_level_0_cnt = 0;
    int top_height = random_height();

    while (try_level_0_cnt <= kMaxTryCount) {
      Node* preds[kMaxHeight];
      Node* succs[kMaxHeight];
      bool found = find(key, preds, succs);

      if (found)
        return false;

      // try to add new_node
      Node* new_node = new Node(key, top_height);

      // NOTE: DON NOT DELETE THE FOLLOWING COMMENT !!!!!!!
      // first we prepare the links for new_node
      // I think the preparation step is not neccesary so I comment them 
      // and the <<The Art of Multiprocessor Programming>> Edition 1 may have bug 
      /*
      for (int level = 0; level < top_height; ++level) {
        Node* succ = succs[level];
        new_node->nexts[level].set_ref(succ);
      }
      */

      // link level 0 first
      if (!try_link(0, new_node, preds[0], succs[0])) {
        // link new_node to level 0 failed, we need to repeat find() all over
        // including refresh preds & succs, for a brand nrew new_node and abandon current new_node
        ++try_level_0_cnt;
        delete new_node;

      } else {
        // We have linked new_node to level 0 successfully.
        // It means the key is in the set. Now we will go on to link for other levels.
        link_other_levels(new_node, top_height, preds, succs);
        ++size_;
        return true;
      }
    }

    std::cerr << "too many failure when add() for try_level_0_cnt, fail times = " << try_level_0_cnt << '\n';
    exit(-1);
  }

  // remove() is set deleted flag, i.e. logically deleted, from top to bottom. 
  // 
  // NOTE the flipping operation may be run at the same time by another thread.
  // But for the following guaratees:
  //    1. The direction for setting flags is from top to bottom
  //    2. Flag can only be set fram false to true, i.e., no deleted -> logically deleted
  // so it does not matter and can supoort concurrent operations and does not violate the property of skip set.
  // 
  // But the trick is that only one thread can flip the level0's flag from false to true,
  // because we need to return true or flase which indicated which thread is the first thread (and only thread) to flip the level0 flag
  // So we need to use CAS in try_flag_in_level0().
  //
  // If the thread is the one who flip the level0 flag, it call find() again to unlink it.
  bool remove(const T& key) {
    Node* preds[kMaxHeight];
    Node* succs[kMaxHeight];
    const bool found = find(key, preds, succs);

    if (!found)
      return false;

    auto* const to_remove_node = succs[0];

    set_other_levels_flags(to_remove_node);

    if (try_flag_in_level0(to_remove_node)) {
      find(key, preds, succs);  // unlink
      return true;
    } else {
      return false;
    }
  }

  // contains traverse like find() but do no unlink work
  // this way it guaratees wait-free buecause no CAS
  // NOTE: an optimization compared to the book. 
  // If we found a node's key equal to the key and is not marked as deleted, we found it
  // because the level0 node must not be marked as deleted. remove() guarantee this by mark from top to bottom.
  bool contains(const T& key) const {
    Node* pred = head_;

    for (int level = kMaxHeight-1; level >= 0; --level) {   
      Node* curr = pred->nexts[level].get_ref();

      bool level_finish = false;
      while (!level_finish) {
        bool curr_delete = curr->nexts[level].get_flag();
        if (curr_delete) {
          curr = curr->nexts[level].get_ref();  // because tail_ is not mark as deleted, we can have an end

        } else {
          if (curr == tail_ || curr->key >= key) {
            level_finish = true;  // for next level
          } else {
            pred = curr;
            curr = pred->nexts[level].get_ref();
          }
        }
      }

      if (curr != tail_ && curr->key == key)
        return true;
    }

    return false;
  }

  Iterator locate(const T& key) {
    Node* preds[kMaxHeight];
    Node* succs[kMaxHeight];

    if (!find(key, preds, succs)) {
      return Iterator(tail_);
    } else {
      return Iterator(succs[0]);
    }
  }

  Iterator begin() {
    return Iterator(head_[0]->nexts[0].get_ref());
  }

  Iterator end() {
    return Iterator(tail_);
  }

  void debug_print() const {
    const int sz = size();
    const int ht = debug_height();
    std::cout << "nodes size = " << sz <<  ", height = " << ht << '\n';
    auto grid = debug_grid();
    debug_print_grid(grid);
  }

private:
  std::string debug_pointer(const int level, Node* const node) const {
    if (node == nullptr) {
      return "nullptr";
    } else if (node == head_ || node == tail_) {
      return node == head_ ? "head ptr" : "tail ptr";
    } else {
      std::string res;
      res += "key = " + std::to_string(node->key);
      res += ", ";
      res += "level = ";
      res += std::to_string(level);
      res += ", ";
      res +=  (node->nexts[level].get_flag() ? "-" : "+");
      res += ", height = ";
      res += std::to_string(node->nexts.size());
      return res;
    }
  }

  void debug_print_grid(const std::vector<std::vector<Node*>>& grid) const {
    const int row_cnt = grid.size();
    const int col_cnt = grid[0].size();
    constexpr int w = 5;

    for (int r = 0; r < row_cnt; ++r) {
      for (int c = 0; c < col_cnt; ++c) {
        Node* curr = grid[r][c];
        if (curr == nullptr) {
          std::cout << std::setw(w) << "->";
        } else if (curr == head_) {
          std::cout << std::setw(w) << "head";
        } else if (curr == tail_) {
          std::cout << std::setw(w) << "tail";
        } else {
          if (curr->nexts[row_cnt-r-1].get_flag()) {
            std::cout << std::setw(w-1) << curr->key;
            std::cout << "d";
          } else {
            std::cout << std::setw(w) << curr->key;
          }
        }
      }
      std::cout << '\n';
    }
  }

  std::vector<std::vector<Node*>> debug_grid() const {
    const int col_cnt = size()+2;
    const int row_cnt = debug_height();
    std::vector<std::vector<Node*>> grid(row_cnt, std::vector<Node*>(col_cnt, nullptr));
    Node* curr = head_;
    for (int c = 0; c < col_cnt; ++c) {
      if (curr == head_) {
        for (int r = 0; r < row_cnt; ++r) 
          grid[r][0] = head_;
      } else if (curr == tail_) {
        for (int r = 0; r < row_cnt; ++r) 
          grid[r][col_cnt-1] = tail_;
      } else {
        for (int r = 0, ht = curr->nexts.size(); r < ht; ++r) 
          grid[row_cnt-r-1][c] = curr;        
      }

      curr = curr->nexts[0].get_ref();
    }
    assert(curr == nullptr);
    return grid;
  }

  int debug_height() const {
    int max_height = 0;
    Node* n = head_->nexts[0].get_ref();
    while (n != tail_) {
      max_height = std::max(max_height, static_cast<int>(n->nexts.size()));
      n = n->nexts[0].get_ref();
    }
    return max_height;
  }

private:
  // Traverse in one level, starting from pred->curr which is guaranteed by caller find().
  // 
  // When success, return true. It means:
  // Locate [pred, curr] where curr is bigger or equal to the key and pred->curr.
  // It will guarantee that:
  //    1. pred->curr
  //       NOTE: other concurrent thread could change it after the exectuing thread 
  //             which guarantee only at the executing time but the time does not mean the exit of the function 
  //    2. pred < key 
  //       NOTE: if pred is head_, it is less than anything
  //    3. curr >= key
  //       NOTE: if curr is tail_, it is bigger than anything
  // 
  // If CAS fails, return false
  // 
  // you can referecne lock_free_set_link_list.h for the similiar function
  bool traverse_with_unlink(const int level, const T& key, Node*& pred, Node*& curr) {
    assert(pred == head_ || pred->key < key);

    while (curr != tail_) { 
      // NOTE: tail_ can not be deleted and tail_ does not point to next object and tail_'s key is bigger than anyone
      auto [succ, curr_logic_delete] = curr->nexts[level].get();

      if (curr_logic_delete) {
        if (!try_unlink(level, pred, curr, succ)) // try to unlink curr, if success, pred->succ in specific level 
          return false;

        curr = succ;  // curr is unlinked, then set a new curr where pred->curr

      } else {
        if (curr == tail_ || curr->key >= key)
          break;  // we found [pred, curr]
        
        pred = curr;
        curr = succ;  // pred and curr both move where pred->curr
      }
    }

    return true; 
  }

  // We just need to make the to_remove_node logically deleted,
  // so it means the to_remove_node will be unlinked in futrue, eaving it for other threads using find().
  // If to_remove_node is marked as deleted,
  // The trick is that how to determine which thread set the flag because multi threads could call it concurrently.
  // So we use CAS to judge which thread is the first one (and only one) for flipping the flag.
  bool try_flag_in_level0(Node* const to_remove_node) const {
    int try_fail_cnt = 0;
    Node* const succ = to_remove_node[0].nexts[0].get_ref();  

    while (try_fail_cnt <= kMaxTryCount) {
      const auto first_thread = to_remove_node->nexts[0].compare_and_set(succ, succ, false, true);
      if (first_thread) {
        return true;  // This thread is the first one to set logically deleted, i.e., flipping flag from false->true.
      } else {
        ++try_fail_cnt;
        // maybe to_remove_node has already been set logically deleted by other threads, we need test it
        const auto already_deleted = to_remove_node->nexts[0].get_flag();
        if (already_deleted)
          return false;   
        // else we will try again because compare_and_set() can not guarantee only one time of success 
        // even the condition is true, i.e., right now, to_remove_node->succ and flag of to_remove_node is false.
        // Check the documents of C++ atomic about compare_exchange_weak()
      }
    }

    std::cerr << "too many failure when try_flag_logically_delete(), fail times = " << try_fail_cnt << '\n';
    exit(-1);
  }

  void set_other_levels_flags(Node* const to_remove_node) const {
    const auto height = to_remove_node->nexts.size();

    for (int level = height-1; level > 0; --level) {
      to_remove_node->nexts[level].set_flag(true);
    }
  }

  // Link other levels for the new_node by caller add(). There are the following important guarantees
  // 
  // 1. The level0 has benn linked successfully by the current thread. check add().
  // 
  // 2. Linkage of levels of [1, top_height) is bottom-up 
  //            and must be sucessful one level after another 
  //            and only be executed by the current thread
  //    because only one thread (that is me) can link successfully in level0 with the new_node
  //            and go on for the linking job of other levels for the new_node.
  //    NOTE: When the linkage finishs, some of the new_node->nexts[0~top_height) could be marked as deleted.
  //          It means two nodes with the same key could exist in the same level other than level0.
  //          e.g. in level1, node(5)->node(5).
  //          But the previous one is absolutely marked as deleted so it does not violate the skip set property
  //    
  // I will give an example for the details of the garantee 2. 
  // 
  //     Thread0 tries to add node_0(5). Here 5 denotes that the key is 5 and 0 denotes that the thread is 0.
  //     It succesfully links in level0. 
  //     The guarantee is that only one node with value 5 in level0 exists, regardless of it is logically deleted or not.
  //     Check add() and find() for the guarantee or read lock_free_set_link_list for more help.
  // 
  //     Then thread0 is trying to link in level1.
  //     At the same time the node_0(5) is marked as deleted by thread1 in remove() 
  //     and is unlinked by thread2 in find() in level0.
  //     So thread3 can add the same key of 5 which is named as node_3(5). 
  //     Thread3 links all level (e.g., level0 and level1) succesfully before thread0 finishs level1.
  // 
  //     Then thread0 comes to uses CAS to link in level1. 
  //     Thread0 will fail for the first time because pred right now points to node_3(5) where it has no idea.
  //     But after find() in thread0 for retry, the succ will be refreshed to be the node_3(5). 
  //     NOTE: Even the retried find() in thread 0 return [preds, succs] which may have nothing about the node_0(5),  
  //           but it does not matter.
  // 
  //     This time thread0 links successfully in level1 using CAS. 
  //     And level1 has two same key nodes, node_0(5)->node_3(5).
  //     But node_0(5) must be marked as deleted otherwise node_3(5) can not be linked in level0 and go on for level1.
  //     It guarantees no violation for the following properties of skip set.
  //        1. For level 0, each node is distinct.
  //        2. For other levels, all nodes which are not marked as deleted are distinct. 
  //           It could exist two nodes with the same key in the same level, but the previous node is marked as deleted.
  //        Could more than two nodes with the same key exist in the heigher level? No!
  //  
  //     Let us see another scenario.
  //       If thread0 linkage in level1 is successful before thread3 links node_3(5) in level1, what happens?
  //       node_0(5) is marked as deleted and exist in level1. 
  //       When thread3 tries to link node_3(5) and we assume that thread3 does not know the node_0(5) existence in level1, 
  //       it will fail for the first time in link_other_levels(). 
  //       Then it will retry with find(), which will unlink node_0(5) in level1. 
  //       After the unlink, thread3 will link node_3(5) succesfully in level1. 
  //       Now level1 only has one node for 5, i.e. node_3(5).
  //
  // The key points are described as the following:
  //     1. In remove() the to-remove-node are marked as deleted (logically) from top to bottom. 
  //     2. In level0 it guarantees distinction. One time, only one node of distinct key exists in level0.
  void link_other_levels(Node* const new_node, const int top_height, Node* preds[], Node* succs[]) {
    int try_fail_cnt = 0;

    for (int level = 1; level < top_height; ++level) {
      while (try_fail_cnt <= kMaxTryCount) {
        Node* pred = preds[level];  // restart point
        Node* succ = succs[level];

        if (try_link(level, new_node, pred, succ)) {
          break;  // for next level
        } else {
          ++try_fail_cnt;
          find(new_node->key, preds, succs);    // refresh preds and succs and restart
        }
      }
    }

    if (try_fail_cnt > kMaxTryCount) {
      std::cerr << "too many failure when link_other_levels(), fail times = " << try_fail_cnt << '\n';
      exit(-1);
    }
  }

  // link new_node to [pred, curr] in the level
  // NOTE1: new_node could be marked as delete in some levels and the key of new_node could be same as curr
  // NOTE2: For other levels, try_link() can only be called by one thread for one new_node,
  //        so it is thread-safe for new_node->nexts[level].set_ref(curr).
  bool try_link(const int level, Node* const new_node, Node* const pred, Node* const curr) const {
    new_node->nexts[level].set_ref(curr); 

    const bool success = pred->nexts[level].compare_and_set(curr, new_node, false, false);
    return success;
  }

  // check lock_free_set_link_list.h for more info
  // unlink curr when pred->curr and pred is not flagged as deleted, set pred->succ if successful
  // when level == 0, recycle the node
  bool try_unlink(const int level, Node* const pred, Node* const curr, Node* const succ) noexcept {
    // must be in logically delete. NOTE: all algorithm guarantee 0->1 for flag, no reverse
    assert(curr != head_ && curr != tail_ && curr->nexts[level].get_flag());  
    
    const bool unlink_success = pred->nexts[level].compare_and_set(curr, succ, false, false);

    if (unlink_success && level == 0) {
      // TODO: gc
      assert(size_.load() > 0);
      --size_;  
    }

    return unlink_success; 
  }

  // return rand height in [1, kMaxHeight], i.e. for level, it is [0, kMaxHeight)
  int random_height() const {
    int lvl = 1;
    if (kProbability == 0.5) {
      while (rand() % 2 == 0 && lvl < kMaxHeight) {
        ++lvl;
      }
    } else {
      while ((static_cast<float>(rand()) / RAND_MAX) < kProbability && lvl < kMaxHeight) {
        ++lvl;
      }
    }
    return lvl;
  }

private:
  Node* head_;
  Node* tail_;
  std::atomic<int> size_;

  const int kMaxHeight = 32;
  const float kProbability = 0.5;
  const int kMaxTryCount = INT_MAX;

};


}