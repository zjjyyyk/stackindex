#pragma once

#include <cstddef>
#include <functional>

template <typename T>
size_t hash_combine(size_t s, const T& v) noexcept {
  size_t h = std::hash<T>{}(v);
  if constexpr (sizeof(size_t) == 8) {
    return s ^ (h + 0x9e3779b97f4a7c55 + (s << 12) + (s >> 4));
  } else if constexpr (sizeof(size_t) == 4) {
    return s ^ (h + 0x9e3779b1 + (s << 6) + (s >> 2));
  } else {
    return s ^ (h + 0x9e3b + (s << 3) + (s >> 1));
  }
}

size_t hash_accumulate(size_t s) noexcept {
  return s;
}

template <typename T, typename... Ts>
size_t hash_accumulate(size_t s, const T& v, const Ts&... vs) noexcept {
  return hash_accumulate(hash_combine(s, v), vs...);
}
