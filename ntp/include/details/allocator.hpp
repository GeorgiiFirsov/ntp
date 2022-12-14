/**
 * @file allocator.hpp
 * @brief Allocator implementations
 *
 * Allocators do not call constructors, they just allocates a
 * block of memory and returns a pointer to it.
 * 
 * This file contains an implementations 
 * of the following allocators:
 * - HeapAllocator
 * - AlignedAllocator 
 */

#pragma once

#include "details/exception.hpp"
#include "details/windows.hpp"


namespace ntp::allocator {

/**
 * @brief Allocator that uses HeapAlloc/HeapFree
 * 
 * @tparam Ty Type, which type an allocated memory pointer has
 */
template<typename Ty>
class HeapAllocator final
{
    // Actually I use allocator for void-pointers
    using basic_allocator_t = HeapAllocator<void>;

public:
    /**
     * @brief Allocates specific number of elements
     * 
     * @param count Number of elements to allocate (default = 1)
     * @returns Pointer to allocated memory
     * @throws exception::Win32Exception if count is zero or in case
     *                                   of allocation failure
     */
    static Ty* Allocate(size_t count = 1)
    {
        return AllocateBytes(count * sizeof(Ty));
    }

    /**
     * @brief Allocates specific number of bytes
     * 
     * @param bytes Number of bytes to allocate (default = sizeof(Ty))
     * @returns Pointer to allocated memory
     * @throws exception::Win32Exception if bytes is less than sizeof(Ty) or
     *                                   in case of allocation failure
     * 
     */
    static Ty* AllocateBytes(size_t bytes = sizeof(Ty))
    {
        if (bytes < sizeof(Ty))
        {
            throw exception::Win32Exception(ERROR_INVALID_PARAMETER);
        }

        return static_cast<Ty*>(basic_allocator_t::AllocateBytes(bytes));
    }

    /**
     * @brief Frees a memory, allocated with HeapAllocator
     * 
     * @param ptr Pointer to memory (may be NULL-pointer)
     */
    static void Free(Ty* ptr) noexcept
    {
        return basic_allocator_t::Free(static_cast<void*>(ptr));
    }
};


/**
 * @brief Allocator that uses HeapAlloc/HeapFree for void-pointers
 */
template<>
struct HeapAllocator<void> final
{
    /**
     * @brief Allocates specific number of bytes
     *
     * @param bytes Number of bytes to allocate
     * @returns Pointer to allocated memory
     * @throws exception::Win32Exception in case of allocation failure
     *
     */
    static void* AllocateBytes(size_t bytes)
    {
        const auto allocated = HeapAlloc(GetProcessHeap(),
            HEAP_ZERO_MEMORY, static_cast<SIZE_T>(bytes));

        if (!allocated)
        {
            throw exception::Win32Exception();
        }

        return allocated;
    }

    /**
     * @brief Frees a memory, allocated with HeapAllocator
     *
     * @param ptr Pointer to memory (may be NULL-pointer)
     */
    static void Free(void* ptr) noexcept
    {
        if (ptr)
        {
            HeapFree(GetProcessHeap(), 0, ptr);
        }
    }
};


/**
 * @brief Allocator that uses _aligned_malloc/_aligned_free
 * 
 * @tparam Ty Type, which type an allocated memory pointer has
 */
template<typename Ty>
class AlignedAllocator final
{
    // Actually I use allocator for void-pointers
    using basic_allocator_t = AlignedAllocator<void>;

public:
    /**
     * @brief Allocates specific number of elements
     * 
     * @tparam alignment Alignment value (default = NTP_ALLOCATION_ALIGNMENT)
     * @param count Number of elements to allocate (default = 1)
     * @returns Pointer to allocated memory
     * @throws exception::Win32Exception if count is zero or in case
     *                                   of allocation failure
     */
    template<size_t alignment = NTP_ALLOCATION_ALIGNMENT>
    static Ty* Allocate(size_t count = 1)
    {
        return AllocateBytes<alignment>(count * sizeof(Ty));
    }

    /**
     * @brief Allocates specific number of bytes
     *
     * @tparam alignment Alignment value (default = NTP_ALLOCATION_ALIGNMENT)
     * @param bytes Number of bytes to allocate (default = sizeof(Ty))
     * @returns Pointer to allocated memory
     * @throws exception::Win32Exception if bytes is less than sizeof(Ty) or
     *                                   in case of allocation failure
     *
     */
    template<size_t alignment = NTP_ALLOCATION_ALIGNMENT>
    static Ty* AllocateBytes(size_t bytes = sizeof(Ty))
    {
        if (bytes < sizeof(Ty))
        {
            throw exception::Win32Exception(ERROR_INVALID_PARAMETER);
        }

        return static_cast<Ty*>(basic_allocator_t::AllocateBytes<alignment>(bytes));
    }

    /**
     * @brief Frees a memory, allocated with AlignedAllocator
     *
     * @param ptr Pointer to memory (may be NULL-pointer)
     */
    static void Free(Ty* ptr) noexcept
    {
        return basic_allocator_t::Free(static_cast<void*>(ptr));
    }
};


/**
 * @brief Allocator that uses _aligned_malloc/_aligned_free for void pointers
 */
template<>
struct AlignedAllocator<void> final
{
    /**
     * @brief Allocates specific number of bytes
     *
     * @tparam alignment Alignment value (default = NTP_ALLOCATION_ALIGNMENT)
     * @param bytes Number of bytes to allocate
     * @returns Pointer to allocated memory
     * @throws exception::Win32Exception in case of allocation failure
     *
     */
    template<size_t alignment = NTP_ALLOCATION_ALIGNMENT>
    static void* AllocateBytes(size_t bytes)
    {
        const auto allocated = _aligned_malloc(bytes, alignment);
        if (!allocated)
        {
            throw exception::Win32Exception(ERROR_NOT_ENOUGH_MEMORY);
        }

        return allocated;
    }

    /**
     * @brief Frees a memory, allocated with AlignedAllocator<void>
     *
     * @param ptr Pointer to memory (may be NULL-pointer)
     */
    static void Free(void* ptr) noexcept
    {
        if (ptr)
        {
            _aligned_free(ptr);
        }
    }
};

}  // namespace ntp::allocator
