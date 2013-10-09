#pragma once

#include <boost/program_options.hpp>
#include "config.hpp"

namespace ci {
    class command_line_options_parser {
        constexpr static auto username_option_name = "username";
        constexpr static auto server_uri_option_name = "server";
        constexpr static auto verbose_option_name = "verbose";
        constexpr static auto password_option_name = "password";
        boost::program_options::options_description desc;
    public:
        command_line_options_parser() {
            desc.add_options()
                    ("help", "display this option")
                    ("verbose", "Verbose output")
                    ("server", boost::program_options::value<std::string>(), "The CI server's address")
                    ("username,u", boost::program_options::value<std::string>(), "Username for authentication with the CI system.")
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

        void show_help(std::ostream& output) {
            output << desc << std::endl;
        }
    };

}
