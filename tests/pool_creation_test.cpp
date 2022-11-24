#include "gtest/gtest.h"

#include "ntp.hpp"


TEST(ThreadPool, Creation_System)
{
    EXPECT_NO_THROW({
        ntp::SystemThreadPool pool;
    });
}

TEST(ThreadPool, Creation_SystemTestCancel)
{
    const auto TestCancel = []() { return false; };

    EXPECT_NO_THROW({
        ntp::SystemThreadPool pool(TestCancel);
    });
}

TEST(ThreadPool, Creation_Custom)
{
    EXPECT_NO_THROW({
        ntp::ThreadPool pool;
    });
}

TEST(ThreadPool, Creation_CustomMinMax)
{
    EXPECT_NO_THROW({
        ntp::ThreadPool pool(1, 10);
    });
}

TEST(ThreadPool, Creation_CustomTestCancel)
{
    const auto TestCancel = []() { return false; };

    EXPECT_NO_THROW({
        ntp::ThreadPool pool(TestCancel);
    });
}

TEST(ThreadPool, Creation_CustomMinMaxTestCancel)
{
    const auto TestCancel = []() { return false; };

    EXPECT_NO_THROW({
        ntp::ThreadPool pool(1, 10, TestCancel);
    });
}
