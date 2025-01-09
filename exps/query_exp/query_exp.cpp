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
#include <fstream>
#include <iomanip>
#include <algorithm>



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
    if (argc < 6) {
        fprintf(stderr, "Usage: %s <dataset> <alpha> <method> <truthdir> <savedir> \n", argv[0]);
        return 1;
    }

    bool force = false;
    for (int i = 6; i < argc; i++) {
        if (strcmp(argv[i], "--force") == 0) {
            force = true;
            break;
        }
    }

    // method can be "stackindex", "rwindex", "realtime"
    std::string method(argv[3]);

    std::string truthdir(argv[4]);
    std::string savedir(argv[5]);
    ensure_dir(savedir);
    std::string savepath = savedir + "/" + method + ".txt";


    std::vector<double> epss = {0.5,0.4,0.3,0.2,0.1};
    std::vector<double> done = {};
    if(!force){
        done = read_done<double>(savepath);
    }

    printf("done: %zu\n",done.size());
    if(done.size() == epss.size()){
        std::sort(done.begin(), done.end(), std::greater<double>());
        bool all_done = true;
        for(int i=0;i<done.size();++i){
            if(std::abs(done[i]-epss[i]) > 1e-7){
                all_done = false;
                break;
            }
        }
        if(all_done){
            printf("All done.\n");
            return 0;
        }
    }

    std::ofstream outfile;
    if(force){
        outfile.open(savepath);
    } else{
        outfile.open(savepath, std::ios::app);
    }
    if (!outfile.is_open()) {
        fprintf(stderr, "Failed to open outfile\n");
        return 1;
    }

    Config C(true, 0.2, 0.3, 0.1, 0.01, 0.01); // precision (0.3,0.1,0.01,0.01 fixed to make omega 12)
    graph *G = read_graph(argv[1],C);
    double alpha = atof(argv[2]);
    C.alpha = alpha;
    FORA<Config> * f = new FORA<Config>; 
    IndexMethod<Config> * I;

    std::vector<std::pair<int,std::vector<double>>> truth = read_singlesource_res(truthdir);
    std::vector<int> sources = {};
    for(auto& [s,ppr]: truth){
        sources.emplace_back(s);
    }

    std::vector<double> res;
    auto outputer = [&](const std::vector<double> & ppr){
        res = std::move(ppr);
    };

    for(auto eps:epss)
    {
        printf("eps: %lf\n",eps);
        if(d_is_in(eps,done)){
            printf("skipped\n");
            continue;
        }


        C.eps = eps;

        // Define singlesource Solver
        if (method == "stackindex") {
            I = new StackIndex_Static(G, &C);
        } else if (method == "rwindex") {
            I = new RwIndex(G, &C);
        } else if (method == "realtime") {
            I = new RealTimeIndex(G, &C);
        } else {
            fprintf(stderr, "Unknown method: %s\n", method.c_str());
            return 1;
        }

        auto solver = [&](int s){
            f->evaluate(I, s, outputer);
            return res;
        };

        std::vector<std::pair<int,std::vector<double>>> estimate = {};

        std::vector<double> ts = {};
        for(int i=0;i<sources.size();++i){
            Timer::reset_all();
            int s = sources[i];
            printf("s = %d\n",s);
            estimate.emplace_back(std::make_pair(s,solver(s)));
            printf("EVALUATE time: %lf\n", Timer::used(TIMER::EVALUATE));
            ts.emplace_back(Timer::used(TIMER::EVALUATE));
        }
        double avg_err = avg_singlesource_err(truth,estimate,l1_err);
        if(avg_err < 0){
            return -1;
        }
        double avg_t = avg(ts);
        std::cout << std::setprecision(16) << eps << "\t" << avg_t << "\t" << avg_err << std::endl;
        outfile << std::setprecision(16) << eps << "\t" << avg_t << "\t" << avg_err << std::endl;
    }
    
    printf("Saved to %s\n", savepath.c_str());

    delete f;
    delete I;
    delete G;
    int a;
    return 0;
}
