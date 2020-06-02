#include "utilities.hpp"
#include <CLI11/CLI11.hpp>

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

namespace dns_cewl {
using namespace fmt::v6::literals;

void start_generating(runtime_info &rt_info, cli_args const &args) {

  if (!rt_info.append_list.empty()) {
    process_append(rt_info, args.level);
  } else {
    process_prepend(rt_info, args.level);
  }
  if (!rt_info.set_list.empty()) {
    process_set(rt_info, args.include_flag);
  }
  if (!rt_info.extension_list.empty()) {
    process_extension(rt_info);
  }
  spdlog::debug(args.level);
  if (args.level == 2 || !args.range_string.empty()) {
    process_range(rt_info, args);
  }
}

runtime_info preprocess_arguments(cli_args const &arg) {
  runtime_info rt_info{};
  rt_info.subs_flag = arg.subs_flag;
  rt_info.no_repeat_flag = arg.no_repeat_flag;

  if (arg.verbose_flag) {
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
  }
  if (!arg.range_string.empty()) {
    if (is_number_signed(arg.range_string, rt_info)) {
      rt_info.range = std::stoi(arg.range_string);
    } else {
      error_fatal("{} is not a valid range."_format(arg.range_string));
    }
  }

  if (arg.target.empty() && arg.target_filename.empty()) // use stdin
  {
    std::string target{};
    while (std::getline(std::cin, target))
      rt_info.target_list.emplace_back(target);
  }
  if (arg.append_list_filename.empty() && arg.prepend_list_filename.empty() &&
      arg.set_list_filename.empty()) {
    error_fatal("File name of words to process a set list, append or prepend "
                "to targets is needed");
  } else if (!arg.append_list_filename.empty()) {
    spdlog::debug("Reading append list.");
    rt_info.append_list = read_file(arg.append_list_filename);
    if (rt_info.append_list.empty()) {
      error_fatal(
          "Please check {}, no words read"_format(arg.append_list_filename));
    }
  } else if (!arg.prepend_list_filename.empty()) {
    spdlog::debug("Reading prepend list.");
    rt_info.prepend_list = read_file(arg.prepend_list_filename);
    if (rt_info.prepend_list.empty()) {
      error_fatal(
          "Please check {}, no words read"_format(arg.prepend_list_filename));
    }
  }
  if (!arg.target.empty()) {
    spdlog::debug("Adding target [{}] to list.", arg.target);
    rt_info.target_list.push_back(arg.target);
  }
  if (!arg.target_filename.empty()) {
    rt_info.target_list = read_file(arg.target_filename);
    if (rt_info.target_list.empty()) {
      error_fatal(
          "Please check {}, no targets read"_format(arg.target_filename));
    }
  }

  if (!arg.exclusion_list_filename.empty()) {
    auto exclusion_list = read_file(arg.exclusion_list_filename);
    using iter = decltype(exclusion_list)::iterator;
    rt_info.exclude_list.insert(
        std::move_iterator<iter>(exclusion_list.begin()),
        std::move_iterator<iter>(exclusion_list.end()));
  }

  if (!arg.set_list_filename.empty()) {
    auto set_list = read_file(arg.set_list_filename);
    using iter = decltype(set_list)::iterator;
    rt_info.set_list.insert(std::move_iterator<iter>(set_list.begin()),
                            std::move_iterator<iter>(set_list.end()));
  }
  if (!arg.domain_extension_filename.empty()) {
    rt_info.extension_list = read_file(arg.domain_extension_filename);
  }
  return rt_info;
}
} // namespace dns_cewl

int main(int argc, char **argv) {
  dns_cewl::print_header();
  dns_cewl::cli_args argument{};
  CLI::App app{"When provided with a list of domain names, generate a wordlist "
               "of potential subdomains to be tested for."};
  app.add_option("-t,--target", argument.target, "Specify a single target.");
  app.add_option("-l,--tL,--target-list", argument.target_filename,
                 "Specify a list of targets.");
  app.add_option("--sL,--set-list", argument.set_list_filename,
                 "Specify a list of targets.");
  app.add_option("-e,--eL,--exclude-list", argument.exclusion_list_filename,
                 "Specify a list of targets to exclude.");
  app.add_option("--eX,--domain-extension", argument.domain_extension_filename,
                 "Specify a list of domain extensions to substitute with.");
  app.add_option("-a,--append-list", argument.append_list_filename,
                 "Specify a file of words to append to a host.");
  app.add_option("-p,--prepend-list", argument.prepend_list_filename,
                 "Specify a file of words to prepend  to a host.");
  app.add_flag(
      "-v,--verbose_flag", argument.verbose_flag,
      "If set then verbose_flag output will be displayed in the terminal.");
  app.add_flag("-i,--include-original", argument.include_flag,
               "If set, original domains (from source files) are included in "
               "the output.");
  app.add_option("--range", argument.range_string,
                 "Set a higher range for integer permutations.");
  app.add_flag("-s,--subs", argument.subs_flag,
               "If set then only subdomains will be generated.");
  app.add_flag(
      "--no-color", argument.no_color,
      "If set then any foreground or background colours will be stripped out.");
  app.add_option("--limit", argument.limit,
                 "Specify a fixed word limit to output.");
  app.add_option("--level", argument.level,
                 "Specify the level of results to output.");
  app.add_flag("--no-repeats", argument.no_repeat_flag,
               "Prevent repeated structures such as one.one.com");

  CLI11_PARSE(app, argc, argv);
  auto rt_info{dns_cewl::preprocess_arguments(argument)};
  dns_cewl::start_generating(rt_info, argument);
}
