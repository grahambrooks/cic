#include <boost/test/unit_test.hpp>

namespace ci {

  class job {
  public:
    enum status_type {
      unknown,
      successful
    } job_status;
    std::string name;
    std::list<std::string> artifacts;
    job(std::string name, status_type status = unknown) : name(name), job_status(status) {
    }

    status_type status() {
      return job_status;
    }

    std::string get_name() {
      return name;
    }

    const std::list<std::string>& get_artifacts() {
      return artifacts;
    }
  };
}

BOOST_AUTO_TEST_CASE(job_default_status_is_unknown)
{
  ci::job job("");

  BOOST_CHECK_EQUAL(job.status(), ci::job::status_type::unknown);
}

BOOST_AUTO_TEST_CASE(status_set_during_creation) {
  ci::job job("", ci::job::status_type::successful);

  BOOST_CHECK_EQUAL(job.status(), ci::job::status_type::successful);
}

BOOST_AUTO_TEST_CASE(jobs_are_named) {
  ci::job job("foo");

  BOOST_CHECK_EQUAL(job.get_name(), "foo");
}

BOOST_AUTO_TEST_CASE(jobs_have_artifacts) {
  ci::job job("foo");

  BOOST_CHECK_EQUAL(job.get_artifacts().size(), 0);
}