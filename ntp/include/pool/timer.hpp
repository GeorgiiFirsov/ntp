/**
 * @file timer.hpp
 * @brief Implementation of threadpool timer callback
 */

#pragma once

#include <tuple>
#include <utility>

#include "config.hpp"
#include "details/time.hpp"
#include "details/utils.hpp"
#include "pool/basic_callback.hpp"


namespace ntp::timer::details {

/**
 * @brief Specific context for threadpool timer objects.
 *        Contatins timeout and period.
 */
struct TimerContext
{
    FILETIME timer_timeout; /**< Timeout of first trigger */

    DWORD timer_period; /**< Timer period (if 0, then timer is non-periodic) */
};


/**
 * @brief Timer callback wrapper (PTP_TIMER).
 *
 * @tparam Functor Type of callable to invoke in threadpool
 * @tparam Args... Types of arguments
 */
template<typename Functor, typename... Args>
class alignas(NTP_ALLOCATION_ALIGNMENT) TimerCallback final
    : public ntp::details::BasicCallback<TimerCallback<Functor, Args...>, Functor, Args...>
{
public:
    /**
     * @brief Constructor from callable and its arguments
     *
     * @param functor Callable to invoke
     * @param args Arguments to pass into callable (they will be copied into wrapper)
     */
    template<typename CFunctor, typename... CArgs>
    explicit TimerCallback(CFunctor&& functor, CArgs&&... args)
        : BasicCallback(std::forward<CFunctor>(functor), std::forward<CArgs>(args)...)
    { }

    /**
	 * @brief Parameter conversion function. Does nothing.
	 */
    void* ConvertParameter(void*) { return nullptr; }

    /**
     * @brief Callback invocation function implementation. Supports invocation of
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
 * @brief Manager for wait callbacks. Binds callbacks and threadpool implementation.
 */
class TimerManager final
    : public ntp::details::BasicManagerEx<PTP_TIMER, TimerContext, TimerManager>
{
    friend class ntp::details::BasicManagerEx<PTP_TIMER, TimerContext, TimerManager>;

public:
    /**
	 * @brief Constructor that initializes all necessary objects.
	 *
	 * @param environment Owning threadpool environment
     */
    explicit TimerManager(PTP_CALLBACK_ENVIRON environment);

    /**
     * @brief Submits a threadpool timer object with a user-defined callback.
     * 
     * Creates a new callback wrapper, new timer object, put 
     * it into a callbacks container and then sets threadpool timer.
	 *
	 * @param timeout Timeout after which timer object calls the callback
	 * @param period If non-zero, timer object willbe triggered each period after first call
	 * @param functor Callable to invoke
	 * @param args Arguments to pass into callable (they will be copied into wrapper)
	 * @returns handle for created wait object
     */
    template<typename Rep1, typename Period1, typename Rep2, typename Period2, typename Functor, typename... Args>
    native_handle_t Submit(const std::chrono::duration<Rep1, Period1>& timeout, const std::chrono::duration<Rep2, Period2>& period,
        Functor&& functor, Args&&... args)
    {
        auto context      = CreateContext();
        context->callback = std::make_unique<TimerCallback<Functor, Args...>>(std::forward<Functor>(functor), std::forward<Args>(args)...);

        //
        // Here we need relative timeout, so I will invert it (negative timeout represets a relative time interval):
        // https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-setthreadpooltimerex
        //

        context->object_context.timer_period  = static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(period).count());
        context->object_context.timer_timeout = ntp::time::AsFileTime(timeout);
        context->object_context.timer_timeout = ntp::time::Negate(context->object_context.timer_timeout);

        const auto native_handle = CreateThreadpoolTimer(reinterpret_cast<PTP_TIMER_CALLBACK>(InvokeCallback),
            context.get(), Environment());

        SubmitContext(native_handle, std::move(context));

        return native_handle;
    }

    /**
	 * @brief Submits a non-periodic threadpool timer object with a user-defined callback.
	 *
	 * Just calls generic version of ntp::wait::details::TimerManager::Submit with
	 * 0 as period parameter.
	 *
	 * @param timeout Timeout after which timer object calls the callback
	 * @param functor Callable to invoke
	 * @param args Arguments to pass into callable (they will be copied into wrapper)
	 * @returns handle for created wait object
     */
    template<typename Rep, typename Period, typename Functor, typename... Args>
    auto Submit(const std::chrono::duration<Rep, Period>& timeout, Functor&& functor, Args&&... args)
        -> std::enable_if_t<!ntp::time::details::is_duration_v<Functor>, native_handle_t>
    {
        return Submit(timeout, std::chrono::milliseconds(0), std::forward<Functor>(functor), std::forward<Args>(args)...);
    }

    /**
     * @brief Replaces an existing threadpool timer callback with a new one.
     * 
     * Cancels current pending callback, that corresponds to the specified 
     * timer handle, creates a new callback wrapper and then sets timer again
	 * with unchanged parameters.
	 *
	 * @param timer_object Handle for an existing timer object (obtained from ntp::timer::details::TimerManager::Submit)
	 * @param functor New callable to invoke
	 * @param args New arguments to pass into callable (they will be copied into wrapper)
	 * @throws ntp::exception::Win32Exception if specified handle is not present or corrupt
	 * @returns handle for the same timer object
     */
    template<typename Functor, typename... Args>
    native_handle_t Replace(native_handle_t timer_object, Functor&& functor, Args&&... args)
    {
        if (const auto context = Lookup(timer_object); context)
        {
            return ReplaceUnsafe(timer_object, context, std::forward<Functor>(functor),
                std::forward<Args>(args)...);
        }

        throw exception::Win32Exception(ERROR_NOT_FOUND);
    }

private:
    template<typename Functor, typename... Args>
    native_handle_t ReplaceUnsafe(native_handle_t native_handle, context_pointer_t context, Functor&& functor, Args&&... args)
    {
        //
        // Firstly we need to cancel current pending callback and only
        // after that we are allowed to replace it with the new one
        //

        ntp::details::SafeThreadpoolCall<SetThreadpoolTimerEx>(native_handle, nullptr, 0, 0);
        ntp::details::SafeThreadpoolCall<WaitForThreadpoolTimerCallbacks>(native_handle, TRUE);

        context->callback = std::make_unique<TimerCallback<Functor, Args...>>(
            std::forward<Functor>(functor), std::forward<Args>(args)...);

        //
        // BUGBUG: here we actually need to handle timeout difference
        //

        SubmitInternal(native_handle, context->object_context);

        return native_handle;
    }

    void SubmitInternal(native_handle_t native_handle, object_context_t& user_context);

private:
    static void NTAPI InvokeCallback(PTP_CALLBACK_INSTANCE instance, context_pointer_t context, PTP_TIMER timer) noexcept;

    static void Close(native_handle_t native_handle) noexcept;
};

}  // namespace ntp::timer::details
