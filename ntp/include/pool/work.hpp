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
#include "utils.hpp"
#include "allocator.hpp"
#include "basic_callback.hpp"


namespace ntp::work::details {

/**
 * @brief Work callback wrapper (PTP_WORK)
 * 
 * @tparam Functor Type of callable to invoke in threadpool
 * @tparam Args... Types of arguments
 */
template<typename Functor, typename... Args>
class alignas(NTP_ALLOCATION_ALIGNMENT) Callback final
    : public ntp::details::ICallback
{
    Callback(const Callback&)            = delete;
    Callback& operator=(const Callback&) = delete;

private:
    // Type of packed arguments
    using tuple_t = std::tuple<std::decay_t<Args>...>;

public:
    /**
     * @brief Constructor from callable and its arguments
     * 
     * @param functor Callable to invoke
     * @param args Arguments to pass into callable (they will be copied into wrapper)
     */
    explicit Callback(Functor functor, Args&&... args)
        : args_(std::forward<Args>(args)...)
        , functor_(std::move(functor))
    { }

    /**
     * @brief Invokation of internal callback (interface's parameter ignored)
     */
    void Call(void* /*parameter*/) override
    {
        std::apply(functor_, args_);
    }

private:
    // Packed arguments
    tuple_t args_;

    // Callable
    Functor functor_;
};


/**
 * @brief Manager for work callbacks. Binds callbacks and threadpool implementation.
 */
class Manager final
    : public ntp::details::BasicManager
{
public:
    /**
     * @brief Constructor that initializes all necessary objects.
     * 
     * @param environment Owning threadpool traits
     */
    explicit Manager(PTP_CALLBACK_ENVIRON environment);

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

private:
    static void NTAPI InvokeCallback(PTP_CALLBACK_INSTANCE instance, PSLIST_HEADER queue, PTP_WORK work);

private:
    // Internal queue with callbacks
    ntp::details::NativeSlist queue_;

    // Internal callback descriptor (one for every callback in the pool)
    PTP_WORK work_;

    // Event: all tasks completed
    ATL::CEvent done_event_;
};

}  // namespace ntp::work::details