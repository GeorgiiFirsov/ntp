#pragma once

#include <Windows.h>

#include "allocator.hpp"


namespace ntp {
namespace details {

struct SystemThreadPoolTraits final
{
    PTP_CALLBACK_ENVIRON Environment() const noexcept { return nullptr; }
};


struct CustomThreadPoolTraits final
{
private:
    using environment_allocator_t = allocator::HeapAllocator<TP_CALLBACK_ENVIRON>;

public:
    CustomThreadPoolTraits();
    ~CustomThreadPoolTraits();

    CustomThreadPoolTraits(const CustomThreadPoolTraits&)            = delete;
    CustomThreadPoolTraits& operator=(const CustomThreadPoolTraits&) = delete;

    PTP_CALLBACK_ENVIRON Environment() const noexcept { return environment_; }

private:
    PTP_POOL pool_;

    PTP_CALLBACK_ENVIRON environment_;
};

}  // namespace details


template<typename ThreadPoolTraits>
class BasicThreadPool final
{
    using traits_t = ThreadPoolTraits;

public:

private:
    traits_t traits_;
};


using SystemThreadPool = BasicThreadPool<details::SystemThreadPoolTraits>;
using ThreadPool       = BasicThreadPool<details::CustomThreadPoolTraits>;

}  // namespace ntp
