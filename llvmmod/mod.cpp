module;

#include <vector>

export module mod;

export auto fill_vector(int n) {
  auto result = std::vector<int>{};
  for (int i = 0; i < n; i++)
    result.push_back(i * n);

  return result;
}
