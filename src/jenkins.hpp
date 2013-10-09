
#pragma once
#include "config.hpp"
#include "http.hpp"


namespace ci {
    namespace servers {
        class jenkins {
            HTTP::client& _client;
            config& _config;
        public:
            jenkins(HTTP::client& client, config& config) : _client(client), _config(config) {}
            
            void summary() {
                auto response = _client.get(_config.server_url + "/api/json", _config.username, _config.password);
                
                parse_builds(response, _config);
                
                for_each_build(response, [&] (boost::property_tree::ptree::value_type& v){
                    assert(v.first.empty());
                    
                    print_build(v, _config);
                });

            }
            
            void for_each_build(std::string buffer, std::function<void (boost::property_tree::ptree::value_type &)> f) {
                boost::property_tree::ptree pt;
                std::stringstream ss(buffer);
                
                boost::property_tree::read_json(ss, pt);
                for (auto v : pt.get_child("jobs")) {
                    f(v);
                }
                
            }
            
            void parse_builds(std::string buffer, config &c) {
                boost::property_tree::ptree pt;
                std::stringstream ss(buffer);
                
                boost::property_tree::read_json(ss, pt);
                for (auto v : pt.get_child("jobs")) {
                    assert(v.first.empty());
                    
                    print_build(v, c);
                }
            }
            
            void print_build(boost::property_tree::ptree::value_type &v, config &c) {
                
                auto print_build_summary = false;
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
                    
                    print_build_summary = true;
                }
                
                std::cout << "\x1b[0m" << std::endl;
                
                if (print_build_summary) {
                    
                    HTTP::curl_client client;
                    
                    auto response = client.get(v.second.get<std::string>("url") + "api/json?pretty=true", c.username, c.password);
                    
                    boost::property_tree::ptree job_tree;
                    std::stringstream ss(response);
                    
                    boost::property_tree::read_json(ss, job_tree);
                    
                    auto last_build_url = job_tree.get<std::string>("lastBuild.url");
                    
                    boost::property_tree::ptree last_build;
                    auto last_build_data = client.get(last_build_url + "api/json?pretty=true", c.username, c.password);
                    std::stringstream lbss(last_build_data);
                    
                    boost::property_tree::read_json(lbss, last_build);
                    
                    std::cout << std::endl;
                    
                    auto actions = last_build.get_child("actions");
                    
                    std::map<std::string, std::function<void (const boost::property_tree::ptree::value_type &)> > x = {
                        {
                            "shortDescription",
                            [](const boost::property_tree::ptree::value_type &vt) -> void {
                                std::cout << vt.second.data() << std::endl;
                            }
                        },
                        {
                            "userId",
                            [](const boost::property_tree::ptree::value_type &vt) -> void {
                                std::cout << "User Id: " << vt.second.data() << std::endl;
                            }
                        },
                        {
                            "userName",
                            [](const boost::property_tree::ptree::value_type &vt) -> void {
                                std::cout << "User : " << vt.second.data() << std::endl;
                            }
                        },
                        {
                            "claimedBy",
                            [](const boost::property_tree::ptree::value_type &vt) -> void {
                                std::cout << "Claimed by: " << vt.second.data() << std::endl;
                            }
                        }
                        
                    };
                    
                    print_tree(actions, x, 1);
                }
                
            }
            
            void print_tree(boost::property_tree::ptree &pt, std::map<std::string, std::function<void (const boost::property_tree::ptree::value_type &)> > &x, int indent) {
                
                for (auto v : pt) {
                    
                    if (x.find(v.first) != x.end()) {
                        for (auto c = 0; c < indent; c++) {
                            std::cout << "   ";
                        }
                        
                        x[v.first](v);
                    }
                    
                    print_tree(v.second, x, 0);
                }
                
            }

            
        };
    }
}