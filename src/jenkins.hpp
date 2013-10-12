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
            typedef boost::property_tree::ptree ptree;
            typedef std::function<void (ptree::value_type &)> tree_visitor_type;

            jenkins(HTTP::client &client, ci::config &config) : client(client), config(config) {
            }

            void summary() {
                auto response = client.get(config.server_url + "/api/json", config.username, config.password);

                build_count build_count;


                std::ostream &out = output_stream();

                for_each_build(response, [&] (ptree::value_type &v) {
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
                return config.verbose_output() ? std::cout : dummy;
            }


            void for_each_build(std::string buffer, tree_visitor_type f) {

                auto pt = json_tree_from_string(buffer);

                for (auto v : pt.get_child("jobs")) {
                    f(v);
                }

            }

            ptree json_tree_from_string(std::string s) {
                ptree pt;
                std::stringstream ss(s);

                boost::property_tree::read_json(ss, pt);
                return pt;
            }

            void print_build(ptree::value_type &v, ci::config &c, build_count &build_count, std::ostream &out) {

                auto print_build_summary = false;
                out << v.second.get<std::string>("name") << ": ";

                std::string build_colour = v.second.get<std::string>("color");

                if (is_successful_build(build_colour)) {
                    print_successful_build(build_colour, build_count, out);

                } else {
                    print_build_summary = print_failed_build(build_colour, build_count, out);

                }

                out << ANSI_ESCAPE::RESEET << std::endl;

                if (print_build_summary && c.verbose_output()) {

                    std::string failed_build_url = v.second.get<std::string>("url");

                    auto actions = get_failed_build_actions(c, failed_build_url);

                    print_actions(out, actions);

                }
            }

            bool print_failed_build(std::string build_colour, build_count &build_count, std::ostream &out) {
                bool print_build_summary;
                out << ANSI_ESCAPE::RED;

                if (is_building(build_colour)) {
                    out << "fixing";
                    build_count.fixing += 1;
                } else {
                    out << "broken";
                    build_count.failed += 1;
                }

                print_build_summary = true;
                return print_build_summary;
            }

            void print_successful_build(std::string build_colour, build_count &build_count, std::ostream &out) {
                out << ANSI_ESCAPE::GREEN;

                if (is_building(build_colour)) {
                    out << "building";
                    build_count.building += 1;
                } else {
                    out << "waiting";
                    build_count.successful += 1;
                }
            }

            bool is_successful_build(std::string build_colour) {
                return build_colour.find("blue") != std::string::npos;
            }

            bool is_building(std::string build_colour) {
                return build_colour.find("anime") != std::string::npos;
            }

            ptree get_json(std::string uri, ci::config config) {

                auto response = client.get(uri, config.username, config.password);

                std::stringstream stream(response);

                ptree tree;
                boost::property_tree::json_parser::read_json(stream, tree);
                return tree;
            }

            ptree get_failed_build_actions(ci::config &config, std::string failed_build_url) {

                auto job_tree = get_json(failed_build_url + "api/json?pretty=true", config);

                auto last_build_url = job_tree.get<std::string>("lastBuild.url");

                auto last_build = get_json(last_build_url + "api/json", config);

                return last_build.get_child("actions");
            }

            void print_actions(std::ostream &out, ptree &actions) {
                char const *indent = "   ";
                auto short_description = [&](const boost::property_tree::ptree::value_type &vt) -> void {
                    if (vt.second.data().length() > 0) {
                        out << indent << vt.second.data() << std::endl;
                    }
                };
                auto user_id = [&](const boost::property_tree::ptree::value_type &vt) -> void {
                    out << indent << "User Id: " << vt.second.data() << std::endl;
                };

                auto user_name = [&](const boost::property_tree::ptree::value_type &vt) -> void {
                    out << indent << "User : " << vt.second.data() << std::endl;
                };

                auto claimed_by = [&](const boost::property_tree::ptree::value_type &vt) -> void {
                    out << indent << "Claimed by: " << vt.second.data() << std::endl;
                };

                std::map<std::string, tree_visitor_type > x = {
                        {"shortDescription", short_description},
                        {"userId", user_id},
                        {"userName", user_name},
                        {"claimedBy", claimed_by}
                };

                out << std::endl;
                print_tree(actions, out, x);
                out << std::endl;
            }

            void print_tree(boost::property_tree::ptree &pt, std::ostream &out, std::map<std::string, tree_visitor_type> &x) {

                for (auto v : pt) {

                    if (x.find(v.first) != x.end()) {
                        x[v.first](v);
                    }

                    print_tree(v.second, out, x);
                }
            }
        };
    }
}