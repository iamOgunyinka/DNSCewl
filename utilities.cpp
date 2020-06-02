#include "utilities.hpp"
#include <fstream>
#include <iostream>

namespace dns_cewl {
namespace utilities {
std::size_t count_char(std::string const &domain, char character) {
  return std::count(domain.cbegin(), domain.cend(), character);
}

bool have_repeats(std::string const &domain) {
  std::vector<std::string> words = split_string(domain, '.');
  for (std::size_t i = 0; i < words.size(); ++i) {
    for (std::size_t j = i + 1; j < words.size(); ++j) {
      if (words[i] == words[j]) {
        spdlog::debug("Repeats found in {}.", domain);
        return true;
      }
    }
  }
  return false;
}

std::pair<int, int> get_range(int const n, int const range,
                              bool const one_side_flag) {
  int lower = 0;
  int upper = 0;
  if (range < 0) {
    if (!one_side_flag) {
      lower = n + range;
      upper = n - range;
    } else {
      lower = n + range;
      upper = n;
    }
  } else if (range == 0) {
    lower = n - 100;
    upper = n + 100;
  } else if (range > 0) {
    if (!one_side_flag) {
      lower = n - range;
      upper = n + range;
    } else {
      lower = n;
      upper = n + range;
    }
  }
  return {lower, upper};
}

void process_range(runtime_info &rt_info, cli_args const &args) {
  spdlog::debug("Processing range.");
  auto &range = rt_info.range;
  for (auto const &target : rt_info.target_list) {
    std::vector<std::string> splits = split_string(target, '.');
    for (auto const &split : splits) {
      if (is_integer(split)) {
        int n = std::stoi(split);
        auto lower_upper = get_range(n, range, rt_info.one_side_flag);
        for (int i = lower_upper.first; i < lower_upper.second; ++i) {
          std::string domain = target;
          domain.replace(domain.find(split), split.length(), std::to_string(i));
          rt_info.print(domain);
        }
      }
    }
  }
  std::cout << "Range string: " << args.range_string << std::endl;
  std::cout << "Range : " << range << std::endl;
  std::cout << "Both side flag: " << std::boolalpha << rt_info.one_side_flag
            << std::noboolalpha << std::endl;
}

bool is_integer(std::string const &word) {
  return !word.empty() &&
         std::find_if(word.cbegin(), word.cend(), [](char const c) {
           return !std::isdigit(c);
         }) == word.cend();
}

bool is_number_signed(std::string const &word, runtime_info &rt_info) {
  if (word.empty())
    return false;
  if (word[0] == '+' || word[0] == '-') {
    rt_info.one_side_flag = is_integer(word.c_str() + 1);
    return rt_info.one_side_flag;
  }
  return is_integer(word);
}

void process_append(runtime_info &rt_info, int const level) {
  spdlog::debug("Processing append list.");
  for (auto const &target : rt_info.target_list) {
    if (rt_info.exclude_list.find(target) != rt_info.exclude_list.cend())
      continue;
    std::string::size_type location{};
    if (count_char(target, '.') >= 2) {
      location = target.find_first_of('.');
    } else {
      location = target.find_last_of(".");
    }
    if (location == std::string::npos)
      continue;
    std::string const left_target(target.cbegin(), target.cbegin() + location);
    std::string const right_target(target.cbegin() + location, target.cend());
    for (auto const &append : rt_info.append_list) {
      rt_info.print("{}-{}{}"_format(left_target, append, right_target));
      // Second level
      if (level != 1) {
        rt_info.print("{}.{}{}"_format(left_target, append, right_target));
      }
      // Third level
      rt_info.print("{}{}{}"_format(left_target, append, right_target));
    }
  }
}

void process_extension(runtime_info &rt_info) {
  spdlog::debug("Processing extension list.");
  for (auto const &target : rt_info.target_list) {
    for (auto const &extension : rt_info.extension_list) {
      std::string domain{target};
      domain.replace(domain.find_last_of('.'), domain.length(), extension);
      rt_info.print(target);
    }
  }
}

void process_prepend(runtime_info &rt_info, int const level) {
  spdlog::debug("Processing prepend list.");
  for (auto const &target : rt_info.target_list) {
    for (auto const &prepend : rt_info.prepend_list) {
      if (rt_info.exclude_list.find(target) == rt_info.exclude_list.cend()) {
        // First one
        rt_info.print(prepend + target);
        // Second one
        if (level != 1) {
          rt_info.print("{}-{}"_format(prepend, target));
        }
        // Third one
        rt_info.print("{}.{}"_format(prepend, target));
      }
    }
  }
}

void process_set(runtime_info &rt_info, bool const include_flag) {
  spdlog::debug("Processing set list.");
  for (auto const &target : rt_info.target_list) {
    std::vector<std::string> splits = split_string(target, '.');
    for (auto const &sub_str : splits) {
      if (rt_info.set_list.find(sub_str) != rt_info.set_list.cend()) {
        for (auto const &set_word : rt_info.set_list) {
          if (set_word != sub_str || include_flag) {
            std::string subset = target;
            subset.replace(subset.find(sub_str), sub_str.length(), set_word);
            rt_info.print(subset);
          }
        }
      }
    }
  }
}

std::vector<std::string> split_string(const std::string &domain,
                                      char delimeter) {
  std::string::size_type from_pos{};
  std::string::size_type index{domain.find(delimeter, from_pos)};
  if (index == std::string::npos)
    return {};
  std::vector<std::string> result{};
  while (index != std::string::npos) {
    result.push_back(domain.substr(from_pos, index - from_pos));
    from_pos = index + 1;
    index = domain.find(delimeter, from_pos);
  }
  if (from_pos < domain.length())
    result.push_back(domain.substr(from_pos));
  return result;
}
void error_fatal(std::string const &message) {
  spdlog::error("ERROR: {}. Exiting", message);
  exit(1);
}

std::vector<std::string> read_file(std::string const &filename) {
  spdlog::debug("Reading words from {} ", filename);

  std::vector<std::string> list{};
  std::ifstream file{filename};
  if (!file) {
    error_fatal("Can not read " + filename);
  }
  std::string line{};
  while (std::getline(file, line)) {
    list.push_back(line);
  }
  spdlog::debug("Sucessfuly read {} words from {}", list.size(), filename);
  return list;
}

void print_header() {
  // add the cool header back
}

void runtime_info::print(std::string const &str) {
  if (subs_flag) {
    if (count_char(str, '.') >= 2) {
      if (no_repeat_flag) {
        if (!(have_repeats(str)))
          std::cout << str << std::endl;
      } else {
        std::cout << str << std::endl;
      }
    }
  } else {
    if (no_repeat_flag) {
      if (!(have_repeats(str)))
        std::cout << str << std::endl;
    } else {
      std::cout << str << std::endl;
    }
  }
  results.push_back(str);
}
} // namespace utilities
} // namespace dns_cewl
