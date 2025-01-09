#pragma once

#include <assert.h>
#include "log/log.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <cstdio>
#include <algorithm>
#include "lib/scarray.hpp"
#include "lib/random.hpp"
#include "uniqueue.hpp"
#include "graph.hpp"
#include "time/timer.hpp"
#include "fora_skeleton.hpp"
#include "simple_walk.hpp"


class RwIndex:public simple_walk, public IndexMethod<Config>{
private:
    std::vector<std::vector<node_id>> records;
    size_t num_walks = 0;
    
public:
    RwIndex(graph *G, Config *conf) : IndexMethod(G, conf) {
        records = std::vector<std::vector<node_id>>(G->num_nodes(),std::vector<node_id>());
        if(num_walks == 0) num_walks = conf->omega();
        printf("num_walks: %zu\n", num_walks);

        Timer tmr(TIMER::BUILD);
        for (node_id i = 0; i < G->num_nodes(); i++) {
            if(i % 10000 == 0){
                // printf("i: %u\n", i);
            }
            for(size_t _ = 0; _ < num_walks; _++){
                records[i].push_back(random_walk(G, i, conf->alpha));
            }
        }
    }

    void refine(ppr_vec &reserve, res_vec &residue) {
        for(node_id i = 0; i < G->num_nodes(); i++){
            for(auto ter:records[i]){
                reserve[ter] += residue[i] / num_walks;
            }
        }
    }

    void update_alpha(double alpha) {
        Timer tmr(TIMER::UPDATE);

        conf->alpha = alpha;
        records = std::vector<std::vector<node_id>>(G->num_nodes(),std::vector<node_id>());
        for (node_id i = 0; i < G->num_nodes(); i++) {
            if(i % 10000 == 0){
                // printf("i: %u\n", i);
            }
            for(size_t _ = 0; _ < num_walks; _++){
                records[i].push_back(random_walk(G, i, conf->alpha));
            }
        }
    }

};