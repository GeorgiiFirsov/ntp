/**
 * @file threadpool.hpp
 * @brief Threadpool implementations
 *
 * This file contains an implementations of a basic threadpool 
 * with its out-of-box instantiations.
 */

#pragma once

#include <Windows.h>

#include "config.hpp"
#include "allocator.hpp"


namespace ntp {
namespace details {

/** 
 * @brief Traits for system-default threadpool
 *  
 * Each Win32 process has its own default threadpool.
 * These traits allow user to use it via SystemThreadPool 
 * class.
 */
struct SystemThreadPoolTraits final
{
    /**
     * @brief Get an environment associated with the system-default threadpool
     * 
     * @returns Pointer to the environment (actually NULL-pointer)
     */
    PTP_CALLBACK_ENVIRON Environment() const noexcept { return nullptr; }
};


/** 
 * @brief Traits for a custom threadpool
 * 
 * These traits create a new threadpool in constructor
 * and then allow user to interact with this pool
 * via ThreadPool class.
 */
class CustomThreadPoolTraits final
{
    using environment_allocator_t = allocator::HeapAllocator<TP_CALLBACK_ENVIRON>;

private:
    CustomThreadPoolTraits(const CustomThreadPoolTraits&)            = delete;
    CustomThreadPoolTraits& operator=(const CustomThreadPoolTraits&) = delete;

public:
    CustomThreadPoolTraits();
    ~CustomThreadPoolTraits();

    /** 
     * @brief Get an environment associated with the custom threadpool
     * 
     * @returns Pointer to the environment
     */
    PTP_CALLBACK_ENVIRON Environment() const noexcept { return environment_; }

private:
    // Custom threadpool descriptor
    PTP_POOL pool_;

    // Environment associated with custom threadpool
    PTP_CALLBACK_ENVIRON environment_;
};

}  // namespace details


/**
 * @brief Basic threadpool class, that provides an interface 
 *        for interacting with a pool
 *
 * Class needs to be instantiated with threadpool traits, that
 * define which threadpool to use: system-default or cutom.
 * One can write its own threadpool traits if necessary.
 * 
 * @tparam ThreadPoolTraits Threadpool traits, that define
 *                          actual implementation internals
 */
template<typename ThreadPoolTraits>
class BasicThreadPool final
{
    using traits_t = ThreadPoolTraits;

public:

private:
    // Treadpool traits
    traits_t traits_;
};


/**
 * @brief System-default threadpool wrapper
 */
using SystemThreadPool = BasicThreadPool<details::SystemThreadPoolTraits>;

/**
 * @brief Custom threadpool wrapper
 */
using ThreadPool = BasicThreadPool<details::CustomThreadPoolTraits>;

}  // namespace ntp
