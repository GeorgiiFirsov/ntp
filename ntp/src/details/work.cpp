#include "work.hpp"
#include "exception.hpp"


namespace ntp::work::details {

Manager::Manager(PTP_CALLBACK_ENVIRON environment)
    : BasicManager(environment)
    , queue_()
    , work_()
    , done_event_(TRUE, FALSE)
{
    work_ = CreateThreadpoolWork(reinterpret_cast<PTP_WORK_CALLBACK>(InvokeCallback),
        static_cast<PSLIST_HEADER>(queue_), Environment());

    if (!work_)
    {
        throw exception::Win32Exception();
    }
}

Manager::~Manager()
{
    if (work_)
    {
        ntp::details::SafeThreadpoolCall<WaitForThreadpoolWorkCallbacks>(work_, TRUE);
        ntp::details::SafeThreadpoolCall<CloseThreadpoolWork>(std::exchange(work_, nullptr));
    }
}

/* static */
void NTAPI Manager::InvokeCallback(PTP_CALLBACK_INSTANCE instance, PSLIST_HEADER queue, PTP_WORK work)
{
    try
    {
        if (!queue)
        {
            throw exception::Win32Exception(ERROR_INVALID_PARAMETER);
        }

        auto entry = InterlockedPopEntrySList(queue);
        if (!entry)
        {
            throw exception::Win32Exception(ERROR_NO_MORE_ITEMS);
        }

        std::unique_ptr<ntp::details::ICallback> callback(
            static_cast<ntp::details::ICallback*>(entry));

        callback->Call(nullptr);
    }
    catch (const std::exception& error)
    {
        // TODO
    }
    catch (...)
	{
		// TODO
    }
}

}  // namespace ntp::work::details
