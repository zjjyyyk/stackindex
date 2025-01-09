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


class RealTimeIndex:public simple_walk, public IndexMethod<Config>{
private:
    
public:
    RealTimeIndex(graph *G, Config *conf) : IndexMethod(G, conf) {
        
    }

    void refine(ppr_vec &reserve, res_vec &residue) {
        size_t num_walks = conf->omega();

        for (node_id i = 0; i < G->num_nodes(); i++) {
            for(size_t _ = 0; _ < num_walks; _++){
                reserve[random_walk(G,i,conf->alpha)] += residue[i] / num_walks;
            }
        }
    }

    void update_alpha(double alpha) {
        Timer tmr(TIMER::UPDATE);
        conf->alpha = alpha;
    }
};