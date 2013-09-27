#pragma once
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>

namespace ci {
    class config {
    private:
    public:
        std::string server_type;
        std::string server_url;
        std::string username;
        std::string password;
        
        void from_string(const std::string config_text) {
            
            boost::property_tree::ptree pt;
            std::stringstream ss(config_text);
            
            boost::property_tree::read_json(ss, pt);
            
            server_type = pt.get<std::string>("server.type");
            server_url = pt.get<std::string>("server.url");
            
            if (pt.get_optional<std::string>("server.username") != NULL) {
                username = pt.get<std::string>("server.username");
            }
            
            if (pt.get_optional<std::string>("server.password") != NULL) {
                password = pt.get<std::string>("server.password");
            }
        }
        
        bool is_valid() {
            return !server_type.empty() && !server_url.empty();
        }
        
        
        
        static config load(const boost::filesystem::path configuration_path) {
            
            boost::property_tree::ptree pt;
            std::ifstream ss(configuration_path.c_str());
            
            boost::property_tree::read_json(ss, pt);
            
            std::cout << "server " <<                 pt.get<std::string>("server.url") << std::endl;
            
            return {
                pt.get<std::string>("server.type"),
                pt.get<std::string>("server.url"),
                
                pt.get_optional<std::string>("server.username") != NULL ? pt.get<std::string>("server.username") : "",
                pt.get_optional<std::string>("server.password") != NULL ? pt.get<std::string>("server.password") : ""
            };
        }
        
        config merge(config other) {
            return {
                server_type.empty() ? other.server_type : server_type,
                server_url.empty() ? other.server_url : server_url,
                username.empty() ? other.username : username,
                password.empty() ? other.password : password
            };
        }

        bool requires_authentication() {
            return !username.empty() || !password.empty();
        }
    };
}
