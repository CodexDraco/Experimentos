#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

int print_fib(int iterations) {
  int64_t fib, fib_1 = 1, fib_2 = 1;
  for (int i = 1; i <= iterations; i++) {
    if (i == 1 || i == 2)
      std::cout << 1 << '\n';
    else {
      if (INT64_MAX - fib_1 - fib_2 < 0) {
        std::cerr << "Integer overflow, stopped at " << i << " iterations."
                  << std::endl;
        return EXIT_FAILURE;
      }
      fib = fib_1 + fib_2;
      fib_2 = fib_1;
      fib_1 = fib;
      std::cout << fib << '\n';
    }
  }
  return EXIT_SUCCESS;
}

int main(int argc, const char *argv[]) {
  if (argc != 2) {
    std::cerr << "Needs one argument: Ex: " << argv[0] << " 10" << std::endl;
    return EXIT_FAILURE;
  }

  int iterations = 0;
  std::string_view s(argv[1]);
  auto [ptr, ec] = std::from_chars(s.begin(), s.end(), iterations);

  if (ec != std::errc() || ptr != s.end()) {
    std::cerr << "Unable to parse argument: " << s << std::endl;
    return EXIT_FAILURE;
  }

  return print_fib(iterations);
}
