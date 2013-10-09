#pragma once
#pragma once
#include <list>
#include <map>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <curl/curl.h>

#include "config.hpp"
#include "http.hpp"
#include "jenkins.hpp"

namespace ci {
    class tool {
    public:
        int run(int argc, char const *argv []) {
            std::cout << "ci v0.0.2 - continuous integration command line tool" << std::endl;
            
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
                    ci::HTTP::curl_client c;
                    ci::servers::jenkins server(c, runtime_config);
                    
                    server.summary();
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
