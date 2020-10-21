#ifndef RESMACK_TYPES
#define RESMACK_TYPES

#include <set>
#include <map>
#include <unordered_map>

namespace resmack {

  template<class T, class U>
  using Map = std::unordered_map<T, U>;

  template<class T>
  using Set = std::set<T>;
}

#endif
