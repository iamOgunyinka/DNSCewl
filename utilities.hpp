#pragma once

#include <set>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace dns_cewl {
inline namespace utilities {
using namespace fmt::literals;

struct cli_args {
  std::string target{};
  std::string target_filename{};
  std::string range_string{};
  std::string exclusion_list_filename{};
  std::string domain_extension_filename{};
  std::string set_list_filename{};
  std::string append_list_filename{};
  std::string prepend_list_filename{};
  bool verbose_flag{false};
  bool subs_flag{false};
  bool include_flag{false};
  bool no_color{false};
  bool no_repeat_flag{false};
  int limit{};
  int level{};
};

class runtime_info {
public:
  std::vector<std::string> target_list{};
  std::vector<std::string> append_list{};
  std::vector<std::string> prepend_list{};
  std::set<std::string> exclude_list{};
  std::set<std::string> set_list{};
  std::vector<std::string> extension_list{};
  std::vector<std::string> results{};
  int range{};
  bool one_side_flag{false};
  bool subs_flag{};
  bool no_repeat_flag{};

  void print(std::string const &str);
};

void error_fatal(std::string const &);
bool is_integer(const std::string &);
std::vector<std::string> split_string(std::string const &, char);
std::vector<std::string> read_file(std::string const &);
void process_set(runtime_info &, bool);
void process_prepend(runtime_info &, int level);
void process_append(runtime_info &, int level);
void process_range(runtime_info &, cli_args const &);
bool is_number_signed(std::string const &word, runtime_info &);
void process_extension(runtime_info &);
void print_header();
} // namespace utilities
} // namespace dns_cewl
