#pragma once

#include "config.hpp"
#include "http.hpp"
#include <sstream>
#include "ansii_escape.hpp"

using namespace std;

namespace cic {
  namespace servers {

    class jenkins {
      HTTP::client &http_client;
      cic::config &config;
      stringstream dummy;

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
      typedef function<void (ptree::value_type &)> tree_visitor_type;

      jenkins(HTTP::client &http_client, cic::config &config) : http_client(http_client), config(config) {
      }

      int summary() {
        auto response = http_client.get(config.server_url + "/api/json", config.username, config.password);

        build_count build_count;


        ostream &out = output_stream();

        for_each_build(response, [&](ptree::value_type &v) {
            assert(v.first.empty());

            print_build(v, config, build_count, out);
        });

        cout << endl
            << "Successful (" << build_count.successful
            << ") building (" << build_count.building
            << ") failed (" << build_count.failed
            << ") fixing (" << build_count.fixing
            << ")" << endl;

        return build_count.failed + build_count.fixing;
      }

      ostream &output_stream() {
        return config.verbose_output() ? cout : dummy;
      }


      void for_each_build(string buffer, tree_visitor_type f) {

        auto pt = json_tree_from_string(buffer);

        for (auto v : pt.get_child("jobs")) {
          f(v);
        }

      }

      ptree json_tree_from_string(string s) {
        ptree pt;
        stringstream ss(s);

        boost::property_tree::read_json(ss, pt);
        return pt;
      }

      void print_build(ptree::value_type &v, cic::config &c, build_count &build_count, ostream &out) {

        out << v.second.get<string>("name") << ": ";

        string build_colour = v.second.get<string>("color");

        if (is_successful_build(build_colour)) {
          print_successful_build(build_colour, build_count, out);
        } else {
            print_failed_build(build_colour, build_count, out);
        }

        out << ANSI_ESCAPE::RESET << endl;

        if (c.verbose_output()) {
          if (has_been_built(build_colour)) {
            print_failed_build_details(v, c, out);
          }
        }
      }

      void print_failed_build_details(ptree::value_type &failed_build_tree, cic::config &c, ostream &out) {
        string failed_build_url = failed_build_tree.second.get<string>("url");

        auto actions = get_failed_build_actions(c, failed_build_url);

        print_actions(out, actions);
      }

      void print_failed_build(string build_colour, build_count &build_count, ostream &out) {
        out << ANSI_ESCAPE::RED;

        if (is_building(build_colour)) {
          out << "fixing";
          build_count.fixing += 1;
        } else {
          if (has_been_built(build_colour)) {
            out << "broken";
            build_count.failed += 1;
          } else {
            out << "not built";
          }
        }
      }

      void print_successful_build(string build_colour, build_count &build_count, ostream &out) {
        out << ANSI_ESCAPE::GREEN;

        if (is_building(build_colour)) {
          out << "building";
          build_count.building += 1;
        } else {
          out << "waiting";
          build_count.successful += 1;
        }
      }

      bool is_successful_build(string build_colour) {
        return build_colour.find("blue") != string::npos;
      }

      bool is_building(string build_colour) {
        return build_colour.find("anime") != string::npos;
      }

      bool has_been_built(string build_colour) {
        return build_colour.find("notbuilt") == string::npos;
      }

      ptree get_json(string uri, cic::config config) {

        auto response = http_client.get(uri, config.username, config.password);

        stringstream stream(response);

        ptree tree;
        boost::property_tree::json_parser::read_json(stream, tree);
        return tree;
      }

      ptree get_failed_build_actions(cic::config &config, string failed_build_url) {

        cerr << "Reading failed build " << failed_build_url << std::endl;

        auto job_tree = get_json(failed_build_url + "api/json?pretty=true", config);

        auto last_build_url = job_tree.get<string>("lastBuild.url");

        auto last_build = get_json(last_build_url + "api/json", config);

        return last_build.get_child("actions");
      }

      void print_actions(ostream &out, ptree &actions) {
        char const *indent = "   ";
        auto short_description = [&](const boost::property_tree::ptree::value_type &vt) -> void {
            if (vt.second.data().length() > 0) {
              out << indent << vt.second.data() << endl;
            }
        };
        auto user_id = [&](const boost::property_tree::ptree::value_type &vt) -> void {
            out << indent << "User Id: " << vt.second.data() << endl;
        };

        auto user_name = [&](const boost::property_tree::ptree::value_type &vt) -> void {
            out << indent << "User : " << vt.second.data() << endl;
        };

        auto claimed_by = [&](const boost::property_tree::ptree::value_type &vt) -> void {
            out << indent << "Claimed by: " << vt.second.data() << endl;
        };

        map<string, tree_visitor_type> x = {
            {"shortDescription", short_description},
            {"userId", user_id},
            {"userName", user_name},
            {"claimedBy", claimed_by}
        };

        out << endl;
        print_tree(actions, out, x);
        out << endl;
      }

      void print_tree(boost::property_tree::ptree &pt, ostream &out, map<string, tree_visitor_type> &x) {

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