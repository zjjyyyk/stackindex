#pragma once

#include "graph.hpp"
#include "lib/scarray.hpp"
#include "log/log.h"
#include "time/timer.hpp"
#include "uniqueue.hpp"
#include <assert.h>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>


using ppr_vec = std::vector<double>;
using res_vec = std::vector<double>;

class Config {
public:
    bool is_dird=false;
    double alpha=0.2, eps=0.3, delta=0.1, pf=0.01, rmax=0.01;
    double beta = 1.0;
    double theta = 0.5;
    double det_exp = 1.0;
    double det_fac = 1.0;
    double pf_exp = 1.0;

public:
    Config() = default;
    Config(bool is_dird, double alpha, double eps, double delta, double pf, double rmax) : is_dird(is_dird),
                                                                                           alpha(alpha), eps(eps), delta(delta), pf(pf), rmax(rmax) {
    }

    double omega() const {
        return rmax * (2 + eps * 2 / 3) * log(2. / pf) / (eps * eps * delta);
    }

    void show(){
        fprintf(stdout, "is_dird: %d, alpha: %lf, eps: %lf, delta: %lf, pf: %lf, rmax: %lf\n", is_dird, alpha, eps, delta, pf, rmax);
    }
};

template <typename CONF>
class IndexMethod {
public:
    graph *G = nullptr;
    CONF *conf = nullptr;
    IndexMethod() = default;
    IndexMethod(graph *G, CONF *conf) : G(G), conf(conf) {}
    virtual void refine(ppr_vec &reserve, res_vec &residue) {}
    virtual void update_alpha(double alpha) {}
    virtual void update_insert(node_id u, node_id v, edge_sno es) {}
    virtual void update_delete(node_id u, node_id v, edge_sno es) {}
};

template <typename CONF>
class FORA {
private:
    using outputer = std::function<void(const std::vector<double> &)>;

    void _forward_push(IndexMethod<CONF> *f, node_id s, ppr_vec &rsv, ppr_vec &rsd) {
        // forward-push
        graph *G = f->G;
        static uniqueue push_queue(G->num_nodes());
        double rmax = f->conf->rmax;
        Timer tmr(TIMER::PUSH);

        if (G->is_dangling_node(s)){
            rsv[s] = 1.0;
            return ;
        } 
        else {
            rsd[s] = 1.0;
            if (rsd[s] >= rmax * G->get_degree(s))
                push_queue.push(s);
        }
        while (!push_queue.empty()) {
            node_id u = push_queue.pop();
            rsv[u] += f->conf->alpha * rsd[u];
            // dangling node cannot be in queue
            double detr = (1 - f->conf->alpha) * rsd[u] / G->get_degree(u);
            log_trace("on node %zu, rsd = %e, inc = %e", (size_t)u, rsd[u], detr);
            rsd[u] = 0;

            for (node_id v : G->get_neighbourhood(u)) {
                // push method will not be invoked at dangling node
                if (G->is_dangling_node(v))
                    rsv[v] += detr;
                else {
                    rsd[v] += detr;
                    if (rsd[v] >= rmax * G->get_degree(v))
                        push_queue.push(v);
                }
            }
        }
    }

    void _refine(IndexMethod<CONF> *f, ppr_vec &rsv, ppr_vec &rsd) {
        Timer tmr(TIMER::REFINE);
        f->refine(rsv, rsd);
    }

    void _evaluate(IndexMethod<CONF> *f, node_id s, ppr_vec &rsv, ppr_vec &rsd) {
        Timer tmr(TIMER::EVALUATE);

        log_debug("forward pushing");
        _forward_push(f, s, rsv, rsd);

        log_debug("refining estimation");
        _refine(f, rsv, rsd);
    }

    void _output(outputer output, const ppr_vec &ppr) {
        Timer tmr(TIMER::OUTPUT);
        output(ppr);
    }

public:
    void evaluate_noprint(IndexMethod<CONF> *f, node_id s, outputer output) {
        ppr_vec rsv(f->G->num_nodes(), 0);
        ppr_vec rsd(f->G->num_nodes(), 0);

        _evaluate(f, s, rsv, rsd);
        _output(output, rsv);
    }

    void evaluate(IndexMethod<CONF> *f, node_id s, outputer output) {
        ppr_vec rsv(f->G->num_nodes(), 0);
        ppr_vec rsd(f->G->num_nodes(), 0);

        _evaluate(f, s, rsv, rsd);
        _output(output, rsv);
        fprintf(stdout, "evaluation time: %lf\n", Timer::used(TIMER::EVALUATE));
        fprintf(stdout, "forward-push time: %lf\n", Timer::used(TIMER::PUSH));
        fprintf(stdout, "refine time: %lf\n", Timer::used(TIMER::REFINE));
    }

    void insert_edge(node_id u, node_id v, IndexMethod<CONF> *I) {
        Timer tmr(TIMER::UPDATE);
        std::optional<edge_sno> esno = I->G->insert_edge(u, v);
        if (!esno) return;
        I->update_insert(u, v, esno.value());
        if (!I->conf->is_dird && u != v) {
            esno = I->G->insert_edge(v, u);
            if (!esno) {
            log_fatal("fail to insert duel edge <%zu, %zu>", (size_t)u, (size_t)v);
            exit(1);
            }
            I->update_insert(v, u, esno.value());
        }
    }

  void delete_edge(node_id u, node_id v, IndexMethod<CONF> *I) {
    Timer tmr(TIMER::UPDATE);
    std::optional<edge_sno> esno = I->G->delete_edge(u, v);
    if (!esno) return;
    I->update_delete(u, v, esno.value());
    if (!I->conf->is_dird && u != v) {
      esno = I->G->delete_edge(v, u);
      if (!esno) {
        log_fatal("fail to delete duel edge <%zu, %zu>", (size_t)u, (size_t)v);
        exit(1);
      }
      I->update_delete(v, u, esno.value());
    }
  }
};
