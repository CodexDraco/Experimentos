#include <array>
#include <iostream>
#include <span>

import arguments;

int main(int argc, const char *argv[]) {
  auto [options, status, index] =
      parse_arguments(std::span(argv + 1, argc - 1));
  /*if (argc > 1) {
    auto [opt, status, index] = parse_arguments(std::span(argv + 1, argc - 1));
    options = std::move(opt);
  }*/

  if (options.build_files.empty())
    options.build_files.emplace_back("build.shinobi");
  if (options.config_files.empty())
    options.config_files.emplace_back(".shinobi");
  if (options.target_dir.empty())
    options.target_dir = "target/debug";

  std::cerr << "Build files:\n";
  for (const auto &build_file : options.build_files)
    std::cerr << '\t' << build_file << '\n';
  std::cerr << "Config files:\n";
  for (const auto &config_file : options.config_files)
    std::cerr << '\t' << config_file << '\n';
  std::cerr << "Target directory:\n\t" << options.target_dir << '\n';
}
