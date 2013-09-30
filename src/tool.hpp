#pragma once
#include <list>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <curl/curl.h>

#include "config.hpp"

namespace ci {
    class tool {
    public:
        int run() {
            std::cout << "ci v0.0.1 - continuous integration command line tool" << std::endl;

            boost::filesystem::path current_path = boost::filesystem::current_path();
            std::list<boost::filesystem::path>  config_files;

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

        static void parse_builds(char *buffer, size_t received) {
            boost::property_tree::ptree pt;
            std::stringstream ss(buffer);

            boost::property_tree::read_json(ss, pt);

            BOOST_FOREACH(boost::property_tree::ptree::value_type & v, pt.get_child("jobs")) {
                        assert(v.first.empty());

                        print_build(v);

                    }
        }

        static void print_build(boost::property_tree::ptree::value_type &v) {

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

        static size_t callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
            parse_builds(ptr, size * nmemb);
            return size * nmemb;
        }

        void print_status(ci::config &config) {
            CURL *curl;
            CURLcode res;

            curl = curl_easy_init();
            if (curl) {
                curl_easy_setopt(curl, CURLOPT_URL, (config.server_url + "/api/json").c_str());

                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

                if (config.requires_authentication()) {
                    curl_easy_setopt(curl, CURLOPT_USERNAME, config.username.c_str());
                    curl_easy_setopt(curl, CURLOPT_PASSWORD, config.password.c_str());
                }
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &callback);

                res = curl_easy_perform(curl);

                if (res != CURLE_OK) {
                    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                }

                curl_easy_cleanup(curl);
            }
        }

        void find_config_files_on_path(boost::filesystem::path path, std::list<boost::filesystem::path>& result) {
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
