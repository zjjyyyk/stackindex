#pragma once

#include <cmath>
#include "graph.hpp"

// basic arguments of indexing schemes
class fspi_base {
protected:
  graph* const _g;
  const bool _is_dird;

public:
  const double alpha, beta, eps, det, pf;

protected:
  record_sno index_size(node_id v) const {
    return ceil(beta * (1 - alpha) * _g->get_degree(v));
  }

public:
  template <typename C>
  fspi_base(graph* g, bool is_dird, C config) :
    _g(g), _is_dird(is_dird),
    alpha(config.alpha), beta(config.beta),
    eps(config.eps),
    det(config.det_fac * pow(g->num_nodes(), -config.det_exp)),
    pf(pow(g->num_nodes(), -config.pf_exp)) { }

  double omega(double delta) const {
    return (2 + eps * 2 / 3) * log(2. / pf) / (eps * eps * delta);
  }

  double rmax(double delta) const {
    return beta / ceil(omega(delta));
  }

  record_sno num_samples(node_id v, double r, double delta) const {
    return ceil((1 - alpha) * r * omega(delta));
  }
};