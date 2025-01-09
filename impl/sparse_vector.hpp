#pragma once

#include <assert.h>
#include <algorithm>
#include <cstring>
#include "graph_types.hpp"

class sparse_vector {
private:
  size_t _n, _c;
  double* _data;
  node_id* _occur;

public:
  class iterator: public
    std::iterator<
      std::input_iterator_tag,
      node_id, node_id, const node_id*, const node_id&>
  {
  private:
    const node_id* _ids;
    const node_id _lim;
    node_id _pos;
  public:
    iterator(node_id* ids, node_id lim, node_id pos) :
      _ids(ids), _lim(lim), _pos(pos) { }
    iterator& operator ++() { ++_pos; return *this; }
    iterator operator ++(int) { iterator ret = *this; ++*this; return ret; }
    bool operator ==(iterator other) const { return _pos == other._pos; }
    bool operator !=(iterator other) const { return !(*this == other); }
    reference operator *() const { return _ids ? _ids[_pos] : _pos; }
  };

  sparse_vector(node_id n) :
    _n(n), _c(0), _data(new double[n]), _occur(new node_id[n >> 6])
  {
    memset(_data, 0, sizeof(double) * n);
    memset(_occur, 0, sizeof(node_id) * (n >> 6));
  }

  ~sparse_vector() {
    if (_data) delete[] _data;
    if (_occur) delete[] _occur;
  }

  void iterize() {
    if (_c < (_n >> 6)) {
      std::sort(_occur, _occur + _c);
      _c = std::unique(_occur, _occur + _c) - _occur;
    }
  }

  size_t size() const noexcept {
    return _c < (_n >> 6) ? _c : _n;
  }

  iterator begin() const noexcept {
    if (_c < (_n >> 6))
      return iterator(_occur, _c, 0);
    return iterator(nullptr, _n, 0);
  }

  iterator end() const noexcept {
    if (_c < (_n >> 6))
      return iterator(_occur, _c, _c);
    return iterator(nullptr, _n, _n);
  }

  void clear() {
    if (_c < (_n >> 6))
      for (size_t i = 0; i < _c; ++i) _data[_occur[i]] = 0;
    else
      memset(_data, 0, sizeof(double) * _n);
    _c = 0;
  }

  sparse_vector& operator =(const sparse_vector& other) {
    if (&other == this) return *this;
    if (_n != other._n) {
      delete[] _data;
      delete[] _occur;
      _n = other._n;
      _data = new double[_n];
      _occur = new node_id[_n >> 6];
    }
    if (_c < (_n >> 6) && other._c < (_n >> 6)) {
      for (size_t i = 0; i < _c; ++i) _data[_occur[i]] = 0;
      for (size_t i = 0; i < other._c; ++i)
        _data[other._occur[i]] = other._data[other._occur[i]];
    } else
      memcpy(_data, other._data, sizeof(double) * _n);
    _c = other._c;
    memcpy(_occur, other._occur, sizeof(node_id) * _c);
    return *this;
  }

  sparse_vector& operator =(sparse_vector&& other) {
    if (&other == this) return *this;
    _n = other._n;
    _c = other._c;
    _data = other._data;
    _occur = other._occur;
    other._n = other._c = 0;
    other._data = nullptr;
    other._occur = nullptr;
    return *this;
  }

  double operator [](node_id v) const {
    assert(v < _n);
    return _data[v];
  }

  void update(node_id v, double val) {
    assert(v < _n);
    _data[v] = val;
    if (_c < (_n >> 6)) _occur[_c++] = v;
  }

  void accumulate(node_id v, double val) {
    assert(v < _n);
    _data[v] += val;
    if (_c < (_n >> 6)) _occur[_c++] = v;
  }
};
