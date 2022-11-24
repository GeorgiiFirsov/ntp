/**
 * @file threadpool.hpp
 * @brief Threadpool implementations
 *
 * This file contains an implementations of a basic threadpool 
 * with its out-of-box instantiations.
 */

#pragma once

#include <Windows.h>

#include <type_traits>

#include "config.hpp"
#include "allocator.hpp"
#include "work.hpp"


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
    // Allocator for TP_CALLBACK_ENVIRON
    using environment_allocator_t = allocator::HeapAllocator<TP_CALLBACK_ENVIRON>;

private:
    CustomThreadPoolTraits(const CustomThreadPoolTraits&)            = delete;
    CustomThreadPoolTraits& operator=(const CustomThreadPoolTraits&) = delete;

public:
    /**
     * @brief Constructor with the ability to set threadpool threads number
     * 
     * If min_threads is 0, then minimum number of threads will be set to 1.
     * If max_threads is 0 or less than min_threads, then maximum number of threads will 
     * be set to ntp::details::HardwareThreads.
     * If max_threads is still less than min_threads after changes described above, then
     * maximum number of threads is set to be equal to minimum.
     * 
     * @param min_threads Minimum number of threads
     * @param max_threads Maximum number of threads
     */
    CustomThreadPoolTraits(DWORD min_threads = 0, DWORD max_threads = 0);
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


/**
 * @brief Wrapper for PTP_CLEANUP_GROUP, that is used to 
          manage all callbacks at once
 */
class CleanupGroup final
{
    CleanupGroup(const CleanupGroup&)            = delete;
    CleanupGroup& operator=(const CleanupGroup&) = delete;

public:
    /**
     * @brief Constructor, that initializes cleanup group and 
     *        associates it with a threadpool if necessary
     * 
     * @param environment Threadpool environment to use
     */
    explicit CleanupGroup(PTP_CALLBACK_ENVIRON environment);

    ~CleanupGroup();

    /**
     * @brief Obtain a pointer to the internal cleanup group
     */
    operator PTP_CLEANUP_GROUP() const noexcept { return cleanup_group_; }

private:
    // Internal cleanup group
    PTP_CLEANUP_GROUP cleanup_group_;
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
    // Alias for traits type
    using traits_t = ThreadPoolTraits;

public:
    /**
     * @brief Default constructor
     * 
     * Initializes stateful threadpool traits and cleanup group
     */
    explicit BasicThreadPool()
        : traits_()
        , cleanup_group_(traits_.Environment())
        , work_manager_(traits_.Environment())
    { }

    /**
     * @brief Constructor with the ability to set threadpool threads number
     * 
     * This constructor is available only for BasicThreadPool<details::CustomThreadPoolTraits>
     * specialization. For the description of parameters refer to details::CustomThreadPoolTraits
     * description.
	 *
	 * @param min_threads Minimum number of threads
	 * @param max_threads Maximum number of threads
     */
    template<typename = std::enable_if_t<std::is_same_v<traits_t, details::CustomThreadPoolTraits>>>
    explicit BasicThreadPool(DWORD min_threads, DWORD max_threads)
        : traits_(min_threads, max_threads)
        , cleanup_group_(traits_.Environment())
        , work_manager_(traits_.Environment())
    { }

    /**
	 * @brief Submits a work callback into threadpool.
	 *
	 * @tparam Functor Type of callable to invoke in threadpool
	 * @tparam Args... Types of arguments
	 * @param functor Callable to invoke
	 * @param args Arguments to pass into callable (they will be copied into wrapper)
     */
    template<typename Functor, typename... Args>
    void SubmitWork(Functor&& functor, Args&&... args)
    {
        return work_manager_.Submit(std::forward<Functor>(functor),
            std::forward<Args>(args)...);
    }

private:
    // Treadpool traits
    traits_t traits_;

    // Cleanup group for callbacks
    details::CleanupGroup cleanup_group_;

    // Managers for callbacks
    work::details::Manager work_manager_;
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
