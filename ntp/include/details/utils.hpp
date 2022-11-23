/**
 * @file utils.hpp
 * @brief Some useful implementation stuff
 */

#pragma once

#include "config.hpp"
#include "allocator.hpp"


namespace ntp::details {

/**
 * @brief Provides SEH-safe way to call functions from Win32 ThreadPool API
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
 * @brief Wrapper for SLIST_HEADER
 * 
 * Native lock-free (atomic) single-linked list.
 */
class NativeSlist final
{
    // Allocator for header
    using allocator_t = allocator::AlignedAllocator<SLIST_HEADER>;

public:
    explicit NativeSlist();
    ~NativeSlist();

    /**
	 * @brief Inserts a new item to the list
     * 
     * @param entry New item to insert into the list
	 */
    void Push(PSLIST_ENTRY entry) noexcept;

    /**
	 * @brief Implicit cast operator to internal list header
     * 
     * @returns Pointer to the internal list headers
	 */
    operator PSLIST_HEADER() const noexcept { return header_; }

private:
    // Internal list header
    PSLIST_HEADER header_;
};


/**
 * @brief Wrapper for SLIST_ENTRY (actually inherits from it)
 */
class alignas(NTP_ALLOCATION_ALIGNMENT) NativeSlistEntry
    : public SLIST_ENTRY
{
    // Use raw aligned allocator
    using allocator_t = allocator::AlignedAllocator<void>;

public:
    /**
	 * @brief Custom new operator, that allocates aligned memory
	 *
	 * @param bytes Number of bytes to allocate
     */
    void* operator new(size_t bytes)
    {
        return allocator_t::AllocateBytes<NTP_ALLOCATION_ALIGNMENT>(bytes);
    }

    /**
     * @brief Matching custom delete operator
     * 
     * @param ptr Pointer to free
     */
    void operator delete(void* ptr) noexcept
    {
        return allocator_t::Free(ptr);
    }
};

}  // namespace ntp::details
