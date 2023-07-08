#include <iostream>
import mod;

int main() {
  auto vec = fill_vector(20);

  for (auto d : vec)
    std::cout << d << '\n';
}
