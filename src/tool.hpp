#pragma once
#include <list>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

namespace ci {
    class tool {
    public:
        int run() {
            std::cout << "ci v0.0.1 - continuous integration command line tool" << std::endl;
            
            auto config_files = find_config(boost::filesystem::current_path());
            
            if (config_files.size() == 0) {
                std::cerr << "No configuration found in current path " << boost::filesystem::current_path() << std::endl;
            } else {
                std::cout << "lets go " << std::endl;
            }
            
            return 0;
        }
        
        std::list<boost::filesystem::path> find_config(boost::filesystem::path path){
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