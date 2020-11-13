#include "lock_free_set_link_list.h"

void add_3_2_1() {
  sss::LockFreeSetLinkList<int> lfsll;
  lfsll.add(3);
  lfsll.add(1);
  lfsll.add(3);
  lfsll.debug_print();
  lfsll.add(2);
  lfsll.debug_print();
}

void test_find_and_contain_with_concurrent_flag() {
  sss::LockFreeSetLinkList<int> lfsll;
  lfsll.add(1);
  lfsll.add(2);
  lfsll.add(3);
  lfsll.debug_print();
  std::cout << "print find() and contains before set flag\n";
  std::cout << "contains 2 result = " << (lfsll.contains(2) ? "true" : "false") << '\n';
  std::cout << "find 2 result = " << (lfsll.debug_find(2) ? "true" : "false") << '\n';
  // simulate a concurrent thread set 
  std::cout << "simulate to concurrently set flag\n";
  lfsll.debug_set_flag(2, true);
  lfsll.debug_print();
  std::cout << "contains 2 result = " << (lfsll.contains(2) ? "true" : "false") << '\n';
  std::cout << "before find() call\n";
  lfsll.debug_print();
  std::cout << "find 2 result = " << (lfsll.debug_find(2) ? "true" : "false") << '\n';
  std::cout << "after find() call\n";
  lfsll.debug_print();
}

void test_find_remove_multi_nodes() {
  sss::LockFreeSetLinkList<int> lfsll;
  lfsll.add(1);
  lfsll.add(2);
  lfsll.add(3);
  lfsll.add(4);
  lfsll.debug_print();
  lfsll.debug_set_flag(2, true);
  lfsll.debug_set_flag(4, true);
  std::cout << "find 4 result = " << (lfsll.debug_find(4) ? "true" : "false") << '\n';
  lfsll.debug_print();
}

// For MacOS use leaks tool which needs install XCode
// leaks --atExit -- ./a.out
void test_memory_leak() {
  sss::LockFreeSetLinkList<int> lfsll;  
  lfsll.add(1);
  lfsll.add(2);
  lfsll.remove(2);
}

int main() {
  add_3_2_1();

  test_find_and_contain_with_concurrent_flag();

  test_find_remove_multi_nodes();

  test_memory_leak();

  return 0;
}