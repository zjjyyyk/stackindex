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
    int min_args = 5;
    if (argc < min_args) {
        fprintf(stderr, "Usage: %s <dataset> <method> <truthdir> <savedir>\n", argv[0]);
        return 1;
    }

    bool force = false;
    for (int i = min_args; i < argc; i++) {
        if (strcmp(argv[i], "--force") == 0) {
            force = true;
            break;
        }
    }

    // method can be "stackindex", "rwindex", "realtime"
    std::string method(argv[2]);

    std::string truthdir(argv[3]);
    std::string savedir(argv[4]);
    ensure_dir(savedir);
    std::string savepath = savedir + "/" + method + ".txt";


    // std::vector<double> alphas = {0.7,0.5,0.3,0.25,0.2,0.15,0.05,0.01}; // alphas must be decreasing.
    std::vector<double> alphas = {0.3,0.25,0.2,0.15,0.05,0.01}; // alphas must be decreasing.
    // std::vector<double> alphas = {0.7}; // alphas must be decreasing.

    std::vector<double> done = {};
    if(!force){
        done = read_done<double>(savepath);
    }

    printf("done: %zu\n",done.size());
    if(done.size() == alphas.size()){
        std::sort(done.begin(), done.end(), std::greater<double>());
        bool all_done = true;
        for(int i=0;i<done.size();++i){
            if(std::abs(done[i]-alphas[i]) > 1e-7){
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

    // Config C(true, 0.2, 0.1, 0.05, 0.05, 0.01);
    Config C(true, 0.2, 0.3, 0.1, 0.01, 0.01); // precision (0.3, 0.1, 0.01, 0.01 fixed to make omega 12)
    graph *G = read_graph(argv[1],C);
    FORA<Config> * f = new FORA<Config>; 
    IndexMethod<Config> * I;

    std::vector<double> res;
    auto outputer = [&](const std::vector<double> & ppr){
        res = std::move(ppr);
    };

    for(size_t i=done.size();i < alphas.size();++i)
    {
        double alpha = alphas[i];
        printf("alpha: %lf\n",alpha);
        double t;
        // Define singlesource Solver
        if(i == done.size()){
            C.alpha = alpha;
            Timer::reset_all();
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
            t = Timer::used(TIMER::BUILD);
            printf("Time:%lf\n",t);
        } else{
            Timer::reset_all();
            ////////////////////////////////////////////////////////////////////// CORE ///////////////////////////////////////////
            I->update_alpha(alpha);
            t = Timer::used(TIMER::UPDATE);
        }
        
        auto solver = [&](int s){
            f->evaluate_noprint(I, s, outputer);
            // printf("s = %d\n",s);
            // printf("[");
            // for(int j = 0; j < res.size(); j++){
            //     printf("%lf, ",res[j]);
            // }
            // printf("]\n");
            return res;
        };

        std::string truthdir_alpha = truthdir + "/" + formatFloat(alpha);
        std::vector<std::pair<int,std::vector<double>>> truth = read_singlesource_res(truthdir_alpha);
        std::vector<int> sources = {};
        for(auto& [s,ppr]: truth){
            sources.emplace_back(s);
        }

        std::vector<std::pair<int,std::vector<double>>> estimate = {};

        for(int i=0;i<sources.size();++i){
            Timer::reset_all();
            int s = sources[i];
            // printf("s = %d\n",s);
            estimate.emplace_back(std::make_pair(s,solver(s)));
            // printf("EVALUATE time: %lf\n", Timer::used(TIMER::EVALUATE));
        }
        double avg_err = avg_singlesource_err(truth,estimate,l1_err);
        if(avg_err < 0){
            return -1;
        }
        
        std::cout << std::setprecision(16) << alpha << "\t" << t << "\t" << avg_err << std::endl;
        outfile << std::setprecision(16) << alpha << "\t" << t << "\t" << avg_err << std::endl;
    }
    
    printf("Saved to %s\n", savepath.c_str());

    delete f;
    delete I;
    delete G;
    // int a;
    return 0;
}
