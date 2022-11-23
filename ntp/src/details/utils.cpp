#include "utils.hpp"
#include "config.hpp"
#include "exception.hpp"


namespace ntp::details {

NativeSlist::NativeSlist()
    : header_(allocator_t::Allocate<NTP_ALLOCATION_ALIGNMENT>())
{
    if (!header_)
    {
        throw exception::Win32Exception(ERROR_NOT_ENOUGH_MEMORY);
    }

    InitializeSListHead(header_);
}

NativeSlist::~NativeSlist()
{
    if (header_)
    {
        allocator_t::Free(header_);
    }
}

void NativeSlist::Push(PSLIST_ENTRY entry) noexcept
{
    InterlockedPushEntrySList(header_, entry);
}

}  // namespace ntp::details
