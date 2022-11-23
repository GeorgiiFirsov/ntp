#include <Windows.h>

#include <thread>

#include "threadpool.hpp"
#include "allocator.hpp"
#include "exception.hpp"


namespace ntp::details {

DWORD HardwareThreads()
{
    static int threads = static_cast<int>(std::thread::hardware_concurrency());
    if (threads <= 0)
    {
        threads = 4;
    }

    return (threads < 8) ? (threads * 4) : (threads * 2);
}


CustomThreadPoolTraits::CustomThreadPoolTraits()
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

    SetThreadpoolThreadMinimum(pool_, 1);
    SetThreadpoolThreadMaximum(pool_, HardwareThreads());

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

}  // namespace ntp::details
