#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <utility>

constexpr auto checked_sum(auto a, auto b) {
  auto c = a;
  bool o = __builtin_add_overflow(a, b, &c);
  return std::make_pair(c, o);
}

int main() {
  auto a = 0;
  auto b = 0;

  while (!std::cin.eof()) {
    // std::string s;
    std::cin >> a;
    std::cin >> b;

    auto [r, o] = checked_sum(a, b);

    std::cout << r << ' ' << std::boolalpha << o << '\n';
  }
}
