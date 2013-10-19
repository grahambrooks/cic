#include <boost/test/unit_test.hpp>

namespace ci {

  class job {
  public:
    enum job_status {
      unknown
    };

    job_status status() {
      return job_status::unknown;
    }
  };
}

BOOST_AUTO_TEST_CASE(job_default_status_is_unknown)
{
  ci::job job;

  BOOST_CHECK_EQUAL(job.status(), ci::job::job_status::unknown);
}
