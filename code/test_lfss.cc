#include <cassert>

#include "lock_free_skip_set.h"

int main() {
  sss::LockFreeSkipSet<int> lfss;

  for (int i = 1; i <= 20; ++i)
    lfss.add(i);  
  // lfss.debug_print();

  for (int i = 0; i < 11; ++i)
    lfss.remove(i);
  // lfss.debug_print();

  lfss.debug_print();
  std::cout << "contains 10 is " << (lfss.contains(10) ? "true" : "false") << '\n';

  return 0;
}