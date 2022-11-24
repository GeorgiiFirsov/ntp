#include <atlsync.h>

#include "gtest/gtest.h"

#include "ntp.hpp"


namespace work {

TEST(Work, Submit)
{
    ATL::CEvent event(TRUE, FALSE);

    const auto Worker = [&event](int& counter) {
        counter++;
        event.Set();
    };

    int counter = 0;

    ntp::SystemThreadPool pool;
    pool.SubmitWork(Worker, std::ref(counter));

    WaitForSingleObject(event, INFINITE);

    EXPECT_EQ(counter, 1);
}

}  // namespace work
