/**
 * @file work.hpp
 * @brief Work callback wrapper and manager implementation
 */

#pragma once

#include <Windows.h>
#include <atlsync.h>

#include <tuple>
#include <memory>
#include <utility>
#include <type_traits>

#include "config.hpp"
#include "details/utils.hpp"
#include "details/allocator.hpp"
#include "pool/basic_callback.hpp"


namespace ntp::work::details {

/**
 * @brief Work callback wrapper (PTP_WORK)
 * 
 * @tparam Functor Type of callable to invoke in threadpool
 * @tparam Args... Types of arguments
 */
template<typename Functor, typename... Args>
class alignas(NTP_ALLOCATION_ALIGNMENT) Callback final
    : public ntp::details::BasicCallback<Functor, Args...>
{
public:
    /**
     * @brief Constructor from callable and its arguments
     * 
     * @param functor Callable to invoke
     * @param args Arguments to pass into callable (they will be copied into wrapper)
     */
    template<typename CFunctor, typename... CArgs>
    explicit Callback(CFunctor&& functor, CArgs&&... args)
        : BasicCallback(std::forward<CFunctor>(functor), std::forward<CArgs>(args)...)
    { }

    /**
     * @brief Invokation of internal callback (interface's parameter ignored)
     */
    void Call(void* /*parameter*/) override
    {
        std::apply(Callable(), Arguments());
    }
};


/**
 * @brief Manager for work callbacks. Binds callbacks and threadpool implementation.
 */
class Manager final
    : public ntp::details::BasicManager
{
    Manager(const Manager&)            = delete;
    Manager& operator=(const Manager&) = delete;

public:
    /**
     * @brief Constructor that initializes all necessary objects.
     * 
	 * @param environment Owning threadpool traits
	 * @param test_cancel Reference to cancellation test function (defaulted to ntp::details::DefaultTestCancel)
     */
    explicit Manager(PTP_CALLBACK_ENVIRON environment, const ntp::details::test_cancel_t& test_cancel);

    ~Manager();

    /**
     * @brief Submits a callback into threadpool.
     * 
     * Creates a callback wrapper and pushes it into a queue,
     * then submits PTP_WORK into corresponding threadpool.
	 *
     * @tparam Functor Type of callable to invoke in threadpool
	 * @tparam Args... Types of arguments
	 * @param functor Callable to invoke
	 * @param args Arguments to pass into callable (they will be copied into wrapper)
     */
    template<typename Functor, typename... Args>
    void Submit(Functor&& functor, Args&&... args)
    {
        const auto callback = new Callback<Functor, Args...>(
            std::forward<Functor>(functor), std::forward<Args>(args)...);

        queue_.Push(callback);
        SubmitThreadpoolWork(work_);
    }

    /**
     * @brief Wait for all callbacks to complete
     * 
     * Wait is performed in separate thread if possible with periodical cancellation checks.
     * If waiting in separate thread is not possible, wait is performed in caller thread,
     * cancellation checks are impossible in this case (error message is reported to logger).
     * 
     * @returns true if all callbacks are completed, false if cancellation occurred while waiting for callbacks
     */
    bool WaitAll() noexcept;

    /**
     * @brief Cancel all pending callbacks
     */
    void CancelAll() noexcept;

private:
    size_t ClearList() noexcept;

private:
    static void NTAPI InvokeCallback(PTP_CALLBACK_INSTANCE instance, PSLIST_HEADER queue, PTP_WORK work);

    static void CALLBACK WaitAllCallback(PTP_CALLBACK_INSTANCE instance, Manager* self);

private:
    // Internal queue with callbacks
    ntp::details::NativeSlist queue_;

    // Internal callback descriptor (one for every callback in the pool)
    PTP_WORK work_;

    // Event: all tasks completed
    ATL::CEvent done_event_;

    // Reference to cancellation test function
    const ntp::details::test_cancel_t& test_cancel_;
};

}  // namespace ntp::work::details
