#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "apps/io/file.hpp"
#include "apps/types.hpp"
#include "fora_interface.hpp"

constexpr char help[] =
  "vectcmp <data_path> <truth_path> <result_path> [workloads]\n";

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

  for (std::string& workload : workloads) {
    size_t o_tot_nodes = 0;
    double o_tot_err = .0, max_err = .0;
    printf("workload %s\n", workload.c_str());
    auto [alpha, eps, det, pf] = load_file<fora_interface::econfigs>(
      file_path(2, result_folder(workload).c_str(), "meta_configs"));
    auto updates = load_file<std::vector<update>>(workload_path(workload));
    for (auto [c, s, k] : updates) {
      if (c != '?') continue;
      if (k != 0) continue;
      size_t cnt = 0;
      double tot_err = .0;
      auto vec0 = load_file<std::vector<double>>(truth_path(workload, s));
      auto vec1 = load_file<std::vector<double>>(result_path(workload, s));
      if (vec0.size() != vec1.size()) {
        log_error("invaild result with deformed size");
        exit(1);
      }
      size_t n = vec0.size() - 1;
      for (size_t i = 1; i <= n; ++i) {
        if (vec0[i] < det) continue;
        ++cnt;
        double err = fabs(vec1[i] - vec0[i]) / vec0[i];
        if (err > 0.5) log_info("%e %e", vec1[i], vec0[i]);
        tot_err += err;
        max_err = std::max(max_err, err);
      }
      o_tot_err += tot_err;
      o_tot_nodes += cnt;
      log_info("source %zu", (size_t)s);
      log_info("average relative error: %e", tot_err / cnt);
      log_info("worst relative error: %e", max_err);
    }
    printf("overall average relative error: %e\n", o_tot_err / o_tot_nodes);
    printf("overall worst relative error: %e\n", max_err);
  }
  return 0;
}
