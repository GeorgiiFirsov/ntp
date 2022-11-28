/**
 * @file wait.hpp
 * @brief Implementation of threadpool wait callback
 */

#pragma once

#include <map>
#include <mutex>
#include <tuple>
#include <atomic>
#include <utility>
#include <optional>

#include "config.hpp"
#include "details/time.hpp"
#include "details/utils.hpp"
#include "details/exception.hpp"
#include "pool/basic_callback.hpp"


namespace ntp::wait::details {

/**
 * @brief Wait callback wrapper (PTP_WAIT).
 *
 * @tparam Functor Type of callable to invoke in threadpool
 * @tparam Args... Types of arguments
 */
template<typename Functor, typename... Args>
class alignas(NTP_ALLOCATION_ALIGNMENT) Callback final
    : public ntp::details::BasicCallback<Callback<Functor, Args...>, Functor, Args...>
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
     * @brief Converts void* parameter into TP_WAIT_RESULT.
     */
    TP_WAIT_RESULT ConvertParameter(void* parameter) { return reinterpret_cast<TP_WAIT_RESULT>(parameter); }

    /**
     * @brief Callback invocation function implementation. Supports invocation of
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
class Manager final
    : public ntp::details::BasicManager
{
    // Forward declaration of context structure
    struct Context;

    // Type of native threadpool object handle
    using native_handle_t = PTP_WAIT;

    // Container with callbacks represented by their handles
    using callbacks_t = std::map<native_handle_t, std::unique_ptr<Context>>;

    // Lock primitive
    using lock_t = ntp::details::RtlResource;

private:
    /**
     * @brief Meta information about context.
     */
    struct MetaContext
    {
        Manager* manager; /**< Pointer to parent wait manager */

        callbacks_t::iterator iterator; /**< Iterator to current context */
    };

    /**
     * 
     */
    struct Context final
    {
        MetaContext meta_context; /**< Meta information about context */

        std::optional<FILETIME> wait_timeout; /**< Wait timeout (pftTimeout parameter of SetThreadpoolWait function) */

        HANDLE wait_handle; /**< Handle to wait for */

        ntp::details::callback_t callback; /**< Pointer to callback wrapper */
    };

    /**
     * @brief Pseudo-lock primitive to prohibit removal from container
     */
    class RemovalPermission
    {
        RemovalPermission(const RemovalPermission&)            = delete;
        RemovalPermission& operator=(const RemovalPermission&) = delete;

    public:
        RemovalPermission()
            : can_remove_(true)
        { }

        ~RemovalPermission() = default;

        void lock() noexcept { can_remove_.store(false, std::memory_order_release); }
        void unlock() noexcept { can_remove_.store(true, std::memory_order_release); }

        operator bool() const noexcept { return can_remove_.load(std::memory_order_acquire); }

    private:
        std::atomic_bool can_remove_;
    };

public:
    /**
	 * @brief Constructor that initializes all necessary objects.
	 *
	 * @param environment Owning threadpool environment
     */
    explicit Manager(PTP_CALLBACK_ENVIRON environment);

    ~Manager();

    /**
     * @brief Submits or replaces a threadpool wait object with a user-defined callback.
     * 
     * Creates a new callback wrapper, new wait object (if not already present), put 
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
    HANDLE Submit(HANDLE wait_handle, const std::chrono::duration<Rep, Period>& timeout, Functor&& functor, Args&&... args)
    {
        auto context         = std::make_unique<Context>();
        context->callback    = std::make_unique<Callback<Functor, Args...>>(std::forward<Functor>(functor), std::forward<Args>(args)...);
        context->wait_handle = wait_handle;

        const auto native_timeout = std::chrono::duration_cast<ntp::time::native_duration_t>(timeout);
        if (native_timeout != ntp::time::max_native_duration)
        {
            //
            // Here we need relative timeout, so I will invert it (negative timeout represets a relative time interval):
            // https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-setthreadpoolwait
            //

            context->wait_timeout                = ntp::time::AsFiletime(native_timeout);
            context->wait_timeout->dwLowDateTime = static_cast<DWORD>(
                -static_cast<LONG>(context->wait_timeout->dwLowDateTime));
        }

        const auto native_handle = CreateThreadpoolWait(reinterpret_cast<PTP_WAIT_CALLBACK>(InvokeCallback),
            context.get(), Environment());

        std::unique_lock lock { lock_ };

        const auto [iter, _]                = callbacks_.emplace(native_handle, std::move(context));
        iter->second->meta_context.manager  = this;
        iter->second->meta_context.iterator = iter;

        return SubmitInternal(iter);
    }

    /**
	 * @brief Submits or replaces a threadpool wait object of default type with a 
     * user-defined callback. It never expires.
	 *
	 * Just calls generic version of ntp::wait::details::Manager::Submit with
	 * ntp::time::max_native_duration as timeout parameter and 
     * ntp::wait::details::Type::kDefault as type parameter.
	 *
	 * @param wait_handle Handle to wait for
	 * @param functor Callable to invoke
	 * @param args Arguments to pass into callable (they will be copied into wrapper)
	 * @returns handle for created wait object
     */
    template<typename Functor, typename... Args>
    auto Submit(HANDLE wait_handle, Functor&& functor, Args&&... args)
        -> std::enable_if_t<!ntp::time::details::is_duration_v<Functor>, HANDLE>
    {
        return Submit(wait_handle, ntp::time::max_native_duration, std::forward<Functor>(functor), std::forward<Args>(args)...);
    }

    /**
     * @brief Replaces an existing threadpool wait callback with a new one.
     * 
     * Cancels current pending callback, that corresponds to the specified 
     * wait handle, creates a new callback wrapper and then sets wait again
	 * with unchanged parameters.
	 *
	 * @param wait_object Handle for an existing wait object (obtained from Submit)
	 * @param functor New callable to invoke
	 * @param args New arguments to pass into callable (they will be copied into wrapper)
	 * @throws ntp::exception::Win32Exception if wait handle is not present or corrupt
	 * @returns handle for the same wait object
     */
    template<typename Functor, typename... Args>
    HANDLE Replace(HANDLE wait_object, Functor&& functor, Args&&... args)
    {
        std::unique_lock lock { lock_ };

        const auto native_handle = static_cast<native_handle_t>(wait_object);
        if (auto callback = callbacks_.find(native_handle); callback != callbacks_.end())
        {
            return ReplaceUnsafe(callback, std::forward<Functor>(functor), std::forward<Args>(args)...);
        }

        throw exception::Win32Exception(ERROR_NOT_FOUND);
    }

    /**
     * @brief Cancels and removes wait object, that corresponds to a specified wait handle.
     * 
     * If no wait for the handle is present, does nothing.
     * 
     * @param wait_object Handle for an existing wait object (obtained from Submit)
     */
    void Cancel(HANDLE wait_object) noexcept;

    /**
     * @brief Cancel all pending callbacks.
     */
    void CancelAll() noexcept;

private:
    template<typename Functor, typename... Args>
    HANDLE ReplaceUnsafe(callbacks_t::iterator callback, Functor&& functor, Args&&... args)
    {
        auto& [native_handle, context] = *callback;

        //
        // Firstly we need to cancel current pending callback and only
        // after that we are allowed to replace it with the new one
        //

        SetThreadpoolWait(native_handle, nullptr, nullptr);

        context->callback.reset(new Callback<Functor, Args...>(
            std::forward<Functor>(functor), std::forward<Args>(args)...));

        SubmitInternal(callback);

        return native_handle;
    }

    void Remove(callbacks_t::iterator callback) noexcept;

    HANDLE SubmitInternal(callbacks_t::iterator callback);

private:
    static void NTAPI InvokeCallback(PTP_CALLBACK_INSTANCE instance, Context* context, PTP_WAIT wait, TP_WAIT_RESULT wait_result) noexcept;

    static void CloseWait(PTP_WAIT wait) noexcept;

private:
    // Container with callbacks
    callbacks_t callbacks_;

    // Syncronization primitive for callbacks container
    mutable lock_t lock_;

    // If true, callback will not release its resource after completion.
    // This flag is used in CancelAll function to prevent container
    // modification while iterating over it.
    mutable RemovalPermission removal_permission_;
};

}  // namespace ntp::wait::details
