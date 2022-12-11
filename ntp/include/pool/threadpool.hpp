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

#include "ntp_config.hpp"
#include "details/allocator.hpp"
#include "pool/work.hpp"
#include "pool/wait.hpp"
#include "pool/timer.hpp"
#include "pool/io.hpp"


namespace ntp {
namespace details {

/**
 * @brief Default cancellation test implementation.
 * 
 * @returns always false
 */
inline bool DefaultTestCancel() noexcept
{
    return false;
}


/** 
 * @brief Basic threadpool traits. Used as system-default threadpool traits.
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
     * @brief Get a threadpool environment associated by default with the system threadpool.
     * 
     * @returns Pointer to the environment
     */
    PTP_CALLBACK_ENVIRON Environment() const noexcept { return environment_; }

private:
    // Environment associated with system threadpool (by default)
    PTP_CALLBACK_ENVIRON environment_;
};


/** 
 * @brief Traits for a custom threadpool.
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
     * @brief Constructor with the ability to set threadpool threads number.
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
          manage all callbacks at once.
 */
class CleanupGroup final
{
    CleanupGroup(const CleanupGroup&)            = delete;
    CleanupGroup& operator=(const CleanupGroup&) = delete;

public:
    /**
     * @brief Constructor, that initializes cleanup group and 
     *        associates it with a threadpool if necessary.
     * 
     * @param environment Threadpool environment to use
     */
    explicit CleanupGroup(PTP_CALLBACK_ENVIRON environment);

    ~CleanupGroup();

    /**
     * @brief Obtain a pointer to the internal cleanup group.
     */
    operator PTP_CLEANUP_GROUP() const noexcept { return cleanup_group_; }

private:
    // Internal cleanup group
    PTP_CLEANUP_GROUP cleanup_group_;
};

}  // namespace details


/**
 * @brief Opaque threadpool wait object descriptor.
 * 
 * Intentionally defined as native handle for WaitManager.
 */
using wait_t = wait::details::WaitManager::native_handle_t;


/**
 * @brief Opaque threadpool timer object descriptor.
 *
 * Intentionally defined as native handle for TimerManager.
 */
using timer_t = timer::details::TimerManager::native_handle_t;


/**
 * @brief Opaque threadpool IO object descriptor.
 *
 * Intentionally defined as native handle for IoManager.
 */
using io_t = io::details::IoManager::native_handle_t;


/**
 * @brief Basic threadpool class, that provides an interface 
 *        for interacting with a pool.
 *
 * Class needs to be instantiated with threadpool traits, that
 * define which threadpool to use: system-default or cutom.
 * One can write its own threadpool traits if necessary.
 * 
 * You can submit 4 different object types into threadpool:
 * - Work objects, that execute your arbitrary callback immediately.
 * 
 * - Wait objects, that execute your arbitrary callback, when specified handle 
 *   becomes signaled or timeout expires.
 * 
 * - Timer objects, that execute your arbitrary callback, when timer expires. 
 *   They may be scheduled for periodical calls. You may exchange callback 
 *   submitted earlier with a new one, and next time timer will call an updated one.
 * 
 * - IO objects, that execute your arbitrary callback, when asynchronous IO
 *   is completed.
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
        , timer_manager_(traits_.Environment())
        , io_manager_(traits_.Environment())
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
        , timer_manager_(traits_.Environment())
        , io_manager_(traits_.Environment())
    { }

    /**
     * @brief Destructor releases all forgotten (or not) resources via cleanup group
     */
    ~BasicThreadPool()
    {
        //
        // Managers dont cancel all their pending callbacks. They are cancelled and closed here.
        //

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
	 * @brief Submits a wait callback into threadpool.
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
     * @returns handle for created wait object
     */
    template<typename Rep, typename Period, typename Functor, typename... Args>
    wait_t SubmitWait(HANDLE wait_handle, const std::chrono::duration<Rep, Period>& timeout, Functor&& functor, Args&&... args)
    {
        return wait_manager_.Submit(wait_handle, timeout, std::forward<Functor>(functor),
            std::forward<Args>(args)...);
    }

    /**
	 * @brief Submits a wait callback into threadpool (timeout never expires).
	 *
	 * @tparam Functor Type of callable to invoke in threadpool
	 * @tparam Args... Types of arguments
	 * @param wait_handle Handle to wait for
	 * @param functor Callable to invoke
	 * @param args Arguments to pass into callable (they will be copied into wrapper)
	 * @returns handle for created wait object
     */
    template<typename Functor, typename... Args>
    auto SubmitWait(HANDLE wait_handle, Functor&& functor, Args&&... args)
        -> std::enable_if_t<!ntp::time::details::is_duration_v<Functor>, wait_t>
    {
        return wait_manager_.Submit(wait_handle, std::forward<Functor>(functor),
            std::forward<Args>(args)...);
    }

    /**
     * @brief Cancel threadpool wait
     * 
     * @param wait_object Handle for an existing wait object (obtained from ntp::BasicThreadPool::SubmitWait)
     */
    void CancelWait(wait_t wait_object) noexcept { return wait_manager_.Cancel(wait_object); }

    /**
     * @brief Cancel all pending wait callbacks
     */
    void CancelWaits() noexcept { return wait_manager_.CancelAll(); }


    /**
     * @brief Submits a threadpool timer object with a user-defined callback.
     *
     * @param timeout Timeout after which timer object calls the callback
     * @param period If non-zero, timer object willbe triggered each period after first call
     * @param functor Callable to invoke
     * @param args Arguments to pass into callable (they will be copied into wrapper)
     * @returns handle for created timer object
     */
    template<typename Rep1, typename Period1, typename Rep2, typename Period2, typename Functor, typename... Args>
    timer_t SubmitTimer(const std::chrono::duration<Rep1, Period1>& timeout, const std::chrono::duration<Rep2, Period2>& period, Functor&& functor, Args&&... args)
    {
        return timer_manager_.Submit(timeout, period, std::forward<Functor>(functor),
            std::forward<Args>(args)...);
    }

    /**
     * @brief Submits a non-periodic threadpool timer object with a user-defined callback.
	 *
	 * @param timeout Timeout after which timer object calls the callback
     * @param functor Callable to invoke
     * @param args Arguments to pass into callable (they will be copied into wrapper)
     * @returns handle for created timer object
     */
    template<typename Rep, typename Period, typename Functor, typename... Args>
    auto SubmitTimer(const std::chrono::duration<Rep, Period>& timeout, Functor&& functor, Args&&... args)
        -> std::enable_if_t<!ntp::time::details::is_duration_v<Functor> && !ntp::time::details::is_time_point_v<Functor>, timer_t>
    {
        return timer_manager_.Submit(timeout, std::forward<Functor>(functor),
            std::forward<Args>(args)...);
    }

    /**
     * @brief Submits a threadpool deadline timer object with a user-defined callback.
     *
     * If deadline is already gone, timer expires immediately.
     * 
     * @param deadline A specific point in time, which the timer will expire at
     * @param period If non-zero, timer object willbe triggered each period after first call
     * @param functor Callable to invoke
     * @param args Arguments to pass into callable (they will be copied into wrapper)
     * @returns handle for created timer object
     */
    template<typename Duration, typename Rep, typename Period, typename Functor, typename... Args>
    auto SubmitTimer(const ntp::time::deadline_t<Duration>& deadline, const std::chrono::duration<Rep, Period>& period, Functor&& functor, Args&&... args)
    {
        return timer_manager_.Submit(deadline, period, std::forward<Functor>(functor),
            std::forward<Args>(args)...);
    }

    /**
     * @brief Submits a non-periodic threadpool deadline timer object with a user-defined callback.
     *
     * If deadline is already gone, timer expires immediately.
     *
     * @param deadline A specific point in time, which the timer will expire at
     * @param functor Callable to invoke
     * @param args Arguments to pass into callable (they will be copied into wrapper)
     * @returns handle for created timer object
     */
    template<typename Duration, typename Functor, typename... Args>
    auto SubmitTimer(const ntp::time::deadline_t<Duration>& deadline, Functor&& functor, Args&&... args)
        -> std::enable_if_t<!ntp::time::details::is_duration_v<Functor> && !ntp::time::details::is_time_point_v<Functor>, timer_t>
    {
        return timer_manager_.Submit(deadline, std::forward<Functor>(functor),
            std::forward<Args>(args)...);
    }

    /**
     * @brief Replaces an existing timer callback in threadpool.
     *        This method cannot be called concurrently for the same timer object.
     *
     * @tparam Functor Type of callable to invoke in threadpool
     * @tparam Args... Types of arguments
     * @param timer_object Handle for an existing timer object (obtained from ntp::BasicThreadPool::SubmitTimer)
     * @param functor Callable to invoke
     * @param args Arguments to pass into callable (they will be copied into wrapper)
     * @throws exception::Win32Exception if specified handle is not present in waits or is corrupt
     * @returns handle for the same timer object
     */
    template<typename Functor, typename... Args>
    timer_t ReplaceTimer(timer_t timer_object, Functor&& functor, Args&&... args)
    {
        return timer_manager_.Replace(timer_object, std::forward<Functor>(functor),
            std::forward<Args>(args)...);
    }

    /**
     * @brief Cancel threadpool timer
     *
     * @param timer_object Handle for an existing timer object (obtained from ntp::BasicThreadPool::SubmitTimer)
     */
    void CancelTimer(timer_t timer_object) noexcept { return timer_manager_.Cancel(timer_object); }

    /**
     * @brief Cancel all pending timer callbacks
     */
    void CancelTimers() noexcept { return timer_manager_.CancelAll(); }


    /**
     * @brief Submits a threadpool IO object with a user-defined callback.
     *
     * If after call to this function async IO failed to start you MUST call
     * ntp::BasicThreadPool::AbortIo to prevent memory leaks).
     * 
     * @param io_handle Handle of and object, wchich asynchronous IO is performed on
     * @param functor Callable to invoke
     * @param args Arguments to pass into callable (they will be copied into wrapper)
     * @returns handle for created IO object
     */
    template<typename Functor, typename... Args>
    io_t SubmitIo(HANDLE io_handle, Functor&& functor, Args&&... args)
    {
        return io_manager_.Submit(io_handle, std::forward<Functor>(functor),
            std::forward<Args>(args)...);
    }

    /**
     * @brief Cancel threadpool IO
     *
     * @param io_object Handle for an existing IO object (obtained from ntp::BasicThreadPool::SubmitIo)
     */
    void CancelIo(io_t io_object) noexcept { return io_manager_.Cancel(io_object); }

    /**
     * @brief Cancel threadpool IO if async IO failed to start.
     *
     * @param io_object Handle for an existing IO object (obtained from ntp::BasicThreadPool::SubmitIo)
     */
    void AbortIo(io_t io_object) noexcept { return io_manager_.Abort(io_object); }

    /**
     * @brief Cancel all pending IO callbacks
     */
    void CancelIos() noexcept { return io_manager_.CancelAll(); }


    /**
     * @brief Cancel all pending callbacks (of any kind)
     */
    void CancelAllCallbacks() noexcept
    {
        work_manager_.CancelAll();
        wait_manager_.CancelAll();
        timer_manager_.CancelAll();
        io_manager_.CancelAll();
    }

private:
    // Treadpool traits
    traits_t traits_;

    // Cleanup group for callbacks
    details::CleanupGroup cleanup_group_;

    // Cancellation test function
    details::test_cancel_t test_cancel_;

    // Managers for callbacks
    work::details::WorkManager work_manager_;
    wait::details::WaitManager wait_manager_;
    timer::details::TimerManager timer_manager_;
    io::details::IoManager io_manager_;
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
