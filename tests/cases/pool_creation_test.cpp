#include "test_config.hpp"


TEST(ThreadPool, System)
{
    EXPECT_NO_THROW({
        ntp::SystemThreadPool pool;
    });
}

TEST(ThreadPool, SystemTestCancel)
{
    const auto TestCancel = []() { return false; };

    EXPECT_NO_THROW({
        ntp::SystemThreadPool pool(TestCancel);
    });
}

TEST(ThreadPool, Custom)
{
    EXPECT_NO_THROW({
        ntp::ThreadPool pool;
    });
}

TEST(ThreadPool, CustomMinMax)
{
    EXPECT_NO_THROW({
        ntp::ThreadPool pool(1, 10);
    });
}

TEST(ThreadPool, CustomTestCancel)
{
    const auto TestCancel = []() { return false; };

    EXPECT_NO_THROW({
        ntp::ThreadPool pool(TestCancel);
    });
}

TEST(ThreadPool, CustomMinMaxTestCancel)
{
    const auto TestCancel = []() { return false; };

    EXPECT_NO_THROW({
        ntp::ThreadPool pool(1, 10, TestCancel);
    });
}
