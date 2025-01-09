#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <utility>
#include "apps/io/file.hpp"
#include "apps/types.hpp"
#include "lib/random.hpp"

constexpr char help[] =
  "generate <data_path> [options]\n"
  "options:\n"
  "  --n_nodes <number of nodes>\n"
  "  --p_edges <probability an edge will occur>\n"
  "  --directed|undirected\n";

#define filepath(file) (file_path(2, argv[1], file))

int main(int argc, char *argv[]) {
  if (argc < 2) {
    log_fatal("missing argument <data_path>\nusage:\n%s", help);
    return -1;
  }

  node_id n = 0;
  double p = 0.0;
  bool dird = true;
  for (int i = 2; i < argc; ++i) {
    if (strcmp(argv[i], "--n_nodes") == 0) {
      n = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--p_edges") == 0) {
      p = atof(argv[++i]);
      if (p < 0 || p > 1) {
        log_fatal("invalid p_edges, must be in [0,1]");
        return -1;
      }
    } else if (strcmp(argv[i], "--directed") == 0) {
      dird = true;
    } else if (strcmp(argv[i], "--undirected") == 0) {
      dird = false;
    } else {
      log_fatal("unknown option %s\nusage:\n%s", argv[i], help);
      return -1;
    }
  }
  edge_list edges;
  if (p > 0) {
    for (node_id u = 1; u <= n; ++u) {
      for (node_id v = dird ? 0 : u; ; ) {
        v += rand_geometric(p);
        if (v > n) break;
        edges.push_back(std::make_pair(u, v));
      }
    }
  }
  edge_id m = edges.size();

  log_info("graph generated, n = %zu, m = %zu", (size_t)n, (size_t)m);

  save_file(filepath("meta"), std::make_tuple(n, m, dird));
  save_file(filepath("graph"), edges);

  return 0;
}