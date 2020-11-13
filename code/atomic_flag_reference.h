// reference and code from 
// https://stackoverflow.com/questions/40247249/what-is-the-c-11-atomic-library-equivalent-of-javas-atomicmarkablereferencet
// https://gcc.godbolt.org/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAKxAEZTUAHAvVAOwGdK0AbVAV2J4OmAIIcAtiADkABmmk0Exnh6YA8mwDCCAIZtgmGfICUpDgOLIj0gKQAmAMx42yHvywBqW4626iEnjIPti2sqIOzq7uXj5%2BHCLEBAB0CCFhEeEA9Fme/GwEmByF6BmFyjz%2BmHFuugmeACrp4bX1ALK6xADWugBGagBKmABmmMSYrtXhtgDsAEIZjEIAblUgGZ6bnsXoICD%2BqIHBvvwuBMzEAPoEIZ6rPD4L4VvbBP5BnmicBHlnF9eeCR1LreRwAEU8tEeGy2pwK/x%2BSl6LkwEAaACpPONhqRPL1UKgeIDOl0TN55jCXptxgRBGwsZgzmMlpgCJdkHUbic/gQrlzsBBsWTZlpid1oc8trMwRlFvx%2BkF1pLNh1un1BiMxhNrGjMdjcfjCWLSZSqSA7roeBAkSjBSNcUDuiYTKbZk9RFStjlPGxUIVPAQ9Ij2MViPxkCx2J5UMNPPo4wEPsIfX7POjgPxOvpCph0JiiHjMAnDkFcS5PnUi4H/MSCIViBxkqaXt7NIDUMUGZbPFoAAoAVQ4uOKJBcwDj2zHajjPDwwDYuc8jFQTOIAFoOHgAF6LpmGYieZMHI7Nr25BoIZPoVBFQGYePVn6EMDSDgpn66baYACO7M7TDGd52GHVBDx%2Ba8ijYF8fg4ABPVwEGIdhtyLAB3QgEDjNhYMDMdT02b0IH0dAAwQLUXzfX1PDaAAxbAADktGwBlv1OcZ0BdZUqTqRICCgcZVxZNkOWKOI4XOXlrhCO1hmFewADZiQ4UlQRlcFPFkbwFIcRSHHsdgeFgzxUJILo33QwMlxXAoxjfR9PD0ZYqzIzw%2BFQvFCE%2BNROlzPSXUcd0XmlWUuNVHp%2BkwIZRnGSYIDC9VIs1GLrC0%2BS/TIshXl2EAJEwCQSFgy4SCwA9irGVSsr2XL8uIQqyquERf2QYpOI9T1PHNe4IHSsZknuZI%2BF0dBuuIErnVdCkZhlLjvQASTBABpQ9Y3GSoAA9dzslzeiKREEBXFLhhIUjhFxY68FjTyOH2/geBIzA1uED8Px4B5QpJBKoq1SZUoA4gDmIHwwTij6Iq%2B5Kix0nriFaoKKS4l5%2BpHcZusDXr%2BsG4adiqvKCqK0axkuVbdA2jjhwIbLqrx%2Bqicwdbc38wL2ppOk01wjgJTazZgumbIcn53IAHFWXrN9vElU0MU8QwCCiiBsZy3HavxkrowJg8gcqxWarq9XLkav8CFhqV4a5qkWeIekBJs4ghPZTk4gxaSMdQIaRrG1LPAAPyBZTGYm6azcNIkZbC%2BWKZxnWVfK%2BqKoVqnlZpg3mqNiamc9C36QgF23fquTFN9k0AoDkLRAF/nPAAZRF2zPCIth7rW5A9AMTBWtNb0E91krLl0ZBf1W4yBDuwsJ2AAkSObg6iwyqYzalpuEGAOWpf1LWu%2Bj0r1bjiPtepvXk5atP8N%2BeFJJ%2BQkSM1nOsd3jeaeJ0n/YRrZg%2B2cNrASTn2s8a9yXTn%2Bp8JJ8h9JgVC9wKrW3rHbESXItDiQRNJIU5JRTdTujpQuz8zY/w4B/Iob5r6WmSEoRgPlLgPWbvoQwlxUL3i6Gg9AuIFzgMtGddWWDAFnjyIwdAVQ3yX2jPSYYuhVCCDnj/aUxlLxqAgJ4MAYBcHIE/m%2BLBJ9M4MkEjSe2olfBO0cAKAROlvaYO/ibQOpplgrhIiIWWIxdQMhxOvJW3cY7b01vHZxm99Y/kNsbbm8xPCL2XpgYAMk2FjUeOSCEng5q/3YNBL8RZPyoV0EZAsuhLF4EnsoIJxkMICA/CmNga5jwfHGENNc%2BV8DDFgmuVCQh/QxlIkWYO942BNi4pk6xrIoqXF9GwUpyA5gEm8mwexa8PFR1jrHdxd9PFJx8SnPx/8T4IPPtGEehCeADVdrfSm8y9aPwZpEmJuQgSwR2kuJCywslJOMqkgMoEvi8iNOzNWJUfA0RPt6eR8iqkXSCEBTgjzml4hGW0vERloakXKegN8O0CC0ImA5LUWESJo1eCQNCqgiSXOKJgRgjBFzxLAKsnkIDmEQM1jaBcYSNmlAUiY4uL9NhIyICjSlrD3ljA4WY0uFirFfgIGHN%2Bjoujk32VM7eMz1KTP3j3Q%2BqcuJuhPhdCAYrlk/36qMAgzd8bquBOEnlpjPR0xECfRGRCdV6uIhAH2hruUwxNVE0uLw35EGAMANQYc5WJ2lW42VcypUKsWUfZVptOHUlZKzQunttWsj1WtEgBrlJGqdcys2PNMhBxGYE5uwARW5rFRKyO8rXGq1mZKstDVQ1KszRG9q3obFhUBHOBAMEWCvU%2BIcFQ04CwjGGEEPAExgxeAumBTwVB%2BCdg5K9RcbziKuVZKCkhqgixMBYIEHccYUlpJctazCpCkhkrPiAy%2BzqXhqo1RarYhj1Lxt1QgfVhc028tNTwc1LKqR3ohA%2Bm1Dc7UvsdW%2B820bLZ10MQpJSRcAEtlyLNH4gRgDttHrc7sBZV19svHZUCa0AAcik5gNErmrTwRGBiHm%2BGGCMrBgUQFQmRekGKBH3H4EWZMC5czHK/d6Xo/BESVAMHkEQb5PzIByfm1yBJGAdPreYqanNS7TrHD6XQuUOCkJSjsTmZw4w6YKHiTm8UwZJW1NUXwZxbgSCuLoCAOldC4l5GxrBxmNTRTM3ESz%2BjARXF6HZhSvRcTCI/W3RT4Q34y2DiDNUJn3OTE8wUW4GCYYrMlOo6zyRQ4kggKohTvNRBdMxeMBoqA5aucSnF6wCX%2BSpWs2WAz9xhQNpyBAdEGWZZy3vocumJMGbCnvZaE53o1D1DQF4KMtDdBdEMmueqi4%2ByDlcngLoRY2izV7JXGErX2s9Lsc6Cq9xFN5ezYVo6qAcupbatZ3uyRm3Zac5gB0Bye5HI4kd8x4RCu9E6BdlVkpru9Aqtd3Q73S5fd0FuX7Db3XKCB755IQSw7BZELy4Ht3WRhwIMoXLYJpBmB4DIAArAoNgMhaAKFQDILQDgFj2DmNsSwKVIjk9IAQYweOzBdBAIT2QpACfSAACwk/ZwwGQCguC87Z3IPHpA4CwBgIgFAPa11kAoNaZXahiAoEE8ARwABOXng6eCi0oL0EXShcoFE0IZEX%2BBxg0aclwaX5AbL89MAwZgtGncbgpkDec/Aac09oLQTwa4ADqloiRrmGL6aPnJKn%2BEwmudQjhQ/DCcsQfEIgSmSDT7H/ISFXprj4EwN8Mv%2BfE9IKT53lO7BOBDxZTC%2BH5JrnkgLryVDPCOGSHr5I8hWfs%2BdKQLnPO%2BcyCF6QKQeHZDJEJ7QAXMxZCE7w3hpwhP7B64F1XkXtfxcgEl4P2XCuIBIAKYwfj5BuAa7GHQUguZCAkDoPjonwua8yGZ3kyyzfW/t9qEJ7vve/eUupgz%2BguCgUgtAveAueG6%2Bm%2BweMwbei%2By%2B2%2Bb%2B0ge%2BB%2B0uQ%2BI%2BvO/Ojgr%2B/eu%2BA%2BmBoB9g%2BBFOYuRBIBpAGem47AIAAuQAA%3D%3D%3D

#pragma once

#include <atomic>
#include <cassert>
#include <tuple>

namespace sss {

template<class T>
class FlagReference {
private:
  static const uintptr_t kLastBitMask = 1;
  std::atomic<uintptr_t> combine_;  // NOTE: ref+flag. ref value points to next object, but flag value is for itself

public:
  // ctor, dtor, assign
  FlagReference() = delete;
  ~FlagReference() noexcept = default;

  explicit FlagReference(T* const ref, const bool mark) : combine_(combine(ref, mark)) {}
  
  explicit FlagReference(const FlagReference& copy, const std::memory_order order = std::memory_order_seq_cst)
           : combine_(copy.combine_.load(order))  {}

  FlagReference& operator=(const FlagReference& rhs) noexcept {
    combine_.store(rhs.combine_.load(std::memory_order_relaxed), std::memory_order_relaxed);
    return *this;
  }

  // Getters 
  T* get_ref(const std::memory_order order = std::memory_order_seq_cst) const noexcept {
    const auto v = combine_.load(order);
    return parse_ref(v);
  }

  bool get_flag(const std::memory_order order = std::memory_order_seq_cst) const noexcept {
    const auto v = combine_.load(order);
    return parse_flag(v);
  }

  std::tuple<T*, bool> get(const std::memory_order order = std::memory_order_seq_cst) const noexcept {
    const auto v = combine_.load(order);
    return {parse_ref(v), parse_flag(v)};
  }

  // Setters by CAS
  bool compare_and_set(T* const expected_ref, T* const desired_ref,
                       const bool expected_flag, const bool desired_flag,
                       const std::memory_order order = std::memory_order_seq_cst) noexcept {
    auto expected_combine = combine(expected_ref, expected_flag);
    const auto desired_combine = combine(desired_ref, desired_flag);
    
    return combine_.compare_exchange_weak(expected_combine, desired_combine, order);
  }

  bool attempt_set_flag(T* const expected_ref, const bool desired_flag,
                        const std::memory_order order = std::memory_order_seq_cst) noexcept {
    const auto old_flag = get_flag();
    auto expected_combine = combine(expected_ref, old_flag);
    const auto desried_combine = combine(expected_ref, desired_flag);

    return combine_.compare_exchange_weak(expected_combine, desried_combine, order);
  }

  // Absolutely setters
  void set_flag(const bool flag, std::memory_order order = std::memory_order_seq_cst) noexcept {
    if (flag) {
      combine_.fetch_or(kLastBitMask, order);
    } else {
      combine_.fetch_and(~kLastBitMask, order);
    }
  }

  T* set_ref(T* const ref, std::memory_order order = std::memory_order_seq_cst) noexcept {
    uintptr_t old_val = combine_.load(std::memory_order_relaxed); // NOTE: if add const, compile for template success, but concreate will fail
    bool finish = false;
    while (!finish) {
      const auto new_val = combine(ref, parse_flag(old_val));
      finish = combine_.compare_exchange_weak(old_val, new_val, order); // if fail, old_val will be refreshed like combine_.load()
    }

    return parse_ref(old_val);
  }

private:
  uintptr_t combine(T* const ref, const bool flag) const noexcept {
    assert(((reinterpret_cast<uintptr_t>(ref) & kLastBitMask) == 0) && "reference must be zero for last bit");
    return reinterpret_cast<uintptr_t>(ref) | flag; // NOTE: bool guarantee to be 0 or 1 in C++
  } 

  T* parse_ref(const uintptr_t combine_val) const noexcept {
    return reinterpret_cast<T*>(combine_val & ~kLastBitMask);
  }

  bool parse_flag(const uintptr_t combine_val) const noexcept {
    return combine_val & kLastBitMask;
  }
};

} // namespace sss


