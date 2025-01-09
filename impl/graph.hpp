#pragma once

#include <assert.h>
#include "log/log.h"
#include <optional>
#include <unordered_map>
#include <vector>
#include "lib/scarray.hpp"
#include "graph_types.hpp"

// evolvable 'directed' graph
class graph {
private:
  node_id _n_nodes;
  edge_id _n_edges;

  std::vector<scarray<node_id>> _edge_list;
  std::vector<std::unordered_map<node_id, edge_sno>> _edge_table;

public:
  graph(node_id n) :
    _n_nodes(n), _n_edges(0), _edge_list(n), _edge_table(n) { }

  node_id num_nodes() const noexcept {
    return _n_nodes;
  }

  edge_id num_edges() const noexcept {
    return _n_edges;
  }

  bool is_dangling_node(node_id v) const {
    assert(v < _n_nodes);
    return _edge_list[v].empty();
  }

  edge_sno get_degree(node_id v) const {
    assert(v < _n_nodes);
    return _edge_list[v].size();
  }

  const scarray<node_id>& get_neighbourhood(node_id v) const {
    assert(v < _n_nodes);
    return _edge_list[v];
  }

  node_id get_neighbour(node_id v, edge_sno e) const {
    assert(v < _n_nodes && e < _edge_list[v].size());
    return _edge_list[v][e];
  }

  std::optional<edge_sno> get_edge_sno(node_id u, node_id v) const {
    auto it = _edge_table[u].find(v);
    if (it == _edge_table[u].end()) return std::nullopt;
    return std::make_optional(it->second);
  }

  std::optional<edge_sno> insert_edge(node_id u, node_id v) {
    if (_edge_table[u].find(v) != _edge_table[u].end()) {
      log_warn("edge <%zu, %zu> already exists", (size_t)u, (size_t)v);
      return std::nullopt;
    }
    log_trace("insert %zu as %zu's %zu-th neighbour",
      (size_t)v, (size_t)u, _edge_list[u].size() + 1);
    ++_n_edges;
    _edge_list[u].emplace(v);
    _edge_table[u][v] = _edge_list[u].size() - 1;
    return std::make_optional(_edge_table[u][v]);
  }

  std::optional<edge_sno> delete_edge(node_id u, node_id v) {
    auto it = _edge_table[u].find(v);
    if (it == _edge_table[u].end()) {
      log_warn("edge <%zu, %zu> does not exist", (size_t)u, (size_t)v);
      return std::nullopt;
    }
    log_trace("delete the %zu-th neighbour %zu of %zu",
      (size_t)it->second + 1, (size_t)v, (size_t)u);
    --_n_edges;
    edge_sno esno = it->second;
    _edge_table[u].erase(it);
    _edge_list[u].remove(esno,
      [this, esno, u](node_id vv) { _edge_table[u][vv] = esno; });
    return std::make_optional(esno);
  }

  void swap_edge(node_id u, edge_sno esno, edge_sno eesno) {
    _edge_list[u].swap(esno, eesno,
      [this, u, esno, eesno](node_id vv, node_id v) {
        _edge_table[u][vv] = esno;
        _edge_table[u][v] = eesno;
    });
  }
};
