#pragma once

#include <assert.h>
#include "log/log.h"
#include "lib/random.hpp"
#include "graph.hpp"

// support simple random walk method
class simple_walk {
protected:
  node_id random_walk(graph* const g, node_id v, double alpha) const {
    while (true) {
      if (g->is_dangling_node(v)) return v;
      edge_sno esno = rand_uniform(g->get_degree(v));
      v = g->get_neighbour(v, esno);
      if (rand_uniformf() < alpha) return v;
    };
  }
};
