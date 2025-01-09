#pragma once

#include <assert.h>
#include "log/log.h"
#include <array>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include <iostream>

/**
 * @brief A simple container for dynamic arrays with automatic scaling.
 * 
 * @tparam T The type of the elements.
 */
template <typename T>
class scarray {
public:
  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using rvalue_reference = T&&;

private:
  pointer _begin = nullptr;
  pointer _end = nullptr;
  pointer _limit = nullptr;

  pointer _scale_return_stale(size_t n) {
    assert(n >= _end - _begin);
    pointer stale_data = _begin;
    if (n == 0) _begin = _end = _limit = nullptr;
    else {
      size_t memsz = sizeof(value_type) * n;
      size_t elemsz = (uintptr_t)_end - (uintptr_t)_begin;
      _begin = (pointer)memcpy(malloc(memsz), _begin, elemsz);
      _end = (pointer)((uintptr_t)_begin + elemsz);
      _limit = (pointer)((uintptr_t)_begin + memsz);
    }
    return stale_data;
  }

  pointer _try_expand() {
    if (_end ==_limit) {
      if (_begin == nullptr) return _scale_return_stale(1);
      return _scale_return_stale((_limit - _begin) * 2);
    }
    return nullptr;
  }

  pointer _try_shrink() {
    if (_begin == _end) return _scale_return_stale(0);
    if ((_end - _begin) * 4 <= _limit - _begin)
      return _scale_return_stale((_limit - _begin) / 2);
    return nullptr;
  }

public:
  constexpr scarray() noexcept :
    _begin(nullptr), _end(nullptr), _limit(nullptr) { }

  constexpr explicit scarray(size_t n) :
    _begin(n ? (pointer)malloc(sizeof(value_type) * n) : nullptr),
    _end(_begin),
    _limit(_begin + n) { }

  constexpr explicit scarray(std::initializer_list<value_type> data) :
    _begin(data.size() ?
      (pointer)malloc(sizeof(value_type)*data.size() ) :
      nullptr),
    _end(data.size() ?
      (pointer)std::copy(data.begin(), data.end(), _begin) :
      nullptr),
    _limit(_end) { }

  constexpr explicit scarray(size_t n, const T* data) :
    _begin(n ? (pointer)malloc(sizeof(value_type) * n) : nullptr),
    _end(n ? std::copy(data, data + n, _begin) : nullptr),
    _limit(_end) { }

  constexpr explicit scarray(const std::vector<value_type>& data) :
    _begin(data.empty() ?
      nullptr :
      (pointer)malloc(sizeof(value_type)*data.size())),
    _end(data.empty() ?
      nullptr :
      (pointer)std::copy(data.begin(), data.end(), _begin)),
    _limit(_end) { }

  template <size_t n>
  constexpr explicit scarray(const std::array<value_type, n>& data):
    _begin(n ? (pointer)malloc(sizeof(value_type) * n) : nullptr),
    _end(n ? (pointer)std::copy(data.begin(), data.end(), _begin) : nullptr),
    _limit(_end) { }

#ifdef SC_COPY_DISABLE
  scarray(const scarray&) = delete;

  scarray& operator =(const scarray&) = delete;
#else
  constexpr scarray(const scarray& c) :
    _begin(c.empty() ?
      nullptr :
      (pointer)malloc((uintptr_t)c._end - (uintptr_t)c._begin)),
    _end(c.empty() ?
      nullptr :
      (pointer)std::copy(c._begin, c._end, _begin)),
    _limit(_end) { }

  constexpr scarray& operator =(const scarray& c) {
    if (this != &c) {
      std::destroy_at(this);
      new (this)scarray(c);
    }
    return *this;
  }
#endif

#ifdef SC_MOVE_DISABLE
  scarray(scarray&&) = delete;

  scarray& operator =(scarray&&) = delete;
#else
  constexpr scarray(scarray&& c) noexcept :
    _begin(c._begin), _end(c._end), _limit(c._limit)
  {
    c._begin = c._end = c._limit = nullptr;
  }

  constexpr scarray& operator =(scarray&& c) noexcept {
    if (this != &c) {
      std::destroy_at(this);
      new (this)scarray(std::forward<scarray>(c));
    }
    return *this;
  }
#endif

  ~scarray() {
    if constexpr (
      !std::is_standard_layout_v<value_type> &&
      !std::is_trivial_v<value_type>)
    {
      for (; _end != _begin; std::destroy_at(--_end));
    }
    free(_begin);
    _begin = _end = _limit = nullptr;
  }

  constexpr bool empty() const noexcept {
    return _begin == _end;
  }

  constexpr size_t size() const noexcept {
    return _end - _begin;
  }

  constexpr pointer begin() noexcept {
    return _begin;
  }

  constexpr const_pointer begin() const noexcept {
    return _begin;
  }

  constexpr pointer end() noexcept {
    return _end;
  }

  constexpr const_pointer end() const noexcept {
    return _end;
  }

  constexpr reference operator[](size_t idx) {
    assert(_begin + idx < _end);
    return _begin[idx];
  }

  constexpr const_reference operator[](size_t idx) const {
    assert(_begin + idx < _end);
    return _begin[idx];
  }

  template <typename... Args>
  void emplace(Args&&... args) {
    pointer stale_data = _try_expand();
    new (_end++)value_type(std::forward<Args>(args)...);
    free(stale_data);
  }

  void append(const_reference val) {
    pointer stale_data = _try_expand();
    new (_end++)value_type(val);
    free(stale_data);
  }

  void append(rvalue_reference val) {
    pointer stale_data = _try_expand();
    new (_end++)value_type(std::forward<value_type>(val));
    free(stale_data);
  }

  void remove() {
    std::destroy_at(--_end);
    free(_try_shrink());
  }

  template <typename F>
  requires std::is_invocable_v<F, T&>
  void remove(size_t idx, F after_swap) {
    pointer pos = _begin + idx;
    assert(pos < _end);
    std::destroy_at(pos);
    if (pos < --_end) {
      *pos = std::move(*_end);
      after_swap(*pos);
    }
    free(_try_shrink());
  }

  template <typename F>
  requires std::is_invocable_v<F, T&, T&>
  void swap(size_t i, size_t j, F after_swap) {
    assert(_begin + i < _end && _begin + j < _end);
    if (i != j) {
      value_type temp = std::move(_begin[i]);
      _begin[i] = std::move(_begin[j]);
      _begin[j] = std::move(temp);
      after_swap(_begin[i], _begin[j]);
    }
  }

  void clear() {
    std::destroy_at(this);
    _begin = _end = _limit = nullptr;
  }
};
