#include "Index-stackindex.hpp"
#include "Index-rw.hpp"
#include "Index-realtime.hpp"
#include "windex_inc.hpp"
#include "apps/types.hpp"
#include "fora_skeleton.hpp"
#include "graph.hpp"
#include "graph_types.hpp"
#include "lib/ConvenientPrint.hpp"
#include "io/file.hpp"
#include "log/log.h"
#include "time/timer.hpp"
#include "exp_util.hpp"
#include <cstdio>
#include <cstring>
#include <functional>
#include <fstream>
#include <iomanip>
#include <algorithm>



graph *read_base_graph(const char *dataset, Config & C) {
    fprintf(stdout, "loading meta data\n");
    auto [n, m, directed] = load_file<graph_meta>(file_path(2, dataset, "meta"));
    fprintf(stdout, "n = %zu, m = %zu, %s\n", (size_t)n, (size_t)m,
            directed ? "directed" : "undirected");
    fflush(stdout);

    fprintf(stdout, "loading base graph\n");
    fflush(stdout);
    auto edges = load_file<edge_list>(file_path(2, dataset, "graph_base"));

    fprintf(stdout, "building base graph\n");
    fflush(stdout);

    graph *g = new graph(n);
    for (auto [u, v] : edges) {
        g->insert_edge(u, v);
        if (!directed && u != v)
            g->insert_edge(v, u);
    }

    C.is_dird = directed;

    return g;
}

std::vector<update> read_workload(const char* dataset, std::string workload) {
  fprintf(stdout, "reading workload %s\n", workload.c_str());
  fflush(stdout);

  std::string workload_path = file_path(3, dataset, "workloads", workload.c_str());

  return load_file<std::vector<update>>(workload_path);
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <dataset> <method> <workload> <savedir>\n", argv[0]);
        return 1;
    }

    std::string dataset(argv[1]);

    // method can be "stackindex", "rwindex", "realtime"
    std::string method(argv[2]);

    std::string workload(argv[3]);

    std::string savedir(argv[4]);
    ensure_dir(savedir);
    savedir += "/" + workload;
    ensure_dir(savedir);

    std::string savepath = savedir + "/" + method + ".txt";
    std::ofstream outfile;
    outfile.open(savepath);

    std::vector<update> w = read_workload(argv[1],workload);

    Config C(true, 0.2, 0.3, 0.1, 0.01, 0.01);
    graph *G = read_base_graph(argv[1],C);
    FORA<Config> * f = new FORA<Config>; 
    IndexMethod<Config> * I;

    // Define singlesource Solver
    if (method == "stackindex") {
        I = new StackIndex(G, &C);
    } else if (method == "rwindex") {
        I = new windex_inc(G,C.is_dird, C);
    } else if (method == "realtime") {
        I = new StackIndex_Realtime(G, &C);
    } else {
        fprintf(stderr, "Unknown method: %s\n", method.c_str());
        return 1;
    }

    std::vector<double> res;
    auto outputer = [&](const std::vector<double> & ppr){
        res = std::move(ppr);
    };


    for (auto [o, u, v] : w) {
      Timer::reset_all();
      // I->conf->is_dird = C.is_dird;
      // I->conf->alpha = C.alpha;
      if (o == '?') {
        node_id s = u, k = v;
        // log_info("querying source %zu", (size_t)s);
        printf("querying source %zu\n", (size_t)s);
        f->evaluate_noprint(I, s, outputer);
        double t = Timer::used(TIMER::EVALUATE);
        std::cout << std::setprecision(16) << o << "\t" << t << std::endl;
        outfile << std::setprecision(16) << o << "\t" << t << std::endl;
      } else if (o == '+') {
        // log_info("inserting edge %zu %zu", (size_t)u, (size_t)v);
        printf("inserting edge %zu %zu\n", (size_t)u, (size_t)v);
        f->insert_edge(u, v, I);
        double t = Timer::used(TIMER::UPDATE);
        std::cout << std::setprecision(16) << o << "\t" << t << std::endl;
        outfile << std::setprecision(16) << o << "\t" << t << std::endl;
      } else if (o == '-') {
        // log_info("deleting edge %zu %zu", (size_t)u, (size_t)v);
        printf("deleting edge %zu %zu\n", (size_t)u, (size_t)v);
        f->delete_edge(u, v, I);
        double t = Timer::used(TIMER::UPDATE);
        std::cout << std::setprecision(16) << o << "\t" << t << std::endl;
        outfile << std::setprecision(16) << o << "\t" << t << std::endl;
      } else {
        log_error("unknown operation %c", o);
      }
    }
    
    printf("Saved to %s\n", savepath.c_str());

    delete f;
    delete I;
    delete G;
    // int a;
    return 0;
}
