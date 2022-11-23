/**
 * @file threadpool.cpp
 * @brief Threadpool implementation (traits and some helper functions)
 */

#include <Windows.h>

#include <thread>

#include "threadpool.hpp"
#include "allocator.hpp"
#include "exception.hpp"


namespace ntp::details {

/**
 * @brief Get number of threads to use as default maximum for custom threadpool
 */
DWORD HardwareThreads()
{
    static int threads = static_cast<int>(std::thread::hardware_concurrency());
    if (threads <= 0)
    {
        threads = 4;
    }

    return (threads < 8) ? (threads * 4) : (threads * 2);
}


CustomThreadPoolTraits::CustomThreadPoolTraits(DWORD min_threads /* = 0 */, DWORD max_threads /* = 0 */)
    : pool_(nullptr)
    , environment_(environment_allocator_t::AllocateBytes(sizeof(TP_CALLBACK_ENVIRON_V3) + 512))
{
    pool_ = CreateThreadpool(nullptr);
    if (!pool_)
    {
        throw exception::Win32Exception();
    }

    //
    // Set threads count for new threadpool
    //

    min_threads = (min_threads) ? min_threads : 1;
    max_threads = (max_threads && max_threads >= min_threads)
                    ? max_threads
                    : HardwareThreads();

    //
    // If max_threads is still less than min_treads, then make them equal
    //

    max_threads = (max_threads >= min_threads) ? max_threads : min_threads;

    SetThreadpoolThreadMinimum(pool_, min_threads);
    SetThreadpoolThreadMaximum(pool_, max_threads);

    //
    // now initialize environment and link with the pool
    //

    InitializeThreadpoolEnvironment(environment_);
    SetThreadpoolCallbackPool(environment_, pool_);
}

CustomThreadPoolTraits::~CustomThreadPoolTraits()
{
    if (environment_)
    {
        environment_allocator_t::Free(environment_);
    }

    if (pool_)
    {
        CloseThreadpool(pool_);
    }
}


CleanupGroup::~CleanupGroup()
{
    if (cleanup_group_)
    {
        CloseThreadpoolCleanupGroup(cleanup_group_);
    }
}

}  // namespace ntp::details
