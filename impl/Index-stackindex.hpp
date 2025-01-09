#pragma once

#include "fora_skeleton.hpp"
#include "graph.hpp"
#include "lib/ConvenientPrint.hpp"
#include "lib/random.hpp"
#include "log/log.h"
#include "time/timer.hpp"
#include "uniqueue.hpp"
#include <algorithm>
#include <assert.h>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <random>
#include <cmath>
#include <bits/stdc++.h>



class StackIndex : public IndexMethod<Config> {
public:
    class Stack{
    public:
        size_t top = 0;
        std::vector<node_id> column;
        node_id& operator[](size_t index){
            if (index >= column.size()) {
                fprintf(stdout, "Index: %zu, size: %zu\n", index, column.size());
                throw std::out_of_range("Index of Class Stack out of range");
            }
            return column[index];
        }
        void push(node_id v){
            if(top == column.size()){
                column.emplace_back(v);
                top++;
            } else{
                column[top]= v;
                top++;
            }
            
        }
        void set_top(size_t t){
            if(t>column.size()){
                fprintf(stdout, "new top: %zu, size: %zu\n", t, column.size());
                throw std::out_of_range("set_top error");
            }
            top = t;
        }
    };

    class StackTree {
    public:
        std::vector<Stack> _stacktree;
        std::vector<node_id> next;
        std::vector<node_id> root;
        std::vector<double> vol;

        std::vector<node_id> aux_traverse;
        std::vector<node_id> aux_last;

        StackTree() {}
        StackTree(node_id num_nodes) : _stacktree(num_nodes,Stack()), next(num_nodes, -1), root(num_nodes, -1), vol(num_nodes, 0), aux_traverse(num_nodes,-1), aux_last(num_nodes,-1){}

        Stack &operator[](size_t index) {
            if (index >= _stacktree.size()) {
                fprintf(stdout, "Index: %zu, size: %zu\n", index, _stacktree.size());
                throw std::out_of_range("Index of Class StackTree out of range");
            }
            return _stacktree[index];
        }
    };

    class Index {
    public:
        std::vector<StackTree> _index;
        Index() : _index() {}
        Index(size_t num_stacks, node_id num_nodes) : _index(num_stacks, StackTree(num_nodes)) {}

        StackTree &operator[](size_t index) {
            if (index >= _index.size()) {
                throw std::out_of_range("Index of Class Index out of range");
            }
            return _index[index];
        }
    };

public:
    Index stack_index;
    size_t num_stacks = 0;

    void show_num_stacks() {
        fprintf(stdout, "num_stacks: %zu\n", num_stacks);
    }
    StackIndex() = default;
    StackIndex(graph *G, Config *conf, Index stack_index) : IndexMethod<Config>(G, conf), stack_index(stack_index), num_stacks(conf->omega()) {}
    StackIndex(graph *G, Config *conf) : IndexMethod<Config>(G, conf) {
        conf->show();

        printf("Building StackIndex\n");
        double alpha = conf->alpha;
        if(num_stacks == 0) num_stacks = conf->omega();
        printf("omega: %zu\n", num_stacks);
        stack_index = Index(num_stacks, G->num_nodes());
        std::vector<bool> intree(G->num_nodes(),false);
        printf("G->n:%d\n",G->num_nodes());

        Timer tmr(TIMER::BUILD);
        for (size_t i = 0; i < num_stacks; i++) {
            printf("Building StackTree %zu\n", i);
            StackTree stacktree(G->num_nodes());
            // stacktree.set_end_geometry(alpha);
            std::fill(intree.begin(),intree.end(),false);

            for (node_id u = 0; u < G->num_nodes(); u++) {
                if(!intree[u]){
                    if(G->is_dangling_node(u)){
                        stacktree.next[u] = -1;
                        stacktree.root[u] = u;
                        stacktree.vol[u] += G->get_degree(u);
                        stacktree.aux_last[u] = u;
                        intree[u] = true;
                        continue;
                    }
                    node_id current = u;
                    bool hit_intree = true;
                    while (!intree[current]) {
                        if (rand_uniformf() < alpha) {
                            hit_intree = false;
                            stacktree.next[current] = -1;
                            stacktree[current].push(-1);

                            stacktree.aux_last[current] = current;
                            break;
                        }
                        stacktree.next[current] = G->get_neighbour(current, rand_uniform(G->get_degree(current)));
                        stacktree[current].push(stacktree.next[current]);
                        current = stacktree.next[current];
                    }
                    node_id last = current;
                    node_id r = hit_intree ? stacktree.root[last] : last;
                    current = u;
                    while (current != last) {
                        stacktree.root[current] = r;
                        stacktree.vol[r] += G->get_degree(current);
                        intree[current] = true;

                        stacktree.aux_traverse[stacktree.aux_last[r]] = current;
                        stacktree.aux_last[r] = current;

                        current = stacktree.next[current];
                    }

                    if (!hit_intree) {
                        stacktree.root[last] = last;
                        stacktree.vol[r] += G->get_degree(current);
                        intree[last] = true;
                    }

                }
            }
            stack_index._index[i] = std::move(stacktree);
        }
        printf("StackIndex built, num_stacks: %zu\n", num_stacks);
    }


    void refine(ppr_vec &rsv, res_vec &rsd) {
        for(node_id u=0;u<G->num_nodes();u++){
            if(rsd[u] == 0) continue;
            if(G->is_dangling_node(u)){
                rsv[u] += rsd[u];
            } else{
                for(auto &stacktree : stack_index._index){
                    node_id r = stacktree.root[u];
                    double vol = stacktree.vol[r];
                    node_id v = r;
                    for(;v!=stacktree.aux_last[r];v=stacktree.aux_traverse[v]){
                        rsv[v] += rsd[u] * G->get_degree(v) / (vol * stack_index._index.size());
                    }
                    rsv[v] += rsd[u] * G->get_degree(v) / (vol * stack_index._index.size());
                }
            }
        }
    }

    void update_alpha(double alpha) {
        printf("StackIndex alpha updated\n");
    }

    
    void update_insert(node_id a, node_id b, edge_sno){
        double alpha = conf->alpha;
        std::vector<bool> intree(G->num_nodes(),false);
        std::vector<size_t> seen(G->num_nodes(),0);


        for(size_t i =0;i<stack_index._index.size();i++){
            StackTree &stacktree = stack_index._index[i];
            Stack &s = stacktree[a];
            if(s.column.size() == 0) continue;
            uint32_t first_appear_index = rand_geometric(1.0/G->get_degree(a)) - 1;
            if(first_appear_index > s.column.size() - 1) continue;
            s[first_appear_index] = b;
            s.set_top(first_appear_index+1);
            std::fill(stacktree.vol.begin(),stacktree.vol.end(),0);
            std::fill(intree.begin(),intree.end(),false);
            std::fill(seen.begin(),seen.end(),0);
            
            for (node_id u = 0; u < G->num_nodes(); u++) {
                if(!intree[u]){
                    if(G->is_dangling_node(u)){
                        stacktree.next[u] = -1;
                        stacktree.root[u] = u;
                        stacktree.vol[u] += G->get_degree(u);
                        stacktree.aux_last[u] = u;
                        intree[u] = true;
                        break;
                    }
                    node_id current = u;
                    bool hit_intree = true;

                    while (!intree[current]) {
                        if(seen[current]<stacktree[current].top){
                            stacktree.next[current] = stacktree[current][seen[current]];
                            seen[current]++;
                            if(stacktree.next[current] == -1){
                                hit_intree = false;
                                stacktree.aux_last[current] = current;
                                break;
                            }
                        } else{
                            if (rand_uniformf() < alpha) {
                                hit_intree = false;
                                stacktree.next[current] = -1;
                                stacktree[current].push(-1);
                                seen[current]++;

                                stacktree.aux_last[current] = current;
                                break;
                            }
                            stacktree.next[current] = G->get_neighbour(current, rand_uniform(G->get_degree(current)));
                            stacktree[current].push(stacktree.next[current]);
                            seen[current]++;
                            current = stacktree.next[current];
                        }
                        
                    }

                    node_id last = current;
                    node_id r = hit_intree ? stacktree.root[last] : last;
                    current = u;

                    while (current != last) {
                        stacktree.root[current] = r;
                        stacktree.vol[r] += G->get_degree(current);
                        intree[current] = true;

                        stacktree.aux_traverse[stacktree.aux_last[r]] = current;
                        stacktree.aux_last[r] = current;

                        current = stacktree.next[current];
                    }

                    if (!hit_intree) {
                        stacktree.root[last] = last;
                        stacktree.vol[r] += G->get_degree(current);
                        intree[last] = true;

                    }

                }
            }
        }
    }



    void update_delete(node_id a, node_id b, edge_sno){
        double alpha = conf->alpha;
        std::vector<bool> intree(G->num_nodes(),false);
        std::vector<size_t> seen(G->num_nodes(),0);


        for(size_t i =0;i<stack_index._index.size();i++){
            StackTree &stacktree = stack_index._index[i];
            Stack &s = stacktree[a];
            bool involved = false;
            for(size_t j=0;j<s.top;j++){
                if(s[j] == b){
                    involved = true;
                    s.set_top(j);
                    break;
                }
            }
            if(!involved) continue;;
            std::fill(stacktree.vol.begin(),stacktree.vol.end(),0);
            std::fill(intree.begin(),intree.end(),false);
            std::fill(seen.begin(),seen.end(),0);
            
            for (node_id u = 0; u < G->num_nodes(); u++) {
                if (G->is_dangling_node(u)) {
                    stacktree[u].push(-1);
                } else {
                    node_id current = u;
                    bool hit_intree = true;

                    while (!intree[current]) {
                        if(seen[current]<stacktree[current].top){
                            stacktree.next[current] = stacktree[current][seen[current]];
                            seen[current]++;
                            if(stacktree.next[current] == -1){
                                hit_intree = false;
                                stacktree.aux_last[current] = current;
                                break;
                            }
                        } else{
                            if (rand_uniformf() < alpha) {
                                hit_intree = false;
                                stacktree.next[current] = -1;
                                stacktree[current].push(-1);
                                seen[current]++;

                                stacktree.aux_last[current] = current;
                                break;
                            }
                            stacktree.next[current] = G->get_neighbour(current, rand_uniform(G->get_degree(current)));
                            stacktree[current].push(stacktree.next[current]);
                            seen[current]++;
                            current = stacktree.next[current];
                        }
                        
                    }

                    node_id last = current;
                    node_id r = hit_intree ? stacktree.root[last] : last;
                    current = u;

                    while (current != last) {
                        stacktree.root[current] = r;
                        stacktree.vol[r] += G->get_degree(current);
                        intree[current] = true;

                        stacktree.aux_traverse[stacktree.aux_last[r]] = current;
                        stacktree.aux_last[r] = current;

                        current = stacktree.next[current];
                    }

                    if (!hit_intree) {
                        stacktree.root[last] = last;
                        stacktree.vol[r] += G->get_degree(current);
                        intree[last] = true;

                    }

                }
            }
        }
    }
    
};


class StackIndex_Realtime : public StackIndex {
public:
    StackIndex_Realtime(graph *G, Config *conf):StackIndex(G,conf){}

    void update_insert(node_id a, node_id b, edge_sno){

        double alpha = conf->alpha;
        stack_index = Index(num_stacks, G->num_nodes());
        std::vector<bool> intree(G->num_nodes(),false);

        Timer tmr(TIMER::BUILD);
        for (size_t i = 0; i < num_stacks; i++) {
            StackTree stacktree(G->num_nodes());
            // stacktree.set_end_geometry(alpha);
            std::fill(intree.begin(),intree.end(),false);

            for (node_id u = 0; u < G->num_nodes(); u++) {
                if(!intree[u]){
                    if(G->is_dangling_node(u)){
                        stacktree.next[u] = -1;
                        stacktree.root[u] = u;
                        stacktree.vol[u] += G->get_degree(u);
                        stacktree.aux_last[u] = u;
                        intree[u] = true;
                        continue;
                    }
                    node_id current = u;
                    bool hit_intree = true;
                    while (!intree[current]) {
                        if (rand_uniformf() < alpha || G->is_dangling_node(current)) {
                            hit_intree = false;
                            stacktree.next[current] = -1;
                            stacktree[current].push(-1);

                            stacktree.aux_last[current] = current;
                            break;
                        }
                        stacktree.next[current] = G->get_neighbour(current, rand_uniform(G->get_degree(current)));
                        stacktree[current].push(stacktree.next[current]);
                        current = stacktree.next[current];
                    }
                    node_id last = current;
                    node_id r = hit_intree ? stacktree.root[last] : last;
                    current = u;
                    while (current != last) {
                        stacktree.root[current] = r;
                        stacktree.vol[r] += G->get_degree(current);
                        intree[current] = true;

                        stacktree.aux_traverse[stacktree.aux_last[r]] = current;
                        stacktree.aux_last[r] = current;

                        current = stacktree.next[current];
                    }

                    if (!hit_intree) {
                        stacktree.root[last] = last;
                        stacktree.vol[r] += G->get_degree(current);
                        intree[last] = true;
                    }

                }
            }
            stack_index._index[i] = std::move(stacktree);
        }
    }  

    void update_delete(node_id a, node_id b, edge_sno _){
        update_insert(a,b,_);
    }
    
};



class StackIndex_Static : public IndexMethod<Config> {
public:
    class StackTree {
    public:
        std::vector<node_id> next;
        std::vector<node_id> root;
        std::vector<double> vol;

        std::vector<node_id> aux_traverse;
        std::vector<node_id> aux_last;

        StackTree() {}
        StackTree(node_id num_nodes) :  next(num_nodes, -1), root(num_nodes, -1), vol(num_nodes, 0), aux_traverse(num_nodes,-1), aux_last(num_nodes,-1){}
    };

    class Index {
    public:
        std::vector<StackTree> _index;
        Index() : _index() {}
        Index(size_t num_stacks, node_id num_nodes) : _index(num_stacks, StackTree(num_nodes)) {}

        StackTree &operator[](size_t index) {
            if (index >= _index.size()) {
                throw std::out_of_range("Index of Class Index out of range");
            }
            return _index[index];
        }
    };

public:
    Index stack_index;
    size_t num_stacks = 0;

    void show_num_stacks() {
        fprintf(stdout, "num_stacks: %zu\n", num_stacks);
    }
    StackIndex_Static() = default;
    StackIndex_Static(graph *G, Config *conf, Index stack_index) : IndexMethod<Config>(G, conf), stack_index(stack_index), num_stacks(conf->omega()) {}
    StackIndex_Static(graph *G, Config *conf) : IndexMethod<Config>(G, conf) {
        conf->show();

        printf("Building StackIndex\n");
        double alpha = conf->alpha;
        if(num_stacks == 0) num_stacks = conf->omega();
        printf("omega: %zu\n", num_stacks);
        stack_index = Index(num_stacks, G->num_nodes());
        std::vector<bool> intree(G->num_nodes(),false);

        Timer tmr(TIMER::BUILD);
        for (size_t i = 0; i < num_stacks; i++) {
            printf("Building StackTree %zu\n", i);
            StackTree stacktree(G->num_nodes());
            // stacktree.set_end_geometry(alpha);
            std::fill(intree.begin(),intree.end(),false);

            for (node_id u = 0; u < G->num_nodes(); u++) {
                if(!intree[u]){
                    if(G->is_dangling_node(u)){
                        stacktree.next[u] = -1;
                        stacktree.root[u] = u;
                        stacktree.vol[u] += G->get_degree(u);
                        stacktree.aux_last[u] = u;
                        intree[u] = true;
                        continue;
                    }
                    node_id current = u;
                    bool hit_intree = true;

                    while (!intree[current]) {
                        if (rand_uniformf() < alpha) {
                            hit_intree = false;
                            stacktree.next[current] = -1;

                            stacktree.aux_last[current] = current;
                            break;
                        }
                        stacktree.next[current] = G->get_neighbour(current, rand_uniform(G->get_degree(current)));
                        current = stacktree.next[current];
                    }

                    node_id last = current;
                    node_id r = hit_intree ? stacktree.root[last] : last;
                    current = u;

                    while (current != last) {
                        stacktree.root[current] = r;
                        stacktree.vol[r] += G->get_degree(current);
                        intree[current] = true;

                        stacktree.aux_traverse[stacktree.aux_last[r]] = current;
                        stacktree.aux_last[r] = current;

                        current = stacktree.next[current];
                    }

                    if (!hit_intree) {
                        stacktree.root[last] = last;
                        stacktree.vol[r] += G->get_degree(current);
                        intree[last] = true;

                    }
                }
                

            }
            stack_index._index[i] = std::move(stacktree);
        }
        printf("StackIndex built, num_stacks: %zu\n", num_stacks);
    }


    void refine(ppr_vec &rsv, res_vec &rsd) {
        for(node_id u=0;u<G->num_nodes();u++){
            if(rsd[u] == 0) continue;
            if(G->is_dangling_node(u)){
                rsv[u] += rsd[u];
            } else{
                for(auto &stacktree : stack_index._index){
                    node_id r = stacktree.root[u];
                    double vol = stacktree.vol[r];
                    node_id v = r;
                    for(;v!=stacktree.aux_last[r];v=stacktree.aux_traverse[v]){
                        rsv[v] += rsd[u] * G->get_degree(v) / (vol * stack_index._index.size());
                    }
                    rsv[v] += rsd[u] * G->get_degree(v) / (vol * stack_index._index.size());
                }
            }
        }
    }

    void update_alpha(double alpha) {
        Timer tmr(TIMER::UPDATE);

        double old_alpha = conf->alpha;
        conf->alpha = alpha;
        double prob = 1 - alpha / old_alpha;
        printf("prob: %lf\n", prob);

        uniqueue active_p_queue(G->num_nodes());
        std::vector<int> status(G->num_nodes(),0);

        for(auto & stacktree : stack_index._index){
            // printf("StackIndex updating\n");
            active_p_queue.clear();
            
            for(node_id u=0;u<G->num_nodes();u++){
                if(stacktree.next[u]==-1 && !G->is_dangling_node(u)){
                    if(rand_uniformf()<prob){
                        // outtree选出的root的子树
                        active_p_queue.push(u);
                        stacktree.next[u] = G->get_neighbour(u, rand_uniform(G->get_degree(u)));
                        status[u] = -1;
                    } else{
                        status[u] = 1;
                    }
                } else{
                    status[u] = 0;
                }
            }

            for(node_id u=0;u<G->num_nodes();u++){
                if(status[stacktree.root[u]]==1){
                    status[u] = 1;
                }
            }

            // 从active_p_queue中取出一个p处理：
            while(!active_p_queue.empty()){
                node_id u = active_p_queue.pop();
                node_id p = u;
                while(status[stacktree.next[p]]==0){
                    p = stacktree.next[p];
                }

                if(status[stacktree.next[p]]==1){
                    node_id r = stacktree.root[stacktree.next[p]];       
                    p = u;
                    while(status[stacktree.next[p]]!=1){
                        status[p] = 1;
                        stacktree.root[p] = r;
                        stacktree.vol[r] += G->get_degree(p);

                        stacktree.aux_traverse[stacktree.aux_last[r]] = p;
                        stacktree.aux_last[r] = p;

                        p = stacktree.next[p];
                        
                    }
                    status[p] = 1;
                    stacktree.root[p] = r;
                    stacktree.vol[r] += G->get_degree(p);

                    stacktree.aux_traverse[stacktree.aux_last[r]] = p;
                    stacktree.aux_last[r] = p;

                } else if(stacktree.next[p]==u){    
                    p = u;
                    while(stacktree.next[p]!=u){
                        node_id np = stacktree.next[p];
                        if(rand_uniformf()<alpha){
                            stacktree.next[p] = -1;
                            status[p] = 1;
                            stacktree.root[p] = p;
                            stacktree.vol[p] += G->get_degree(p);

                            stacktree.aux_last[p] = p;
                        } else{
                            stacktree.next[p] = G->get_neighbour(p, rand_uniform(G->get_degree(p)));
                            status[p] = -1;
                            active_p_queue.push(p);
                        }
                        p = np;
                    }
                    if(rand_uniformf()<alpha){
                        stacktree.next[p] = -1;
                        status[p] = 1;
                        stacktree.root[p] = p;
                        stacktree.vol[p] += G->get_degree(p);

                        stacktree.aux_last[p] = p;
                    } else{
                        stacktree.next[p] = G->get_neighbour(p, rand_uniform(G->get_degree(p)));
                        status[p] = -1;
                        active_p_queue.push(p);
                    }
                } else{
                    status[u] = 0;
                }
                

            }

            for(node_id u=0;u<G->num_nodes();u++){
                if(status[u]==0){
                    node_id p = u;
                    while(status[p]==0){
                        p = stacktree.next[p];
                    }
                    node_id r = stacktree.root[p];
                    p = u;
                    while(status[p]==0){
                        status[p] = 1;
                        stacktree.root[p] = r;
                        stacktree.vol[r] += G->get_degree(p);

                        stacktree.aux_traverse[stacktree.aux_last[r]] = p;
                        stacktree.aux_last[r] = p;

                        p = stacktree.next[p];
                    }
                }
            }
            
        }
            
        printf("StackIndex updated, num_stacks: %zu\n", num_stacks);
    }
};

