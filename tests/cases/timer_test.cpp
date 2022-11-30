#include "test_config.hpp"


TEST(Timer, Submit)
{
    using namespace std::chrono_literals;

    std::atomic_int counter = 0;
    ntp::SystemThreadPool pool;

    pool.SubmitTimer(2ms, [&counter]() {
        ++counter;
    });

    std::this_thread::sleep_for(40ms);

    EXPECT_EQ(counter, 1);
}

TEST(Timer, Periodic)
{
    using namespace std::chrono_literals;

    std::atomic_int counter = 0;
    ntp::SystemThreadPool pool;

    const auto timer = pool.SubmitTimer(2ms, 2ms, [&counter]() {
        ++counter;
    });

    std::this_thread::sleep_for(40ms);

    EXPECT_GT(counter, 1);
}

TEST(Timer, Replace)
{
    using namespace std::chrono_literals;

    std::atomic_int counter = 0;
    ntp::SystemThreadPool pool;

    const auto timer = pool.SubmitTimer(10ms, []() {});
    pool.ReplaceTimer(timer, [&counter]() {
        ++counter;
    });

    std::this_thread::sleep_for(50ms);

    EXPECT_EQ(counter, 1);
}
