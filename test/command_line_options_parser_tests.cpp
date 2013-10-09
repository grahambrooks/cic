#include <boost/test/unit_test.hpp>

#include <boost/program_options.hpp>

BOOST_AUTO_TEST_CASE(sets_help_request_for_help_switch) {
  boost::program_options::options_description desc;
  
  desc.add_options()
  ("help", "display this option");
  
  boost::program_options::variables_map variables;
  
  const char* argv[] = {"--help"};
  
  boost::program_options::store(boost::program_options::parse_command_line(1, argv, desc), variables);
  
  boost::program_options::notify(variables);
  
  if (variables.count("help")) {
    std::cout << desc << std::endl;
  }
}