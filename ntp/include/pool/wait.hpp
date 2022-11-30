/**
 * @file wait.hpp
 * @brief Implementation of threadpool wait callback
 */

#pragma once

#include <tuple>
#include <utility>
#include <optional>

#include "config.hpp"
#include "details/time.hpp"
#include "details/utils.hpp"
#include "pool/basic_callback.hpp"


namespace ntp::wait::details {

/**
 * @brief Specific context for threadpool wait objects. 
 *        Contatins wait handle and wait timeout.
 */
struct WaitContext
{
    std::optional<FILETIME> wait_timeout; /**< Wait timeout (pftTimeout parameter of SetThreadpoolWait function) */

    HANDLE wait_handle; /**< Handle to wait for */
};


/**
 * @brief Wait callback wrapper (PTP_WAIT).
 *
 * @tparam Functor Type of callable to invoke in threadpool
 * @tparam Args... Types of arguments
 */
template<typename Functor, typename... Args>
class alignas(NTP_ALLOCATION_ALIGNMENT) WaitCallback final
    : public ntp::details::BasicCallback<WaitCallback<Functor, Args...>, Functor, Args...>
{
    friend class ntp::details::BasicCallback<WaitCallback<Functor, Args...>, Functor, Args...>;

public:
    /**
     * @brief Constructor from callable and its arguments
     *
     * @param functor Callable to invoke
     * @param args Arguments to pass into callable (they will be copied into wrapper)
     */
    template<typename CFunctor, typename... CArgs>
    explicit WaitCallback(CFunctor&& functor, CArgs&&... args)
        : BasicCallback(std::forward<CFunctor>(functor), std::forward<CArgs>(args)...)
    { }

private:
    /**
     * @brief Converts void* parameter into TP_WAIT_RESULT.
     */
    TP_WAIT_RESULT ConvertParameter(void* parameter) { return reinterpret_cast<TP_WAIT_RESULT>(parameter); }

    /**
     * @brief WaitCallback invocation function implementation. Supports invocation of
     *        callbacks with or without PTP_CALLBACK_INSTANCE parameter.
     */
    template<typename = void> /* if constexpr works only for templates */
    void CallImpl(PTP_CALLBACK_INSTANCE instance, TP_WAIT_RESULT wait_result)
    {
        if constexpr (std::is_invocable_v<std::decay_t<Functor>, PTP_CALLBACK_INSTANCE, TP_WAIT_RESULT, std::decay_t<Args>...>)
        {
            const auto args = std::tuple_cat(std::make_tuple(instance, wait_result), Arguments());
            std::apply(Callable(), args);
        }
        else
        {
            const auto args = std::tuple_cat(std::make_tuple(wait_result), Arguments());
            std::apply(Callable(), args);
        }
    }
};


/**
 * @brief Manager for wait callbacks. Binds callbacks and threadpool implementation.
 */
class WaitManager final
    : public ntp::details::BasicManagerEx<PTP_WAIT, WaitContext, WaitManager>
{
    friend class ntp::details::BasicManagerEx<PTP_WAIT, WaitContext, WaitManager>;

public:
    /**
	 * @brief Constructor that initializes all necessary objects.
	 *
	 * @param environment Owning threadpool environment
     */
    explicit WaitManager(PTP_CALLBACK_ENVIRON environment);

    /**
     * @brief Submits a threadpool wait object with a user-defined callback.
     * 
     * Creates a new callback wrapper, new wait object, put 
     * it into a callbacks container and then sets threadpool wait.
     * 
     * @param wait_handle Handle to wait for
     * @param timeout Timeout while wait object waits for the specified handle 
     *                (pass ntp::time::max_native_duration for infinite wait timeout)
	 * @param functor Callable to invoke
	 * @param args Arguments to pass into callable (they will be copied into wrapper)
	 * @returns handle for created wait object
     */
    template<typename Rep, typename Period, typename Functor, typename... Args>
    native_handle_t Submit(HANDLE wait_handle, const std::chrono::duration<Rep, Period>& timeout, Functor&& functor, Args&&... args)
    {
        auto context                        = CreateContext();
        context->callback                   = std::make_unique<WaitCallback<Functor, Args...>>(std::forward<Functor>(functor), std::forward<Args>(args)...);
        context->object_context.wait_handle = wait_handle;

        if (const auto native_timeout = std::chrono::duration_cast<ntp::time::native_duration_t>(timeout);
            native_timeout != ntp::time::max_native_duration)
        {
            //
            // Here we need relative timeout, so I will invert it (negative timeout represets a relative time interval):
            // https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-setthreadpoolwait
            //

            context->object_context.wait_timeout = ntp::time::AsFileTime(native_timeout);
            context->object_context.wait_timeout = ntp::time::Negate(context->object_context.wait_timeout.value());
        }

        const auto native_handle = CreateThreadpoolWait(reinterpret_cast<PTP_WAIT_CALLBACK>(InvokeCallback),
            context.get(), Environment());

        SubmitContext(native_handle, std::move(context));

        return native_handle;
    }

    /**
	 * @brief Submits a threadpool wait object with a 
     * user-defined callback. It never expires.
	 *
	 * Just calls generic version of ntp::wait::details::WaitManager::Submit with
	 * ntp::time::max_native_duration as timeout parameter.
	 *
	 * @param wait_handle Handle to wait for
	 * @param functor Callable to invoke
	 * @param args Arguments to pass into callable (they will be copied into wrapper)
	 * @returns handle for created wait object
     */
    template<typename Functor, typename... Args>
    auto Submit(HANDLE wait_handle, Functor&& functor, Args&&... args)
        -> std::enable_if_t<!ntp::time::details::is_duration_v<Functor>, native_handle_t>
    {
        return Submit(wait_handle, ntp::time::max_native_duration, std::forward<Functor>(functor), std::forward<Args>(args)...);
    }

    /**
     * @brief Replaces an existing threadpool wait callback with a new one.
     * 
     * Cancels current pending callback, creates a new callback wrapper and then 
     * sets wait again with unchanged parameters.
	 *
	 * @param wait_object Handle for an existing wait object (obtained from ntp::wait::details::WaitManager::Submit)
	 * @param functor New callable to invoke
	 * @param args New arguments to pass into callable (they will be copied into wrapper)
	 * @throws ntp::exception::Win32Exception if wait handle is not present or corrupt
	 * @returns handle for the same wait object
     */
    template<typename Functor, typename... Args>
    native_handle_t Replace(native_handle_t wait_object, Functor&& functor, Args&&... args)
    {
        if (const auto context = Lookup(wait_object); context)
        {
            return ReplaceUnsafe(wait_object, context, std::forward<Functor>(functor),
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

        ntp::details::SafeThreadpoolCall<SetThreadpoolWait>(native_handle, nullptr, nullptr);
        ntp::details::SafeThreadpoolCall<WaitForThreadpoolWaitCallbacks>(native_handle, TRUE);

        context->callback = std::make_unique<WaitCallback<Functor, Args...>>(
            std::forward<Functor>(functor), std::forward<Args>(args)...);

        SubmitInternal(native_handle, context->object_context);

        return native_handle;
    }

    void SubmitInternal(native_handle_t native_handle, object_context_t& user_context);

private:
    static void NTAPI InvokeCallback(PTP_CALLBACK_INSTANCE instance, context_pointer_t context, PTP_WAIT wait, TP_WAIT_RESULT wait_result) noexcept;

    static void Close(native_handle_t native_handle) noexcept;
};

}  // namespace ntp::wait::details
