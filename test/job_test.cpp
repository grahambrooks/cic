#include <boost/test/unit_test.hpp>

namespace ci {

  class job {
  public:
    enum status_type {
      unknown,
      successful
    } job_status;

    job(status_type status = unknown) : job_status(status) {
    }

    status_type status() {
      return job_status;
    }
  };
}

BOOST_AUTO_TEST_CASE(job_default_status_is_unknown)
{
  ci::job job;

  BOOST_CHECK_EQUAL(job.status(), ci::job::status_type::unknown);
}

BOOST_AUTO_TEST_CASE(status_set_during_creation) {
  ci::job job(ci::job::status_type::successful);

  BOOST_CHECK_EQUAL(job.status(), ci::job::status_type::successful);
}