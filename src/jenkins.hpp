#pragma once
#include "config.hpp"
#include "http.hpp"
#include <sstream>
#include "ansii_escape.hpp"

namespace ci {
    namespace servers {
        class jenkins {
            HTTP::client &client;
            ci::config &config;
            std::stringstream dummy;

            struct build_count {
                build_count() : successful(0), building(0), failed(0), fixing(0) {
                }

                int successful;
                int building;
                int failed;
                int fixing;
            };

        public:
            jenkins(HTTP::client &client, ci::config &config) : client(client), config(config) {
            }

            void summary() {
                auto response = client.get(config.server_url + "/api/json", config.username, config.password);

                build_count build_count;


                std::ostream &out = output_stream();

                for_each_build(response, [&] (boost::property_tree::ptree::value_type &v){
                    assert(v.first.empty());

                    print_build(v, config, build_count, out);
                });

                std::cout
                        << std::endl
                        << "Successful (" << build_count.successful
                        << ") building (" << build_count.building
                        << ") failed (" << build_count.failed
                        << ") fixing (" << build_count.fixing
                        << ")" << std::endl;
            }

            std::ostream &output_stream() {
                return config.verbose_output() ? std::__1::cout : dummy;
            }

            void for_each_build(std::string buffer, std::function<void (boost::property_tree::ptree::value_type &)> f) {
                boost::property_tree::ptree pt;
                std::stringstream ss(buffer);

                boost::property_tree::read_json(ss, pt);
                for (auto v : pt.get_child("jobs")) {
                    f(v);
                }

            }


            void print_build(boost::property_tree::ptree::value_type &v, ci::config &c, build_count &build_count, std::ostream &out) {

                auto print_build_summary = false;
                out << v.second.get<std::string>("name") << ": ";

                std::string build_colour = v.second.get<std::string>("color");
                if (build_colour.find("blue") != std::string::npos) {
                    out << ANSI_ESCAPE::GREEN << "\x1b[32m";

                    if (build_colour.find("anime") != std::string::npos) {
                        out << "building";
                        build_count.building += 1;
                    } else {
                        out << "waiting";
                        build_count.successful += 1;
                    }
                } else {
                    out << ANSI_ESCAPE::RED;

                    if (build_colour.find("anime") != std::string::npos) {
                        out << "fixing";
                        build_count.fixing += 1;
                    } else {
                        out << "broken";
                        build_count.failed += 1;
                    }

                    print_build_summary = true;
                }

                out << ANSI_ESCAPE::RESEET << std::endl;

                if (print_build_summary && c.verbose_output()) {

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


                    auto actions = last_build.get_child("actions");

                    std::map<std::string, std::function<void (const boost::property_tree::ptree::value_type &)> > x = {
                            {
                                    "shortDescription",
                    [&](const boost::property_tree::ptree::value_type &vt) -> void {
                        if (vt.second.data().length() > 0)
                            out << "   " << vt.second.data() << std::endl;
                    }
                },
                {
                    "userId",
                            [&](const boost::property_tree::ptree::value_type &vt) -> void {
                                out << "   " << "User Id: " << vt.second.data() << std::endl;
                            }
                },
                {
                    "userName",
                            [&](const boost::property_tree::ptree::value_type &vt) -> void {
                                out << "   " << "User : " << vt.second.data() << std::endl;
                            }
                },
                {
                    "claimedBy",
                            [&](const boost::property_tree::ptree::value_type &vt) -> void {
                                out << "   " << "Claimed by: " << vt.second.data() << std::endl;
                            }
                }
            };

            out <<
            std::endl;
            print_tree(actions, out, x,
            1);
            out <<
            std::endl;
        }


    }

    void print_tree(boost::property_tree::ptree &pt, std::ostream &out, std::map<std::string, std::function<void (const boost::property_tree::ptree::value_type &)> > &x, int indent) {

        for (auto v : pt) {

            if (x.find(v.first) != x.end()) {
                x[v.first](v);
            }

            print_tree(v.second, out, x, 0);
        }

    }


};
}
}