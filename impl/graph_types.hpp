#pragma once

#include <cstddef>
#include <utility>
#include <vector>
#include "lib/scaling.hpp"

#ifndef ULTRASCALE_GRAPH
  using node_id = uint32_t;
#else
  using node_id = uint64_t;
#endif

using edge = std::pair<node_id, node_id>;

using edge_list = std::vector<edge>;

#ifndef DENSE_GRAPH
  using edge_id = node_id;
#else
  using edge_id = scaling::int_scaling_t<node_id, 1>;
#endif

using edge_sno = node_id;

#ifndef INDEX_EXTEND
  using path_id = edge_id;
#else
  using path_id = scaling::int_scaling_t<edge_id, 1>;
#endif

using path_leng = scaling::int_scaling_t<path_id, -2>;

using record_sno = path_id;
