/**
 * @file wait.hpp
 * @brief Implementation of threadpool wait callback
 */

#pragma once

#include <tuple>
#include <utility>
#include <optional>

#include "ntp_config.hpp"
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
    TP_WAIT_RESULT* ConvertParameter(void* parameter) { return static_cast<TP_WAIT_RESULT*>(parameter); }

    /**
     * @brief WaitCallback invocation function implementation. Supports invocation of
     *        callbacks with or without PTP_CALLBACK_INSTANCE parameter.
     */
    template<typename = void> /* if constexpr works only for templates */
    void CallImpl(PTP_CALLBACK_INSTANCE instance, TP_WAIT_RESULT* wait_result)
    {
        if constexpr (std::is_invocable_v<std::decay_t<Functor>, PTP_CALLBACK_INSTANCE, TP_WAIT_RESULT, std::decay_t<Args>...>)
        {
            const auto args = std::tuple_cat(std::make_tuple(instance, *wait_result), Arguments());
            std::apply(Callable(), args);
        }
        else
        {
            const auto args = std::tuple_cat(std::make_tuple(*wait_result), Arguments());
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

        if (!native_handle)
        {
            throw exception::Win32Exception();
        }

        static_assert(noexcept(SubmitContext(native_handle, std::move(context))),
            "[ntp::wait::details::WaitManager::Submit]: inspect ntp::details::BasicCallback::SubmitContext and "
            "ntp::wait::details::WaitManager::SubmitInternal for noexcept property, because an exception thrown "
            "here can lead to handle and memory leaks. SubmitContext is noexcept if and only if SubmitInternal "
            "is noexcept.");

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

private:
    void SubmitInternal(native_handle_t native_handle, object_context_t& user_context) noexcept;

private:
    static void NTAPI InvokeCallback(PTP_CALLBACK_INSTANCE instance, context_pointer_t context, PTP_WAIT wait, TP_WAIT_RESULT wait_result) noexcept;

    static void CloseInternal(native_handle_t native_handle) noexcept;
};

}  // namespace ntp::wait::details
