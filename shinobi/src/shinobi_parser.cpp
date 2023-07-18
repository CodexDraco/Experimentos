#include <fstream>
#include <string_view>

#include <toml++/toml.h>

void parse_shinobi_file(const std::string_view &build_file) {
  toml::table tbl;
  try {
    tbl = toml::parse_file(build_file);
  } catch (const toml::parse_error &err) {
    // do nutin
  }
}
