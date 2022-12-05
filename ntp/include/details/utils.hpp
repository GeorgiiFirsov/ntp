/**
 * @file utils.hpp
 * @brief Some useful implementation stuff
 */

#pragma once

#include "ntp_config.hpp"
#include "native/ntrtl.h"
#include "details/allocator.hpp"


namespace ntp::details {

/**
 * @brief Provides SEH-safe way to call functions from Win32 ThreadPool API.
 * 
 * @tparam function Pointer to function to call
 * @tparam Args... Types of arguments
 * @param args Arbitrary number of arguments passed to the function
 * @returns Win32 error code
 */
template<auto function, typename... Args>
DWORD SafeThreadpoolCall(Args&&... args) noexcept
{
    __try
    {
        function(std::forward<Args>(args)...);
        return ERROR_SUCCESS;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }
}


/**
 * @brief Wrapper for Win32 API FormatMessage.
 *
 * @param flags formatting flags (refer to Win32 API for possible values)
 * @param source optional format string (if FORMAT_MESSAGE_FROM_SYSTEM is specified this parameter MUST be NULL)
 * @param message_id optional message identifier (ignored if FORMAT_MESSAGE_FROM_STRING is specified)
 * @returns formatted message of empty string in case of error
 */
std::string FormatMessage(DWORD flags, LPCSTR source, DWORD message_id, ...) noexcept;


/**
 * @brief Wrapper for Win32 API FormatMessage.
 *
 * @param flags formatting flags (refer to Win32 API for possible values)
 * @param source optional format string (if FORMAT_MESSAGE_FROM_SYSTEM is specified this parameter MUST be NULL)
 * @param message_id optional message identifier (ignored if FORMAT_MESSAGE_FROM_STRING is specified)
 * @returns formatted message of empty string in case of error
 */
std::wstring FormatMessage(DWORD flags, LPCWSTR source, DWORD message_id, ...) noexcept;


/**
 * @brief converts std::string to std::wstring using MultiByteToWideChar.
 * 
 * Uses Windows-1251 codepage.
 * 
 * @param source Source string
 * @returns Converted string
 */
std::wstring Convert(const std::string& source) noexcept;


/**
 * @brief Wrapper for SLIST_HEADER.
 * 
 * Native lock-free (atomic) single-linked list.
 */
class NativeSlist final
{
    // Allocator for header
    using allocator_t = allocator::AlignedAllocator<SLIST_HEADER>;

private:
    NativeSlist(const NativeSlist&)            = delete;
    NativeSlist& operator=(const NativeSlist&) = delete;

public:
    explicit NativeSlist();
    ~NativeSlist();

    /**
	 * @brief Inserts a new item to the list.
     * 
     * @param entry New item to insert into the list
	 */
    void Push(PSLIST_ENTRY entry) noexcept;

    /**
	 * @brief Implicit cast operator to internal list header.
     * 
     * @returns Pointer to the internal list headers
	 */
    operator PSLIST_HEADER() const noexcept { return header_; }

private:
    // Internal list header
    PSLIST_HEADER header_;
};


/**
 * @brief Wrapper for SLIST_ENTRY (actually inherits from it).
 */
class alignas(NTP_ALLOCATION_ALIGNMENT) NativeSlistEntry
    : public SLIST_ENTRY
{
    // Use raw aligned allocator
    using allocator_t = allocator::AlignedAllocator<void>;

public:
    /**
	 * @brief Custom new operator, that allocates aligned memory.
	 *
	 * @param bytes Number of bytes to allocate
     */
    void* operator new(size_t bytes)
    {
        return allocator_t::AllocateBytes<NTP_ALLOCATION_ALIGNMENT>(bytes);
    }

    /**
     * @brief Matching custom delete operator.
     * 
     * @param ptr Pointer to free
     */
    void operator delete(void* ptr) noexcept
    {
        return allocator_t::Free(ptr);
    }
};


/**
 * @brief STL-compatible (in terms of SharedLockable and Lockable 
 * named requirements) wrapper for RTL_RESOURCE.
 */
class RtlResource final
{
    RtlResource(const RtlResource&)            = delete;
    RtlResource& operator=(const RtlResource&) = delete;

public:
    explicit RtlResource();
    ~RtlResource();

    /**
     * @brief Blocks until a lock can be acquired for the current execution agent (thread, process, task).
     */
    void lock();

    /**
     * @brief Attempts to acquire the lock for the current execution agent (thread, process, task) without blocking.
     */
    bool try_lock();

    /**
     * @brief Releases the non-shared lock held by the execution agent.
     */
    void unlock() noexcept;

    /**
     * @brief Blocks until a lock can be obtained for the current execution agent (thread, process, task).
     */
    void lock_shared();

    /**
     * @brief Attempts to obtain a lock for the current execution agent (thread, process, task) without blocking.
     */
    bool try_lock_shared();

    /**
     * @brief Releases the shared lock held by the execution agent.
     */
    void unlock_shared() noexcept;

private:
    RTL_RESOURCE resource_;
};


/**
 * @brief Wrapper for Win32 API Event handle. Automatically closes HANDLE in destructor.
 */
class Event final
{
    Event(const Event&)            = delete;
    Event& operator=(const Event&) = delete;

public:
    /**
     * @brief The most generic constructor, accepts all of the arguments, that CreateEvent accepts.
     *
     * @param security_attributes A pointer to a SECURITY_ATTRIBUTES structure. If this parameter is NULL,
     *                            the handle cannot be inherited by child processes.
     * @param manual_reset        If this parameter is TRUE, the function creates a manual-reset event object,
     *                            which requires the use of the ResetEvent function to set the event state to
     *                            nonsignaled. If this parameter is FALSE, the function creates an auto-reset
     *                            event object, and system automatically resets the event state to nonsignaled
     *                            after a single waiting thread has been released.
     * @param initially_signaled  If this parameter is TRUE, the initial state of the event object is signaled;
     *                            otherwise, it is nonsignaled.
     * @param name                The name of the event object. The name is limited to MAX_PATH characters.
     *                            Name comparison is case sensitive.
     */
    explicit Event(LPSECURITY_ATTRIBUTES security_attributes, BOOL manual_reset, BOOL initially_signaled, LPCWSTR name = nullptr);

    /**
     * @brief Constructur, that accepts all CreateEvent arguments, but without the very first one. Sets it to nullptr.
     *
     * @param manual_reset       If this parameter is TRUE, the function creates a manual-reset event object,
     *                           which requires the use of the ResetEvent function to set the event state to
     *                           nonsignaled. If this parameter is FALSE, the function creates an auto-reset
     *                           event object, and system automatically resets the event state to nonsignaled
     *                           after a single waiting thread has been released.
     * @param initially_signaled If this parameter is TRUE, the initial state of the event object is signaled;
     *                           otherwise, it is nonsignaled.
     * @param name               The name of the event object. The name is limited to MAX_PATH characters.
     *                           Name comparison is case sensitive.
     */
    explicit Event(BOOL manual_reset, BOOL initially_signaled, LPCWSTR name = nullptr);

    /**
     * @brief Destructor closes opened event handle.
     */
    ~Event();

    /**
     * @brief Implicit cats to HANDLE (useful for more readable code)
     */
    operator HANDLE() const noexcept { return event_; }

    /**
     * @brief Wrapper for SetEvent.
     *
     * @throws ntp::exception::Win32Exception in case of failure.
     */
    void Set();

    /**
     * @brief Wrapper for ResetEvent()
     *
     * @throws ntp::exception::Win32Exception in case of failure.
     */
    void Reset();

private:
    // Internal event handle
    HANDLE event_;
};

}  // namespace ntp::details
