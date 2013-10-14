#pragma once

#include <initializer_list>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>

using namespace std;

namespace ci {
  class config {
  private:
    bool verbose;
    bool help_only;
    bool version_only;
  public:
    string server_type;
    string server_url;
    string username;
    string password;


    config() {
      verbose = false;
      help_only = false;
    }

    config(string server_type, string server_url, string username = "", string password = "") : server_type(server_type), server_url(server_url), username(username), password(password) {
      verbose = false;
      help_only = false;
    }

    static ci::config from_string(const string config_text) {
      stringstream ss(config_text);

      return load_from(ss);
    }

    bool is_valid() {
      return !server_type.empty() && !server_url.empty();
    }


    static ci::config load(const boost::filesystem::path configuration_path) {
      ifstream input(configuration_path.c_str());

      return load_from(input);
    }

    static ci::config load_from(istream &ss) {
      ci::config result;
      boost::property_tree::ptree pt;
      boost::property_tree::read_json(ss, pt);

      result.server_type = pt.get<string>("server.type");
      result.server_url = pt.get<string>("server.url");

      if (pt.get_optional<string>("server.username") != NULL) {
        result.username = pt.get<string>("server.username");
      }

      if (pt.get_optional<string>("server.password") != NULL) {
        result.password = pt.get<string>("server.password");
      }

      return result;
    }

    config &merge(const config &other) {
      server_type = server_type.empty() ? other.server_type : server_type;
      server_url = server_url.empty() ? other.server_url : server_url;
      username = username.empty() ? other.username : username;
      password = password.empty() ? other.password : password;
      verbose |= other.verbose;
      help_only |= other.help_only;

      return *this;
    }

    void set_verbose(bool b) {
      verbose = b;
    }

    bool verbose_output() {
      return verbose;
    }

    void set_help_only() {
      help_only = true;
    }

    bool just_need_help() {
      return help_only;
    }

    void set_version_only() {
      version_only = true;
    }

    bool show_version_only() {
      return version_only;
    }
  };
}
