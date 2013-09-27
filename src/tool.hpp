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
            
            auto config_files = find_config(boost::filesystem::current_path());
            
            if (config_files.size() == 0) {
                std::cerr << "No configuration found in current path " << boost::filesystem::current_path() << std::endl;
            } else {
                ci::config runtime_config;
                
                
                for (auto c : config_files) {
                    runtime_config = runtime_config.merge(ci::config::load(c));
                }
                
                print_status(runtime_config);
            }
            
            return 0;
        }
        
        static void parseBuilds(char *buffer, size_t received) {
            boost::property_tree::ptree pt;
            std::stringstream ss(buffer);
            
            boost::property_tree::read_json(ss, pt);
            
            BOOST_FOREACH(boost::property_tree::ptree::value_type & v, pt.get_child("jobs"))
            {
                assert(v.first.empty());
                
                std::string color = v.second.get<std::string>("color");
                
                if (color == "blue") std::cout << "\x1b[32m"; else std::cout << "\x1b[31m";
                
                std::cout << v.second.get<std::string>("name") << ": " << v.second.get<std::string>("color") << "\x1b[0m" << std::endl;
            }
        }

        
        static size_t callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
            parseBuilds(ptr, size * nmemb);
            return size * nmemb;
        }

        
        
        void print_status(ci::config& config) {
            CURL *curl;
            CURLcode res;
            
            curl = curl_easy_init();
            if (curl) {
                std::cout << "reading " << config.server_url << std::endl;
                curl_easy_setopt(curl, CURLOPT_URL, config.server_url.c_str());
                /* example.com is redirected, so we tell libcurl to follow redirection */
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_USERNAME, config.username.c_str());
                curl_easy_setopt(curl, CURLOPT_PASSWORD, config.password.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &callback);
                //        curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
                
                /* Perform the request, res will get the return code */
                res = curl_easy_perform(curl);
                /* Check for errors */
                if (res != CURLE_OK)
                    fprintf(stderr, "curl_easy_perform() failed: %s\n",
                            curl_easy_strerror(res));
                curl_easy_cleanup(curl);
            }
        }
        
        
        std::list<boost::filesystem::path> find_config(boost::filesystem::path path) {
            std::list<boost::filesystem::path> result;
            
            if (boost::filesystem::is_regular(path)) {
                path = path.parent_path();
            }
            
            
            do {
                if (boost::filesystem::exists(path / ".ci")) {
                    result.push_front(path / ".ci");
                }
                path = path.parent_path();
            } while (path.has_parent_path());
            return result;
        }
    };
}
