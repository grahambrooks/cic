#include <boost/test/unit_test.hpp>

#include "command_line_options_parser.hpp"

BOOST_AUTO_TEST_CASE(recognises_verbose_output_option) {
  ci::command_line_options_parser parser;
  const char *argv[] = {"ci", "--verbose"};
  auto config = parser.parse(2, argv);

  BOOST_CHECK(config.verbose_output());
}

BOOST_AUTO_TEST_CASE(recognises_server_address_as_an_option) {
  ci::command_line_options_parser parser;
  const char *argv[] = {"ci", "--server=http://server.com/path"};
  auto config = parser.parse(2, argv);

  BOOST_CHECK_EQUAL("http://server.com/path", config.server_url);
}

BOOST_AUTO_TEST_CASE(recognises_username_as_an_option) {
  ci::command_line_options_parser parser;
  const char *argv[] = {"ci", "--username=graham"};
  auto config = parser.parse(2, argv);

  BOOST_CHECK_EQUAL("graham", config.username);
}

BOOST_AUTO_TEST_CASE(recognises_username_short_option) {
  ci::command_line_options_parser parser;
  const char *argv[] = {"ci", "--u=graham"};
  auto config = parser.parse(2, argv);

  BOOST_CHECK_EQUAL("graham", config.username);
}

BOOST_AUTO_TEST_CASE(recognises_password_as_an_option) {
  ci::command_line_options_parser parser;
  const char *argv[] = {"ci", "--password=passwd"};
  auto config = parser.parse(2, argv);

  BOOST_CHECK_EQUAL("passwd", config.password);
}

BOOST_AUTO_TEST_CASE(recognises_password_short_option) {
  ci::command_line_options_parser parser;
  const char *argv[] = {"ci", "--p=passwd"};
  auto config = parser.parse(2, argv);

  BOOST_CHECK_EQUAL("passwd", config.password);
}

BOOST_AUTO_TEST_CASE(sets_help_request_for_help_switch) {
  boost::program_options::options_description desc;

  desc.add_options()
      ("help", "display this option");

  boost::program_options::variables_map variables;

  const char *argv[] = {"ci", "--help"};

  boost::program_options::store(boost::program_options::parse_command_line(2, argv, desc), variables);

  boost::program_options::notify(variables);

  BOOST_CHECK(variables.count("help") > 0);
}

