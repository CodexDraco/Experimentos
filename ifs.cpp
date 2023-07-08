#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>

int main(const int argc, const char *argv[]) {
  for (int i = 1; i < argc; i++) {
    std::cerr << "Opening '" << argv[1] << "'.\n";
    std::ifstream file;
    file.open(argv[i]);
    if (file.is_open()) {
      while (file.good()) {
        char c;
        file.get(c);
        if (c < 127)
          std::cout.put(c);
        else
          std::cout.put('.');
      }
    } else {
      std::cerr << "Error opening '" << argv[i] << "': " << strerror(errno)
                << '\n';
    }
  }
}
