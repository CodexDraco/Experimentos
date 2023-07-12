#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <utility>

template <typename T> class fib_gen {
private:
  T fib_1 = 0;
  T fib_2 = 1;

public:
  constexpr auto next() {
    T fib;
    bool overflow = __builtin_add_overflow(fib_1, fib_2, &fib);
    fib_2 = fib_1;
    fib_1 = fib;
    return std::make_pair(fib, overflow);
  };
};

bool print_fib(const int iterations) {
  fib_gen<uint64_t> fib;

  for (int i = 1; i <= iterations; i++) {
    auto [f, ov] = fib.next();
    if (!ov) {
      std::cout << std::format("{}\n", f);
    } else {
      std::cerr << std::format("Stoped at {} iterations: Integer overflow.\n",
                               i);
      return false;
    }
  }

  return true;
}

int main(int argc, const char *argv[]) {
  if (argc != 2) {
    std::cerr << std::format("Needs one argument: {} 10\n", argv[0]);
    return EXIT_FAILURE;
  }

  int iterations = 0;
  std::string_view s(argv[1]);
  auto const [ptr, ec] = std::from_chars(s.begin(), s.end(), iterations);

  if (ec != std::errc() || ptr != s.end()) {
    std::cerr << std::format("Unable to parse argument: {}\n", s);
    return EXIT_FAILURE;
  }

  return print_fib(iterations) ? EXIT_SUCCESS : EXIT_FAILURE;
}
