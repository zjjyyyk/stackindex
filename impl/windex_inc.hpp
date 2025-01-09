#pragma once

#include "fora_skeleton.hpp"
#include <assert.h>
#include "log/log.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "lib/scarray.hpp"
#include "fspi_base.hpp"
#include "simple_walk.hpp"

// randon-walk indexing scheme for forasp
class windex_inc : public simple_walk, public fspi_base, public IndexMethod<Config> {
private:
  struct records {
    scarray<path_id> wid;
    scarray<path_leng> wstep;
    bool empty() const noexcept { return wid.empty(); }
    size_t size() const noexcept { return wid.size(); }
  };

  std::vector<std::vector<path_id>> _walks;
  std::vector<std::vector<node_id>> _tpoints;

  std::vector<edge_sno> _n_act_edges;
  std::vector<record_sno> _n_node_recs;
  std::vector<records> _node_recs;
  std::vector<scarray<records>> _edge_recs;

  struct path {
  private:
    struct record { node_id v; record_sno sno; };
    std::vector<record> _recs;
  public:
    path(node_id src, record_sno sno, path_leng leng) :
      _recs((size_t)leng + 1) { _recs[0] = {src, sno}; }
    ~path() { _recs = std::vector<record>{}; }

    path& operator =(const path&) = delete;

    path_leng leng() const noexcept { return _recs.size() - 1; }
    record& operator[](path_leng i) { return _recs[i]; }
    const record& operator[](path_leng i) const { return _recs[i]; }
  };

  class {
  private:
    std::vector<path> _data = {path(0, 0, 0)};
    std::unordered_set<path_id> _inact;

  public:
    bool is_active(path_id id) const noexcept {
      return id > 0 && id < _data.size() && _inact.find(id) == _inact.end();
    }

    path& operator [](path_id id) {
      assert(is_active(id));
      return _data[id];
    }

    const path& operator [](path_id id) const {
      assert(is_active(id));
      return _data[id];
    }

    path_id emplace(node_id src, record_sno sno, path_leng leng) {
      path_id id;
      if (_inact.empty()) {
        id = _data.size();
        _data.emplace_back(src, sno, leng);
      } else {
        auto it = _inact.begin();
        id = *it;
        _inact.erase(it);
        new (&_data[id])path(src, sno, leng);
      }
      return id;
    }

    void release(path_id id) {
      std::destroy_at(&_data[id]);
      _inact.insert(id);
    }
  } _paths;

  class {
  private:
    std::unordered_map<path_id, path_leng> _data;
  public:
    bool exist(path_id wid) const {
      return _data.find(wid) != _data.end();
    }

    void update(path_id wid, path_leng wstep) {
      if (auto it = _data.find(wid); it != _data.end())
        it->second = std::min(it->second, wstep);
      else
        _data[wid] = wstep;
    }

    bool empty() const noexcept { return _data.empty(); }
    size_t size() const noexcept { return _data.size(); }
    auto begin() noexcept { return _data.begin(); }
    auto begin() const noexcept { return _data.begin(); }
    auto end() noexcept { return _data.end(); }
    auto end() const noexcept { return _data.end(); }
    void clear() { _data.clear(); }
  } __update_list;

  void _hit_node(path_id wid, path_leng wstep, node_id v) {
    log_trace("path-%zu hit %zu at step-%u",
      (size_t)wid, (size_t)v, (unsigned)wstep);
    path& w = _paths[wid];
    w[wstep].v = v;
    if (wstep < w.leng())
      ++_n_node_recs[v];
    else
      _tpoints[w[0].v][w[0].sno - 1] = v;
  }

  void _unhit_node(path_id wid, path_leng wstep) {
    path& w = _paths[wid];
    node_id v = w[wstep].v;
    log_trace("path-%zu unhit %zu at step-%u",
      (size_t)wid, (size_t)v, (unsigned)wstep);
    if (wstep < w.leng())
      --_n_node_recs[v];
    else
      _tpoints[w[0].v][w[0].sno - 1] = 0;
  }

  record_sno _append_record(records& recs, path_id wid, path_leng wstep) {
    recs.wid.append(wid);
    recs.wstep.append(wstep);
    return recs.size();
  }

  void _remove_record(records& recs, record_sno csno) {
    path_id wid = recs.wid[csno - 1];
    path_leng wstep = recs.wstep[csno - 1];
    assert(_paths[wid][wstep].sno == csno);
    _paths[wid][wstep].sno = 0;
    path_leng pstep;
    recs.wstep.remove(csno - 1,
      [this, &pstep](path_leng step) { pstep = step; });
    recs.wid.remove(csno - 1,
      [this, csno, pstep](path_id ww) { _paths[ww][pstep].sno = csno; });
  }

  void _swap_edge(node_id u, edge_sno esno, edge_sno eesno) {
    if (esno == eesno) return;
    _g->swap_edge(u, esno, eesno);
    _edge_recs[u].swap(esno, eesno, [](records&, records&) { });
  }

  void _hit_edge(path_id wid, path_leng wstep, node_id u, edge_sno esno) {
    node_id v = _g->get_neighbour(u, esno);
    _paths[wid][wstep].sno = _append_record(_edge_recs[u][esno], wid, wstep);
    _hit_node(wid, wstep, v);

    if (_edge_recs[u][esno].size() == 1) {
      assert(esno >= _n_act_edges[u]);
      ++_n_act_edges[u];
      _swap_edge(u, esno, _n_act_edges[u] - 1);
    }
  }

  void _unhit_edge(node_id u, edge_sno esno, record_sno csno) {
    records& recs = _edge_recs[u][esno];
    _unhit_node(recs.wid[csno - 1], recs.wstep[csno - 1]);
    _remove_record(recs, csno);

    if (_edge_recs[u][esno].empty()) {
      assert(esno < _n_act_edges[u]);
      --_n_act_edges[u];
      _swap_edge(u, esno, _n_act_edges[u]);
    }
  }

  void _random_walk(path_id wid, path_leng wstep) {
    assert(_paths.is_active(wid) && wstep > 0);
    path& w = _paths[wid];
    for (; wstep <= w.leng(); ++wstep) {
      node_id u = w[wstep - 1].v;
      if (!_g->is_dangling_node(u))
        _hit_edge(wid, wstep, u, rand_uniform(_g->get_degree(u)));
      else {
        log_trace("path-%zu hung on %zu at step-%u",
          (size_t)wid, (size_t)u, (unsigned)wstep);
        w[wstep].v = 0;
        w[wstep].sno = _append_record(_node_recs[u], wid, wstep);
        _tpoints[w[0].v][w[0].sno - 1] = u;
        break;
      }
    }
    assert(
      _tpoints[w[0].v][w[0].sno - 1] &&
      _tpoints[w[0].v][w[0].sno - 1] <= _g->num_nodes());
  }

  void _revert_walk(path_id wid, path_leng wstep) {
    assert(_paths.is_active(wid) && wstep > 0);
    path& w = _paths[wid];
    for (path_leng step = w.leng(); step >= wstep; --step) {
      node_id u = w[step - 1].v, v = w[step].v;
      record_sno csno = w[step].sno;
      if (!csno) continue;
      log_trace("path-%zu is reverted at step-%u",
        (size_t)wid, (unsigned)step);
      if (!v) {
        _tpoints[w[0].v][w[0].sno - 1] = 0;
        _remove_record(_node_recs[u], csno);
      } else {
        assert(_g->get_edge_sno(u, v).has_value());
        _unhit_edge(u, _g->get_edge_sno(u, v).value(), csno);
      }
    }
    assert(!_tpoints[w[0].v][w[0].sno - 1]);
  }

  void _append_random_walk(node_id v) {
    constexpr path_leng max_leng = ~(path_leng)0;
    path_leng l = (rand_geometric(alpha) - 1) % max_leng + 1;
    path_id wid = _paths.emplace(v, _walks[v].size() + 1, l);
    log_trace("add new path-%zu with length %zu at node %zu",
      (size_t)wid, (size_t)l, (size_t)v);
    _walks[v].push_back(wid);
    _tpoints[v].push_back(0);
    _hit_node(wid, 0, v);
    _random_walk(wid, 1);
  }

  void _remove_random_walk(node_id v) {
    path_id wid = _walks[v].back();
    log_trace("remove path-%zu on %zu", (size_t)wid, (size_t)v);
    _revert_walk(wid, 1);
    _unhit_node(wid, 0);
    _walks[v].pop_back();
    _tpoints[v].pop_back();
    _paths.release(wid);
  }

public:
  template <typename C>
  windex_inc(graph* g, bool is_dird, C config) :
    _walks(g->num_nodes() + 1),
    _tpoints(g->num_nodes() + 1),
    _n_act_edges(g->num_nodes() + 1),
    _n_node_recs(g->num_nodes() + 1),
    _node_recs(g->num_nodes() + 1),
    _edge_recs(g->num_nodes() + 1),
    IndexMethod<Config>(g, &config),
    fspi_base(g, is_dird, config)
  {
    for (node_id v = 0; v < _g->num_nodes(); ++v)
      for (edge_sno e = 0; e < _g->get_degree(v); ++e)
        _edge_recs[v].emplace();
    for (node_id v = 0; v < _g->num_nodes(); ++v)
      for (record_sno wnum = index_size(v); wnum; --wnum)
        _append_random_walk(v);
  }

  template <typename Vec>
  void adapt(const Vec&, double) { }

  node_id get(node_id s, record_sno wsno) const {
    return _tpoints[s][wsno];
  }

  void update_insert(node_id u, node_id v, edge_sno) {
    _edge_recs[u].emplace();
    edge_sno d_out = _g->get_degree(u);
    record_sno n_recs = _n_node_recs[u];

    __update_list.clear();
    if (d_out == 1) {
      // if the node had no out-edges, just remove all the hanging records
      assert(_node_recs[u].size() == n_recs);
      log_debug("reacting %zu hung random-walk(s)", (size_t)n_recs);
      for (; n_recs; --n_recs) {
        path_id wid = _node_recs[u].wid[n_recs - 1];
        path_leng wstep = _node_recs[u].wstep[n_recs - 1];
        assert(_paths[wid][wstep - 1].v == u && _paths[wid][wstep].v == 0);
        assert(_paths[wid][wstep].sno > 0);
        assert(!__update_list.exist(wid));
        __update_list.update(wid, wstep);
        _tpoints[_paths[wid][0].v][_paths[wid][0].sno - 1] = 0;
        _remove_record(_node_recs[u], n_recs);
        log_trace("react hung path-%zu at step-%u on node %zu",
          (size_t)wid, (unsigned)wstep, (size_t)u);
      }
    } else if (n_recs) {
      // otherwise, sample records to be adjusted
      assert(_node_recs[u].empty());
      log_debug("sampling among %zu hitting record(s)", (size_t)n_recs);
      record_sno n_upd = rand_binomial(n_recs, 1. / d_out);
      assert(_n_act_edges[u] < _g->get_degree(u));
      for (; n_upd; --n_upd) {
        assert(_n_act_edges[u] > 0);
        edge_sno esno = rand_uniform(_n_act_edges[u]);
        assert(!_edge_recs[u][esno].empty());
        record_sno csno = rand_uniform(_edge_recs[u][esno].size()) + 1;
        path_id wid = _edge_recs[u][esno].wid[csno - 1];
        path_leng wstep = _edge_recs[u][esno].wstep[csno - 1];
        assert(_paths[wid][wstep - 1].v == u && _paths[wid][wstep].v > 0);
        assert(_paths[wid][wstep].sno > 0);
        __update_list.update(wid, wstep);
        log_trace("sampled path-%zu at step-%u on node %zu",
          (size_t)wid, (unsigned)wstep, (size_t)u);
        _unhit_edge(u, esno, csno);
      }
      log_debug("sampled %zu random-walk(s)", __update_list.size());

      // revert sampled random-walks
      for (auto it : __update_list) _revert_walk(it.first, it.second);
    }

    // adjust sampled random-walks
    for (auto it : __update_list) {
      _hit_edge(it.first, it.second, u, d_out - 1);
      _random_walk(it.first, it.second + 1);
    }

    // add new random-walks if necessary
    while (index_size(u) > _walks[u].size()) _append_random_walk(u);
  }

  void update_delete(node_id u, node_id v, edge_sno esno) {
    __update_list.clear();
    records& recs = _edge_recs[u][esno];
    log_debug("traced %zu affected random-walk(s)", recs.size());
    for (record_sno n_upd = recs.size(); n_upd; --n_upd) {
      path_id wid = recs.wid[n_upd - 1];
      path_leng wstep = recs.wstep[n_upd - 1];
      __update_list.update(wid, wstep);
      _unhit_node(wid, wstep);
      _remove_record(recs, n_upd);
      log_trace("path-%zu is reverted at step-%u", (size_t)wid, (unsigned)wstep);
    }
    _edge_recs[u].remove(esno, [](records&) { });

    // maintain active edges
    assert(_n_act_edges[u] <= _g->get_degree(u) + 1);
    if (_n_act_edges[u] == _g->get_degree(u) + 1)
      --_n_act_edges[u];
    else if (esno < _n_act_edges[u]) {
      --_n_act_edges[u];
      _swap_edge(u, esno, _n_act_edges[u]);
    }

    // repair traced random-walks
    for (auto it : __update_list) {
      _revert_walk(it.first, it.second);
      _random_walk(it.first, it.second);
    }

    // remove random-walks if no longer require
    while (index_size(u) < _walks[u].size()) _remove_random_walk(u);
  }
};
