#include <algorithm>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>
#include "apps/io/file.hpp"
#include "apps/types.hpp"

constexpr char help[] =
  "process <data_path> [workloads]\n"
  "workload format:\n"
  "  i<num insert>d<num delete>q<num query>k<topk>\n"
  "  <topk> = 0 to generate full queries\n";

#define filepath(file) (file_path(2, argv[1], file))
#define workload_path(workload) \
  (file_path(3, argv[1], "workloads", workload.c_str()))

int main(int argc, char *argv[]) {
  if (argc < 2) {
    log_fatal("missing argument <data_path>\nusage:\n%s", help);
    return -1;
  }

  std::vector<std::string> workloads;
  for (int i = 2; i < argc; ++i) {
    size_t _;
    bool ok = sscanf(argv[i], "i%zud%zuq%zuk%zu", &_, &_, &_, &_) == 4;
    if (ok) {
      workloads.push_back(std::string(argv[i]));
    } else {
      log_fatal("invalid workload %s\nusage:\n%s", argv[i], help);
      return -1;
    }
  }

  auto [n, m, dird] = load_file<graph_meta>(filepath("meta"));
  auto e_ins = load_file<edge_list>(filepath("edges_ins"));
  auto e_del = load_file<edge_list>(filepath("edges_del"));

  std::mt19937 rand(std::random_device{}());
  std::uniform_int_distribution<node_id> rand_node_id(1, n);

  for (auto& workload : workloads) {
    size_t num_ins, num_del, num_q, topk;
    std::vector<update> queries;

    sscanf(workload.c_str(), "i%zud%zuq%zuk%zu",
      &num_ins, &num_del, &num_q, &topk);
    log_info("generating workload");
    log_info("n_ins = %zu, n_del = %zu, n_q = %zu, topk = %zu",
      num_ins, num_del, num_q, topk);

    if (num_ins > e_ins.size()) {
      log_error("skipped, no enough edges to be inserted");
      continue;
    }

    if (num_del > e_del.size()) {
      log_error("skipped, no enough edges to be deleted");
      continue;
    }

    while (num_ins--) {
      auto [u, v] = e_ins.back();
      log_debug("add inserting edge %zu %zu", (size_t)u, (size_t)v);
      queries.push_back(std::make_tuple('+', u, v));
      e_ins.pop_back();
    }

    while (num_del--) {
      auto [u, v] = e_del.back();
      log_debug("add deleting edge %zu %zu", (size_t)u, (size_t)v);
      queries.push_back(std::make_tuple('-', u, v));
      e_del.pop_back();
    }

    while (num_q--) {
      node_id s =  rand_node_id(rand);
      log_debug("add query source %zu", (size_t)s);
      queries.push_back(std::make_tuple('?', s, topk));
    }

    std::shuffle(queries.begin(), queries.end(), rand);

    save_file(workload_path(workload), queries);
  }

  return 0;
}
