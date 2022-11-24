#include <atlsync.h>

#include <atomic>

#include "gtest/gtest.h"

#include "ntp.hpp"


namespace work {

TEST(Work, Submit)
{
    const auto Worker = [](int& counter) {
        counter++;
    };

    int counter = 0;

    ntp::SystemThreadPool pool;
    pool.SubmitWork(Worker, std::ref(counter));
    pool.WaitWorks();

    EXPECT_EQ(counter, 1);
}

TEST(Work, SubmitMultiple)
{
    static constexpr auto kWorkers = 50;

    const auto Worker = [](std::atomic_int& counter) {
        counter++;
    };

    std::atomic_int counter = 0;
    ntp::SystemThreadPool pool;

    for (auto i = 0; i < kWorkers; ++i)
    {
        pool.SubmitWork(Worker, std::ref(counter));
    }

    pool.WaitWorks();

    EXPECT_EQ(counter, kWorkers);
}

TEST(Work, Cancel)
{
	static constexpr auto kWorkers = 50;

	const auto Worker = [](std::atomic_int& counter) {
		counter++;
	};

	std::atomic_int counter = 0;
	ntp::SystemThreadPool pool;

	for (auto i = 0; i < kWorkers; ++i)
	{
		pool.SubmitWork(Worker, std::ref(counter));
	}

	pool.CancelWorks();

	EXPECT_LE(counter, kWorkers);
}

}  // namespace work
