/**
 * @file threadpool.hpp
 * @brief Threadpool implementations
 *
 * This file contains an implementations of a basic threadpool 
 * with its out-of-box instantiations.
 */

#pragma once

#include <Windows.h>

#include <chrono>
#include <type_traits>

#include "config.hpp"
#include "details/allocator.hpp"
#include "pool/work.hpp"
#include "pool/wait.hpp"


namespace ntp {
namespace details {

/**
 * @brief Default cancellation test implementation
 * 
 * @returns always false
 */
inline bool DefaultTestCancel() noexcept
{
    return false;
}


/** 
 * @brief Basic threadpool traits. Used as system-default threadpool traits
 *  
 * Each Win32 process has its own default threadpool.
 * These traits allow user to use it via SystemThreadPool 
 * class.
 */
class BasicThreadPoolTraits
{
    // Allocator for TP_CALLBACK_ENVIRON
    using environment_allocator_t = allocator::HeapAllocator<TP_CALLBACK_ENVIRON>;

private:
    BasicThreadPoolTraits(const BasicThreadPoolTraits&)            = delete;
    BasicThreadPoolTraits& operator=(const BasicThreadPoolTraits&) = delete;

public:
    BasicThreadPoolTraits();
    ~BasicThreadPoolTraits();

    /**
     * @brief Get a threadpool environment associated by default with the system threadpool
     * 
     * @returns Pointer to the environment
     */
    PTP_CALLBACK_ENVIRON Environment() const noexcept { return environment_; }

private:
    // Environment associated with system threadpool (by default)
    PTP_CALLBACK_ENVIRON environment_;
};


/** 
 * @brief Traits for a custom threadpool
 * 
 * These traits create a new threadpool in constructor
 * and then allow user to interact with this pool
 * via ThreadPool class.
 */
class CustomThreadPoolTraits final
    : public BasicThreadPoolTraits
{
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

private:
    // Custom threadpool descriptor
    PTP_POOL pool_;
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
    // Traits must inherit details::BasicThreadPoolTraits
    static_assert(std::is_base_of_v<details::BasicThreadPoolTraits, ThreadPoolTraits> || std::is_same_v<details::BasicThreadPoolTraits, ThreadPoolTraits>,
        "[BasicThreadPool]: ThreadPoolTraits MUST inherit details::BasicThreadPoolTraits");

    // Alias for traits type
    using traits_t = ThreadPoolTraits;

private:
    BasicThreadPool(const BasicThreadPool&)            = delete;
    BasicThreadPool& operator=(const BasicThreadPool&) = delete;

public:
    /**
     * @brief Default constructor
     * 
	 * Initializes stateful threadpool traits and cleanup group
     * 
	 * @param test_cancel Cancellation test function (defaulted to ntp::details::DefaultTestCancel)
     */
    explicit BasicThreadPool(details::test_cancel_t test_cancel = details::DefaultTestCancel)
        : traits_()
        , cleanup_group_(traits_.Environment())
        , test_cancel_(std::move(test_cancel))
        , work_manager_(traits_.Environment())
        , wait_manager_(traits_.Environment())
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
	 * @param test_cancel Cancellation test function (defaulted to ntp::details::DefaultTestCancel)
     */
    template<typename = std::enable_if_t<std::is_same_v<traits_t, details::CustomThreadPoolTraits>>>
    explicit BasicThreadPool(DWORD min_threads, DWORD max_threads, details::test_cancel_t test_cancel = details::DefaultTestCancel)
        : traits_(min_threads, max_threads)
        , cleanup_group_(traits_.Environment())
        , test_cancel_(std::move(test_cancel))
        , work_manager_(traits_.Environment())
        , wait_manager_(traits_.Environment())
    { }

    /**
     * @brief Destructor releases all forgotten (or not) resources via cleanup group
     */
    ~BasicThreadPool()
    {
        ntp::details::SafeThreadpoolCall<CloseThreadpoolCleanupGroupMembers>(
            cleanup_group_, TRUE, nullptr);
    }

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

    /**
     * @brief Waits until all work callbacks are completed or cancellation is requested
     *
     * @returns true if all callbacks are completed, false if cancellation
     *          occurred while waiting for callbacks
     */
    bool WaitWorks() noexcept { return work_manager_.WaitAll(test_cancel_); }

    /**
     * @brief Cancel all pending work callbacks
     */
    void CancelWorks() noexcept { return work_manager_.CancelAll(); }


    /**
	 * @brief Submits or replaces a wait callback of default type into threadpool.
	 *
	 * @tparam Rep Type of ticks representation for timeout
	 * @tparam Period Type of period for timeout ticks
	 * @tparam Functor Type of callable to invoke in threadpool
	 * @tparam Args... Types of arguments
	 * @param wait_handle Handle to wait for
	 * @param timeout Timeout while wait object waits for the specified handle
	 *                (pass ntp::time::max_native_duration for infinite wait timeout)
	 * @param functor Callable to invoke
	 * @param args Arguments to pass into callable (they will be copied into wrapper)
     */
    template<typename Rep, typename Period, typename Functor, typename... Args>
    void SubmitWait(HANDLE wait_handle, const std::chrono::duration<Rep, Period>& timeout, Functor&& functor, Args&&... args)
    {
        return wait_manager_.Submit(wait_handle, timeout, std::forward<Functor>(functor),
            std::forward<Args>(args)...);
    }

    /**
	 * @brief Submits or replaces a wait callback into threadpool (timeout never expires).
	 *
	 * @tparam Functor Type of callable to invoke in threadpool
	 * @tparam Args... Types of arguments
	 * @param wait_handle Handle to wait for
	 * @param functor Callable to invoke
	 * @param args Arguments to pass into callable (they will be copied into wrapper)
     */
    template<typename Functor, typename... Args>
    auto SubmitWait(HANDLE wait_handle, Functor&& functor, Args&&... args)
        -> std::enable_if_t<!ntp::time::details::is_duration_v<Functor>>
    {
        return wait_manager_.Submit(wait_handle, std::forward<Functor>(functor),
            std::forward<Args>(args)...);
    }

    /**
	 * @brief Replaces an existing wait callback in threadpool.
	 *
	 * @tparam Functor Type of callable to invoke in threadpool
	 * @tparam Args... Types of arguments
	 * @param wait_handle Handle to wait for
	 * @param functor Callable to invoke
	 * @param args Arguments to pass into callable (they will be copied into wrapper)
     * @throws exception::Win32Exception if specified handle is not present in waits
     */
	template<typename Functor, typename... Args>
    void Replace(HANDLE wait_handle, Functor&& functor, Args&&... args)
    {
        return wait_manager_.Replace(wait_handle, std::forward<Functor>(functor),
            std::forward<Args>(args)...);
    }

    /**
     * @brief Cancel threadpool wait for the specified handle
     */
    void CancelWait(HANDLE wait_handle) noexcept { return wait_manager_.Cancel(wait_handle); }

    /**
     * @brief Cancel all pending wait callbacks
     */
    void CancelWaits() noexcept { return wait_manager_.CancelAll(); }


    /**
     * @brief Cancel all pending callbacks (of any kind)
     */
    void CancelAllCallbacks() noexcept
    {
        work_manager_.CancelAll();
        wait_manager_.CancelAll();
    }

private:
    // Treadpool traits
    traits_t traits_;

    // Cleanup group for callbacks
    details::CleanupGroup cleanup_group_;

    // Cancellation test function
    details::test_cancel_t test_cancel_;

    // Managers for callbacks
    work::details::Manager work_manager_;
    wait::details::Manager wait_manager_;
};


/**
 * @brief System-default threadpool wrapper
 */
using SystemThreadPool = BasicThreadPool<details::BasicThreadPoolTraits>;

/**
 * @brief Custom threadpool wrapper
 */
using ThreadPool = BasicThreadPool<details::CustomThreadPoolTraits>;

}  // namespace ntp
