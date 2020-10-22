#ifndef RESMACK_TYPES
#define RESMACK_TYPES

#include <set>
#include <map>
#include <vector>
#include <unordered_map>

namespace resmack {
  template <class T>
  using Vector = std::vector<T>;

  template<class T, class U>
  using Map = std::unordered_map<T, U>;

  template<class T>
  using Set = std::set<T>;
}

#endif
