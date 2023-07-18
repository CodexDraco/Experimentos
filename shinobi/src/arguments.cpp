module;

#include <array>
#include <string_view>
#include <vector>

export module arguments;

export struct Options {
  std::vector<std::string_view> config_files;
  std::vector<std::string_view> build_files;
  std::string_view target_dir;
  std::vector<std::string_view> defines;
};

export struct ParserResult {
  enum struct Status {
    Ok,
    ParseError,
  };

  Options options;
  Status status;
  int index;
};

enum class ArgToken {
  TokenConfig,
  TokenBuildfile,
  TokentTargetDir,
  TokenDefine,
  TokenString,
};

enum class TokenizerState {
  Start,
  Dash,
  Option,
  String,
};

constexpr ArgToken arg_tokenize(const char *arg) {
  TokenizerState state = TokenizerState::Start;
  int index = 0;
  ArgToken result = ArgToken::TokenString;

  while (arg[index] != '\0') {
    if (state == TokenizerState::Start && arg[index] == '-') {
      state = TokenizerState::Dash;
      index++;
      continue;
    }

    if (state == TokenizerState::Dash) {
      state = TokenizerState::Option;
      switch (arg[index]) {
      case 'c':
        result = ArgToken::TokenConfig;
        break;
      case 'b':
        result = ArgToken::TokenBuildfile;
        break;
      case 't':
        result = ArgToken::TokentTargetDir;
        break;
      case 'D':
        result = ArgToken::TokenDefine;
        break;
      default:
        result = ArgToken::TokenString;
        state = TokenizerState::String;
        break;
      }
      index++;
      continue;
    }

    result = ArgToken::TokenString;
    break;
  }
  return result;
}

enum class ParserState {
  Start,
  ExpectConfig,
  ExpectBuildfile,
  ExpectTargetdir,
  ExpectDefine,
};

export constexpr auto parse_arguments(const auto &args) -> ParserResult {
  auto state = ParserState::Start;
  auto options = Options{};
  int index = 0;

  for (auto arg : args) {
    if (state == ParserState::Start) {
      switch (arg_tokenize(arg)) {
      case ArgToken::TokenConfig:
        state = ParserState::ExpectConfig;
        break;
      case ArgToken::TokenBuildfile:
        state = ParserState::ExpectBuildfile;
        break;
      case ArgToken::TokentTargetDir:
        state = ParserState::ExpectTargetdir;
        break;
      case ArgToken::TokenDefine:
        state = ParserState::ExpectDefine;
        break;
      case ArgToken::TokenString:
        return ParserResult{options, ParserResult::Status::ParseError, index};
        break;
      default:
        break;
      }
    } else {
      switch (state) {
      case ParserState::ExpectConfig:
        options.config_files.push_back(arg);
        break;
      case ParserState::ExpectBuildfile:
        options.build_files.push_back(arg);
        break;
      case ParserState::ExpectTargetdir:
        /*if(!options.target_dir.empty())
          result.warnings.push_back(ShinWarn::OVERWRITE_TARGET_DIR);*/
        options.target_dir = arg;
        break;
      case ParserState::ExpectDefine:
        options.defines.push_back(arg);
        break;
      default:
        break;
      }
      state = ParserState::Start;
    }
    index++;
  }
  if (state != ParserState::Start) {
    return ParserResult{options, ParserResult::Status::ParseError, index};
  }
  return ParserResult{.options = options, .index = index};
}

//========== Test cases ==========//

constexpr void test_arg_tokenize() {
  static_assert(arg_tokenize("") == ArgToken::TokenString);
  static_assert(arg_tokenize("-c") == ArgToken::TokenConfig);
  static_assert(arg_tokenize("foo") == ArgToken::TokenString);
  static_assert(arg_tokenize("-") == ArgToken::TokenString);
  static_assert(arg_tokenize("--") == ArgToken::TokenString);
  static_assert(arg_tokenize("-k") == ArgToken::TokenString);
  static_assert(arg_tokenize("-D") == ArgToken::TokenDefine);
  static_assert(arg_tokenize("-Dfoo=bar") == ArgToken::TokenString,
                "should not support -Dfoo style");
}

constexpr bool check_arg_parser_result(const ParserResult &result,
                                       const ParserResult::Status status,
                                       const auto &build_files,
                                       const auto &config_files,
                                       const auto &target_dir,
                                       const auto &defines) {
  return (result.status == status &&
          result.options.build_files == build_files &&
          result.options.config_files == config_files &&
          result.options.target_dir == target_dir &&
          result.options.defines == defines);
};

constexpr void test_args_parser() {
  using vec_t = std::vector<std::string_view>;
  static_assert(
      check_arg_parser_result(parse_arguments(std::array{"-b", "b.txt"}),
                              ParserResult::Status::Ok, vec_t{"b.txt"}, vec_t{},
                              "", vec_t{}),
      "-b should add build files");

  static_assert(check_arg_parser_result(
                    parse_arguments(std::array{"-c", "conf", "-t", "build"}),
                    ParserResult::Status::Ok, vec_t{}, vec_t{"conf"}, "build",
                    vec_t{}),
                "-c should add config files and -t set the output directory");

  static_assert(check_arg_parser_result(
                    parse_arguments(std::array{"-t", "a", "-t", "b"}),
                    ParserResult::Status::Ok, vec_t{}, vec_t{}, "b", vec_t{}),
                "multiple -t should keep the last path");

  static_assert(parse_arguments(std::array{"foo"}).status ==
                    ParserResult::Status::ParseError,
                "starting non option argument should be a ParseError");

  auto f = [](const ParserResult &result) {
    return (result.status == ParserResult::Status::ParseError &&
            result.index == 2);
  };
  static_assert(f(parse_arguments(std::array{"-t", "foo", "bar"})),
                "orphan non option argument should cause a ParseError");

  static_assert(check_arg_parser_result(
                    parse_arguments(std::array{"-D", "foo", "-D", "bar=baz"}),
                    ParserResult::Status::Ok, vec_t{}, vec_t{}, "",
                    vec_t{"foo", "bar=baz"}),
                "should accept multiple defines");
}
