#pragma once

#include <Windows.h>

#include "exception.hpp"


namespace ntp::allocator {

template<typename Ty>
struct HeapAllocator final
{
    static Ty* Allocate(size_t count = 1)
    {
        return AllocateBytes(count * sizeof(Ty));
    }

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

    static void Free(Ty* ptr)
    {
        if (ptr)
        {
            HeapFree(GetProcessHeap(), 0, ptr);
        }
    }
};

}  // namespace ntp::allocator
