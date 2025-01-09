#pragma once

#include <string>
#include <tuple>
#include "graph_types.hpp"

using graph_meta = std::tuple<node_id, edge_id, bool>;

struct workload {
  edge_id num_insert, num_delete;
  node_id num_query, topk;
};

std::vector<std::string> split(const std::string& s, const std::string& c) {
  std::vector<std::string> ret;
  std::string::size_type pos1 = 0, pos2 = s.find(c);
  while (pos2 != std::string::npos) {
    ret.push_back(s.substr(pos1, pos2-pos1));
    pos1 = pos2 + c.size();
    pos2 = s.find(c, pos1);
  }
  if (pos1 != s.length())
    ret.push_back(s.substr(pos1));
  return ret;
}

using update = std::tuple<char, node_id, node_id>;
