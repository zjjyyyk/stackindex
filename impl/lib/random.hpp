#pragma once

#include <cmath>
#include <cstdint>
#include <random>

std::mt19937 rand_uint{(std::random_device())()};

double rand_uniformf() {
  return 0x1.0p-32 * rand_uint();
}

uint32_t rand_uniform(uint32_t n) {
  uint64_t m = (uint64_t)rand_uint() * n;
  if ((uint32_t)m < n) {
    uint32_t t = -n;
    if (t >= n) t -= n;
    if (t >= n) t %= n;
    while ((uint32_t)m < t)
      m = (uint64_t)rand_uint() * n;
  }
  return m >> 32;
}

uint32_t rand_geometric(double p) {
  if (p == 1) return 1;
  uint32_t x = rand_uint();
  while (x == 0)
    x = rand_uint();
  double u = 0x1.0p-32 * x;
  return (uint32_t)ceil(std::log(u) / std::log(1 - p));
}

uint32_t rand_binomial(uint32_t n, double p) {
  if (p == 0) return 0;
  if (p == 1) return n;
  uint32_t k = 0;
  for (uint64_t i = 0; i <= n; ++k)
    i += rand_geometric(p);
  return k - 1;
}
