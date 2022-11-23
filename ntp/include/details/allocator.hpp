/**
 * @file allocator.hpp
 * @brief Allocator implementations
 * 
 * This file contains an implementations 
 * of the following allocators:
 * - HeapAllocator
 * 
 */

#pragma once

#include <Windows.h>

#include "exception.hpp"


namespace ntp::allocator {

/**
 * @brief Allocator that uses HeapAlloc/HeapFree
 * 
 * Allocator does not call constructors, it just allocates a 
 * block of zero-initialized memory and returns a pointer to it.
 * 
 * @tparam Ty Type, which type an allocated memory pointer has
 */
template<typename Ty>
struct HeapAllocator final
{
    /**
     * @brief Allocates specific number of elements
     * 
	 * @arg count Number of elements to allocate (default = 1)
	 *
	 * @returns Pointer to allocated memory
	 *
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
     * @arg bytes Number of bytes to allocate (default = sizeof(Ty))
     * 
     * @returns Pointer to allocated memory
     * 
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

        const auto allocated = HeapAlloc(GetProcessHeap(),
            HEAP_ZERO_MEMORY, static_cast<SIZE_T>(bytes));

        if (!allocated)
        {
            throw exception::Win32Exception();
        }

        return static_cast<Ty*>(allocated);
    }

    /**
     * @brief Frees a memory, allocated with this allocator
     * 
     * @arg ptr Pointer to memory (may be NULL-pointer)
     */
    static void Free(Ty* ptr)
    {
        if (ptr)
        {
            HeapFree(GetProcessHeap(), 0, ptr);
        }
    }
};

}  // namespace ntp::allocator
