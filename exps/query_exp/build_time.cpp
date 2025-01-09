#include "Index-stackindex.hpp"
#include "Index-rw.hpp"
#include "Index-realtime.hpp"
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
#include <iostream>
#include <iomanip>
#include <memory>
#include <sys/time.h>
#include <cstdlib>
#include <ctime>



graph *read_graph(const char *dataset, Config & C) {
    fprintf(stdout, "loading meta data\n");
    auto [n, m, directed] = load_file<graph_meta>(file_path(2, dataset, "meta"));
    fprintf(stdout, "n = %zu, m = %zu, %s\n", (size_t)n, (size_t)m,
            directed ? "directed" : "undirected");
    fflush(stdout);

    fprintf(stdout, "loading base graph\n");
    fflush(stdout);
    auto edges = load_file<edge_list>(file_path(2, dataset, "graph"));

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

void handle_workload(char *argv[], std::string workload, bool output) {
    fprintf(stdout, "handling workload %s\n", workload.c_str());
    fflush(stdout);
    Timer::reset_all();

    fprintf(stdout, "time for updates: %lf\n", Timer::used(TIMER::UPDATE));
    fprintf(stdout, "time for queries: %lf"
                    "(adapt: %lf, push: %lf, refine: %lf)\n",
            Timer::used(TIMER::EVALUATE),
            Timer::used(TIMER::ADAPT),
            Timer::used(TIMER::PUSH),
            Timer::used(TIMER::REFINE));
    fprintf(stdout, "time for output: %lf\n", Timer::used(TIMER::OUTPUT));
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    std::srand(std::time(0));

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <dataset> <method> <savedir> \n", argv[0]);
        return 1;
    }

    bool force = false;
    for (int i = 4; i < argc; i++) {
        if (strcmp(argv[i], "--force") == 0) {
            force = true;
            break;
        }
    }

    // method can be "stackindex", "rwindex", "realtime"
    std::string method(argv[2]);

    std::string savedir(argv[3]);
    ensure_dir(savedir);
    std::string savepath = savedir + "/" + method + ".txt";

    int done = 0;
    int xs = 41;

    if(!force){
        std::ifstream infile(savepath);
        if(infile.is_open()){
            std::vector<double> exist_alphas;
            double value;
            while (infile >> value) {
                exist_alphas.push_back(value);
            }
            done = exist_alphas.size() / 2;
        }
        infile.close();
        printf("done: %d\n",done);
        if(done == xs){
            printf("All done\n");
            return 0;
        }
    }

    
    Config C(true, 0.2, 0.3, 0.1, 0.01, 0.01); // precision (0.3,0.1,0.01,0.01 fixed to make omega 12)

    graph *G = read_graph(argv[1],C);
    FORA<Config> * f = new FORA<Config>; // Invariant: Config is fixed.

    
    std::ofstream outfile;
    if(!force){
        outfile.open(savepath, std::ios_base::app);
    } else{
        outfile.open(savepath);
    }

    if (!outfile.is_open()) {
        fprintf(stderr, "Failed to open file: %s\n", savepath.c_str());
        return 1;
    }

    std::vector<std::pair<double,double>> time_vec = {};

    std::unique_ptr<IndexMethod<Config>> I; // 使用std::unique_ptr而不是裸指针
    for(int i = done; i < xs; i++){
        size_t repeat_time = 12;
        double alpha = 0.99 - (0.99-0.01)/(xs-1)*i;
        C.alpha = alpha;

        Timer::reset_all();
    
        // Define singlesource Solver
        if (method == "stackindex") {
            I.reset(new StackIndex_Static(G, &C)); // 使用reset来分配新的对象
        } else if (method == "rwindex") {
            I.reset(new RwIndex(G, &C));
        } else if (method == "realtime") {
            I.reset(new RealTimeIndex(G, &C));
        } else {
            fprintf(stderr, "Unknown method: %s\n", method.c_str());
            return 1;
        }

        printf("alpha:%lf, build_time:%lf s \n",alpha,Timer::used(TIMER::BUILD));
        time_vec.emplace_back(std::make_pair(alpha,Timer::used(TIMER::BUILD)));
        outfile << std::setprecision(16) << alpha << "\t" << Timer::used(TIMER::BUILD) << std::endl;

        // 不需要手动delete I，当I离开作用域或被重新reset时，它指向的对象会自动被删除
    }

    printf("Saved to %s\n",savepath.c_str());
    outfile.close();


    delete f;
    delete G;

    // int a;
    return 0;
}
