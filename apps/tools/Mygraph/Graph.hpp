#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <random>
#include <queue>
#include <algorithm>
#include "lib/random.hpp"
#include "lib/io/ConvenientPrint.hpp"

typedef uint64_t nodeint;

using Random = effolkronium::basic_random_static<std::mt19937>;

class Graph{
public:
    std::vector<std::vector<nodeint>> g;
    std::vector<nodeint> deg;
    nodeint n;
    double m;
    std::string name;
    void show() const {
        std::cout << "Graph Details:" << std::endl;
        std::cout << "n: " << n << std::endl;
        std::cout << "m: " << m << std::endl;
        if(n < 1000){
            std::cout << "g: " << g << std::endl;
            std::cout << "deg: " << deg << std::endl;
        }
    }
} ;


class EdgeGraph : public Graph{
public:
    virtual inline nodeint choose_neighbor(const nodeint node) const;
    std::pair<std::vector<double>,std::vector<double>> v_absorbed_push(nodeint s,nodeint v,double rmax) const ;
    std::pair<std::vector<double>,std::vector<double>> v_absorbed_push(const std::vector<double>& s,nodeint v,double rmax) const;
    std::pair<std::vector<double>, std::vector<double>> vl_absorbed_push(nodeint s, const std::vector<nodeint>& in_vl, double rmax) const;
    std::pair<std::vector<double>, std::vector<double>> forward_push(nodeint s, double rmax, double alpha) const;
    std::vector<nodeint> BFS(nodeint root) const ;
    std::vector<nodeint> BFS(const std::vector<nodeint>& roots) const;
    std::vector<nodeint> wilson(nodeint r) const;
    std::vector<nodeint> wilson(std::vector<nodeint> rl) const;
} ;

inline nodeint EdgeGraph::choose_neighbor(const nodeint node) const
{
    return g[node][Random::get<nodeint>() % g[node].size()];
}

std::pair<std::vector<double>, std::vector<double>> EdgeGraph::v_absorbed_push(nodeint s, nodeint v, double rmax) const
{
    std::vector<double> r(n, 0.0); // Residual probabilities initialized to 0
    r[s] = 1.0;
    std::vector<double> q(n, 0.0); // Absorption probabilities initialized to 0

    if (s == v)
    {
        return make_pair(q, r);
    }

    std::queue<nodeint> push_queue;
    push_queue.push(s);
    std::vector<bool> is_inQueue(n, false);
    is_inQueue[s] = true;

    size_t pushcnt = 0;

    while (!push_queue.empty())
    {
        nodeint u = push_queue.front();
        push_queue.pop();
        is_inQueue[u] = false;
        q[u] += r[u];

        for (size_t i = 0; i < g[u].size(); ++i)
        {
            nodeint neighbor = g[u][i];
            if (neighbor != v)
            {
                r[neighbor] +=  r[u] / deg[u];
                if (r[neighbor] >= deg[neighbor] * rmax && !is_inQueue[neighbor])
                {
                    push_queue.push(neighbor);
                    is_inQueue[neighbor] = true;
                    pushcnt++;
                }
            }
        }
        r[u] = 0.0;
    }
    // cout << "in s=" << s << ",push cnt:" << pushcnt << endl;
    return make_pair(q, r);
}

std::pair<std::vector<double>, std::vector<double>> EdgeGraph::v_absorbed_push(const std::vector<double>& s, nodeint v, double rmax) const
{
    std::vector<double> r(s);
    std::vector<double> q(n, 0.0); // Absorption probabilities initialized to 0

    std::queue<nodeint> push_queue;
    std::vector<bool> is_inQueue(n, false);

    // 按r[i]/deg[i]从大到小进行push,但是这步就要O(n)
    std::vector<size_t> indexVector(r.size());
    for (size_t i = 0; i < indexVector.size(); ++i) {
        indexVector[i] = i;
    }
    std::sort(indexVector.begin(), indexVector.end(),
              [&](size_t i, size_t j) {
                  return r[i]/deg[i] > r[j]/deg[j];
              });
    for(const auto i:indexVector){
        if(r[i] <= deg[i] * rmax) break;
        push_queue.push(i);
        is_inQueue[i] = true;
    }

    size_t pushcnt = 0;

    while (!push_queue.empty())
    {
        nodeint u = push_queue.front();
        push_queue.pop();
        is_inQueue[u] = false;
        q[u] += r[u];

        for (size_t i = 0; i < g[u].size(); ++i)
        {
            nodeint neighbor = g[u][i];
            if (neighbor != v)
            {
                r[neighbor] +=  r[u] / deg[u];
                if (r[neighbor] >= deg[neighbor] * rmax && !is_inQueue[neighbor])
                {
                    push_queue.push(neighbor);
                    is_inQueue[neighbor] = true;
                    pushcnt++;
                }
            }
        }
        r[u] = 0.0;
    }
    // cout << "in s=" << s << ",push cnt:" << pushcnt << endl;
    return make_pair(q, r);
}

std::pair<std::vector<double>, std::vector<double>> EdgeGraph::vl_absorbed_push(nodeint s, const std::vector<nodeint> &in_vl, double rmax) const // in_vl可以是index_vl (见 resistance-singlepair.cpp)
{
    std::vector<double> r(n, 0.0);
    std::vector<double> q(n, 0.0);
    r[s] = 1.0;
    std::queue<nodeint> push_queue;
    push_queue.push(s);
    std::vector<bool> is_inQueue(n, false);
    is_inQueue[s] = true;

    while (!push_queue.empty())
    {
        int u = push_queue.front();
        push_queue.pop();
        is_inQueue[u] = false;
        q[u] += r[u];

        for (size_t i = 0; i < g[u].size(); ++i)
        {
            int neighbor = g[u][i];
            if (!(in_vl[neighbor]>0))
            {
                r[neighbor] += r[u] / deg[u];
                if (r[neighbor] >= deg[neighbor] * rmax && !is_inQueue[neighbor])
                {
                    push_queue.push(neighbor);
                    is_inQueue[neighbor] = true;
                }
            }
        }

        r[u] = 0;
    }

    return std::make_pair(q, r);
}

std::pair<std::vector<double>, std::vector<double>> EdgeGraph::forward_push(nodeint s, double rmax, double alpha = 0.15) const
{
    std::vector<double> r(n, 0.0);
    std::vector<double> q(n, 0.0);
    r[s] = 1.0;
    std::queue<nodeint> push_queue;
    push_queue.push(s);
    std::vector<bool> is_inQueue(n, false);
    is_inQueue[s] = true;

    while (!push_queue.empty())
    {
        nodeint u = push_queue.front();
        push_queue.pop();
        is_inQueue[u] = false;
        for (auto neighbor: g[u])
        {
            r[neighbor] += (1-alpha) * r[u] / deg[u];
            if (r[neighbor] >= deg[neighbor] * rmax && !is_inQueue[neighbor])
            {
                push_queue.push(neighbor);
                is_inQueue[neighbor] = true;
            }
        }
        q[u] += alpha * r[u];
        r[u] = 0;
    }
    return std::make_pair(q, r);
}

std::vector<nodeint> EdgeGraph::BFS(nodeint root) const{
    std::vector<nodeint> bfsnext(n, -2);
    std::queue<nodeint> search_queue;
    bfsnext[root] = -1;
    search_queue.push(root);
    while (!search_queue.empty())
    {
        nodeint current = search_queue.front();
        search_queue.pop();
        for (nodeint neighbor : g[current])
        {
            if (bfsnext[neighbor] == -2)
            {
                bfsnext[neighbor] = current;
                search_queue.push(neighbor);
            }
        }
    }
    return bfsnext;
}

std::vector<nodeint> EdgeGraph::BFS(const std::vector<nodeint> &roots) const{
    std::vector<nodeint> next(n, -2);
    std::queue<nodeint> search_queue;

    for (nodeint root : roots)
    {
        next[root] = -1;
        search_queue.push(root);
    }

    while (!search_queue.empty())
    {
        nodeint current = search_queue.front();
        search_queue.pop();
        for (nodeint neighbor : g[current])
        {
            if (next[neighbor] == -2)
            {
                next[neighbor] = current;
                search_queue.push(neighbor);
            }
        }
    }
    return next;
}

std::vector<nodeint> EdgeGraph::wilson(nodeint r) const{
    std::vector<bool> inTree(n, false);
    std::vector<nodeint> next(n, -1);
    inTree[r] = true;

    for (nodeint i = 0; i < n; ++i)
    {
        nodeint u = i;
        while (!inTree[u])
        {
            next[u] = choose_neighbor(u);
            u = next[u];
        }

        u = i;
        while (!inTree[u])
        {
            inTree[u] = true;
            u = next[u];
        }
    }
    return next;
}


