#pragma once

#include <queue>
#include <vector>
#include "graph_types.hpp"

class uniqueue {
private:
  std::queue<node_id> _queue;
  std::vector<bool> _isact;

public:
  uniqueue(size_t n) : _queue(), _isact(n) { }

  bool empty() const noexcept {
    return _queue.empty();
  }

  size_t size() const noexcept {
    return _queue.size();
  }

  void push(node_id v) {
    if (!_isact[v]) {
      _isact[v] = true;
      _queue.push(v);
    }
  }

  node_id pop() {
    node_id v = _queue.front();
    _isact[v] = false;
    _queue.pop();
    return v;
  }

  void clear() {
    while (!_queue.empty()) {
      _isact[_queue.front()] = false;
      _queue.pop();
    }
  }

  bool is_active(node_id v){
    return _isact[v];
  }
};
