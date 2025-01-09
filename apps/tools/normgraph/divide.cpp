#include <algorithm>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>
#include "apps/io/file.hpp"
#include "apps/types.hpp"

constexpr char help[] =
  "divide <data_path> [options]\n"
  "options:\n"
  "  --base_ratio <size ratio of basic graph>\n"
  "  --del_ratio <size ratio of removable edges>\n";

#define filepath(file) (file_path(2, argv[1], file))

int main(int argc, char *argv[]) {
  if (argc == 1) {
    log_fatal("missing argument <data_path>\nusage:\n%s", help);
    return -1;
  }

  double base_ratio = 0.9, del_ratio = 0.1;
  for (int i = 2; i < argc; ++i) {
    if (strcmp(argv[i], "--base_ratio") == 0) {
      base_ratio = atof(argv[++i]);
      if (base_ratio < 0 || base_ratio > 1) {
        log_fatal("invalid base_ratio, must be in [0,1]");
        return -1;
      }
    } else if (strcmp(argv[i], "--del_ratio") == 0) {
      del_ratio = atof(argv[++i]);
      if (del_ratio < 0 || del_ratio > 1) {
        log_fatal("invalid del_ratio, must be in [0,1]");
        return -1;
      }
    } else {
      log_fatal("unknown option %s\nusage:\n%s", argv[i], help);
      return -1;
    }
  }

  auto [n, m, dird] = load_file<graph_meta>(filepath("meta"));
  auto edges = load_file<edge_list>(filepath("graph"));

  std::mt19937 rand(std::random_device{}());
  std::shuffle(edges.begin(), edges.end(), rand);

  edge_list e_ins;
  for (size_t i = 0; i < m * (1 - base_ratio); ++i) {
    e_ins.push_back(edges.back());
    edges.pop_back();
  }

  edge_list e_del;
  del_ratio = std::min(del_ratio, base_ratio);
  for (size_t i = 0; i < m * del_ratio; ++i) {
    e_del.push_back(edges[i]);
  }

  save_file(filepath("graph_base"), edges);
  save_file(filepath("edges_ins"), e_ins);
  save_file(filepath("edges_del"), e_del);

  return 0;
}
