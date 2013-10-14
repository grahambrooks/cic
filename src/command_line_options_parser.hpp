#pragma once

#include <boost/program_options.hpp>
#include "config.hpp"

namespace ci {
  class command_line_options_parser {
    constexpr static auto username_option_name = "username";
    constexpr static auto server_uri_option_name = "server";
    constexpr static auto verbose_option_name = "verbose";
    constexpr static auto version_option_name = "version";
    constexpr static auto password_option_name = "password";
    boost::program_options::options_description desc;
  public:
    command_line_options_parser() {
      desc.add_options()
          ("help,h", "Display this message of command line options and exit")
          ("version", "Display the application version and exit")
          ("verbose,v", "Include more details of builds and status. By default a count of builds in each 'state' is printed to the console. Verbose output prints the status of each build and the causes of any failures.")
          ("server,s", boost::program_options::value<std::string>(), "A fully qualified URI of the Jenkins server view e.g. http://jenkins.example.com/myview. The view path is optional and defaults to the default view. If not specified ci searches for a .ci files in the current path. If ci is run in /user/graham/projects/myproject then ci looks in the current directory, /user/graham/projects, /user/graham and /user for ci files. Configuration closest to the current directory takes precidence. Command line arguments override any configuration file settings.")
          ("username,u", boost::program_options::value<std::string>(), "Username for authentication with the CI system. CI uses libcurl to retrieve json data from the server and does basic HTTP authentication using the supplied username and password")
          ("password,p", boost::program_options::value<std::string>(), "Basic authentication password used to access CI system");
    }

    ci::config parse(int argc, const char *argv[]) {
      ci::config result;

      boost::program_options::variables_map variables;

      boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), variables);

      boost::program_options::notify(variables);

      if (variables.count(verbose_option_name) > 0) {
        result.set_verbose(true);
      }

      if (variables.count("help") > 0) {
        result.set_help_only();
      }

      if (variables.count(version_option_name) > 0) {
        result.set_version_only();
      }

      if (variables.count(server_uri_option_name) > 0) {
        result.server_url = variables[server_uri_option_name].as<std::string>();
      }

      if (variables.count(username_option_name) > 0) {
        result.username = variables[username_option_name].as<std::string>();
      }

      if (variables.count(password_option_name) > 0) {
        result.password = variables[password_option_name].as<std::string>();
      }

      return result;
    }

    void show_version(std::ostream &output) {
      std::cout << "ci v0.0.3 - continuous integration command line tool" << std::endl;
    }

    void show_help(std::ostream &output) {
      show_version(output);
      output << std::endl;
      output << "usage:" << std::endl;
      output << "   ci [options]" << std::endl;
      output << std::endl;
      output << "Options:" << std::endl;
      output << desc << std::endl;
    }
  };
}
