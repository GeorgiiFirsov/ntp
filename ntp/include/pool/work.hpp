/**
 * @file work.hpp
 * @brief Work callback wrapper and manager implementation
 */

#pragma once

#include <tuple>
#include <utility>

#include "details/windows.hpp"
#include "details/utils.hpp"
#include "pool/basic_callback.hpp"


namespace ntp::work::details {

/**
 * @brief Work callback wrapper (PTP_WORK)
 * 
 * @tparam Functor Type of callable to invoke in threadpool
 * @tparam Args... Types of arguments
 */
template<typename Functor, typename... Args>
class alignas(NTP_ALLOCATION_ALIGNMENT) WorkCallback final
    : public ntp::details::BasicCallback<WorkCallback<Functor, Args...>, Functor, Args...>
{
    friend class ntp::details::BasicCallback<WorkCallback<Functor, Args...>, Functor, Args...>;

public:
    /**
     * @brief Constructor from callable and its arguments
     * 
     * @param functor Callable to invoke
     * @param args Arguments to pass into callable (they will be copied into wrapper)
     */
    template<typename CFunctor, typename... CArgs>
    explicit WorkCallback(CFunctor&& functor, CArgs&&... args)
        : BasicCallback(std::forward<CFunctor>(functor), std::forward<CArgs>(args)...)
    { }

private:
    /**
     * @brief Parameter conversion function. Does nothing.
     */
    void* ConvertParameter(void*) { return nullptr; }

    /**
     * @brief WorkCallback invocation function implementation. Supports invocation of 
     *        callbacks with or without PTP_CALLBACK_INSTANCE parameter.
     */
    template<typename = void> /* if constexpr works only for templates */
    void CallImpl(PTP_CALLBACK_INSTANCE instance, void* /* parameter */)
    {
        if constexpr (std::is_invocable_v<std::decay_t<Functor>, PTP_CALLBACK_INSTANCE, std::decay_t<Args>...>)
        {
            const auto args = std::tuple_cat(std::make_tuple(instance), Arguments());
            std::apply(Callable(), args);
        }
        else
        {
            std::apply(Callable(), Arguments());
        }
    }
};


/**
 * @brief WorkManager for work callbacks. Binds callbacks and threadpool implementation.
 */
class WorkManager final
    : public ntp::details::BasicManager
{
    WorkManager(const WorkManager&)            = delete;
    WorkManager& operator=(const WorkManager&) = delete;

public:
    /**
     * @brief Constructor that initializes all necessary objects.
     * 
     * @param environment Owning threadpool environment
     */
    explicit WorkManager(PTP_CALLBACK_ENVIRON environment);

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
        const auto callback = new WorkCallback<Functor, Args...>(
            std::forward<Functor>(functor), std::forward<Args>(args)...);

        queue_.Push(callback);
        ntp::details::SafeThreadpoolCall<SubmitThreadpoolWork>(work_);
    }

    /**
     * @brief Wait for all callbacks to complete
     * 
     * Wait is performed in separate thread if possible with periodical cancellation checks.
     * If waiting in separate thread is not possible, wait is performed in caller thread,
     * cancellation checks are impossible in this case (error message is reported to logger).
     * 
     * @param test_cancel Reference to cancellation test function
     * 
     * @returns true if all callbacks are completed, false if cancellation occurred while waiting for callbacks
     */
    bool WaitAll(const ntp::details::test_cancel_t& test_cancel) noexcept;

    /**
     * @brief Cancel all pending callbacks
     */
    void CancelAll() noexcept;

private:
    size_t ClearList() noexcept;

private:
    static void NTAPI InvokeCallback(PTP_CALLBACK_INSTANCE instance, PSLIST_HEADER queue, PTP_WORK work) noexcept;

    static void CALLBACK WaitAllCallback(PTP_CALLBACK_INSTANCE instance, WorkManager* self) noexcept;

private:
    // Internal queue with callbacks
    ntp::details::NativeSlist queue_;

    // Internal callback descriptor (one for every callback in the pool)
    PTP_WORK work_;

    // Event: all tasks completed
    ntp::details::Event done_event_;
};

}  // namespace ntp::work::details
