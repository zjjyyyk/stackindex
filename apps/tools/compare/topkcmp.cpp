#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>
#include "apps/io/file.hpp"
#include "apps/types.hpp"
#include "fora_interface.hpp"

constexpr char help[] =
  "topkcmp <data_path> <truth_path> <result_path> [workloads]\n";

#define workload_path(workload) \
  (file_path(3, argv[1], "workloads", workload.c_str()))
#define truth_path(workload, node) \
  (file_path( \
    5, argv[1], "results", argv[2], \
    workload.c_str(), std::to_string(node).c_str()))
#define result_folder(workload) \
  (file_path(4, argv[1], "results", argv[3], workload.c_str()))
#define result_path(workload, node) \
  (file_path(2, result_folder(workload).c_str(), std::to_string(node).c_str()))

int main(int argc, char *argv[]) {
  if (argc < 2) {
    log_fatal("missing argument <data_path>\nusage:\n%s", help);
    return -1;
  } else if (argc < 3) {
    log_fatal("missing argument <truth_path>\nusage:\n%s", help);
    return -1;
  } else if (argc < 4) {
    log_fatal("missing argument <result_path>\nusage:\n%s", help);
    return -1;
  }

  std::vector<std::string> workloads;
  for (int i = 4; i < argc; ++i) {
    size_t _;
    bool ok = sscanf(argv[i], "i%zud%zuq%zuk%zu", &_, &_, &_, &_) == 4;
    if (ok) {
      workloads.push_back(std::string(argv[i]));
    } else {
      log_fatal("invalid workload %s\nusage:\n%s", argv[i], help);
      return -1;
    }
  }

  size_t o_truth_size = 0, o_ret_cnt = 0;
  for (std::string& workload : workloads) {
    log_info("processing workload %s", workload.c_str());
    auto [alpha, eps, det, pf] = load_file<fora_interface::econfigs>(
      file_path(2, result_folder(workload).c_str(), "meta_configs"));
    auto updates = load_file<std::vector<update>>(workload_path(workload));
    for (auto [c, s, k] : updates) {
      if (c != '?') continue;
      if (k == 0) continue;
      auto vec0 = load_file<std::vector<node_id>>(truth_path(workload, s));
      auto vec1 = load_file<std::vector<node_id>>(result_path(workload, s));
      size_t truth_size = vec0.size(), ret_cnt = 0;
      assert(truth_size <= k);
      std::unordered_set<node_id> truth;
      for (node_id v : vec0) truth.insert(v);
      for (node_id v : vec1) {
        if (truth.find(v) != truth.end()) ++ret_cnt;
      }
      o_truth_size += truth_size;
      o_ret_cnt += ret_cnt;
      log_info("source %zu", (size_t)s);
      log_info("precision: %.2lf", 1. * ret_cnt / truth_size);
    }
    printf("overall precision: %.2lf\n", 1. * o_ret_cnt / o_truth_size);
  }
  return 0;
}
