#pragma once
#pragma once
#include <list>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <curl/curl.h>

#include "config.hpp"
#include "http.hpp"

namespace ci {
    class tool {
    public:
        int run() {
            std::cout << "ci v0.0.1 - continuous integration command line tool" << std::endl;

            boost::filesystem::path current_path = boost::filesystem::current_path();
            std::list<boost::filesystem::path> config_files;

            find_config_files_on_path(current_path, config_files);

            if (config_files.size() == 0) {
                std::cerr << "No configuration found in current path " << current_path << std::endl;
                return 1;
            } else {
                config runtime_config = read_configuration(config_files);

                if (!runtime_config.is_valid()) {
                    std::cerr << "Configuration not complete please check your .ci files" << std::endl;
                    return 1;
                } else {
                    print_status(runtime_config);
                }
            }
            return 0;
        }

        config read_configuration(std::list<boost::filesystem::path> config_files) {
            config runtime_config;

            for (auto c : config_files) {
                runtime_config = runtime_config.merge(config::load(c));
            }
            return runtime_config;
        }

        void parse_builds(std::string buffer) {
            boost::property_tree::ptree pt;
            std::stringstream ss(buffer);

            boost::property_tree::read_json(ss, pt);

            BOOST_FOREACH(boost::property_tree::ptree::value_type & v, pt.get_child("jobs")) {
                        assert(v.first.empty());

                        print_build(v);

                    }
        }

        void print_build(boost::property_tree::ptree::value_type &v) {

            std::cout << v.second.get<std::string>("name") << ": ";

            std::string color = v.second.get<std::string>("color");
            if (color.find("blue") != std::string::npos) {
                std::cout << "\x1b[32m";

                if (color.find("anime") != std::string::npos) {
                    std::cout << "building";
                } else {
                    std::cout << "waiting";
                }

            } else {
                std::cout << "\x1b[31m";

                if (color.find("anime") != std::string::npos) {
                    std::cout << "fixing";
                } else {
                    std::cout << "broken";
                }
            }

            std::cout << "\x1b[0m" << std::endl;
        }


        void print_status(ci::config &config) {
            HTTP client;

            auto response = client.get(config.server_url + "/api/json", config.username, config.password);

            parse_builds(response);
        }

        void find_config_files_on_path(boost::filesystem::path path, std::list<boost::filesystem::path> &result) {
            if (boost::filesystem::is_regular(path)) {
                path = path.parent_path();
            }

            do {
                if (boost::filesystem::exists(path / ".ci")) {
                    result.push_front(path / ".ci");
                }
                path = path.parent_path();
            } while (path.has_parent_path());
        }
    };
}
