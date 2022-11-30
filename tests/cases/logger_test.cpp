#include "test_config.hpp"


void TraceCallback(ntp::logger::Severity severity, const wchar_t* message)
{
    if (severity >= ntp::logger::Severity::kNormal)
    {
        //
        // Ignore kExtended trace
        //

        std::wcerr << message << L'\n';
    }
}


TEST(Logger, Set)
{
    EXPECT_NO_THROW({
        ntp::logger::SetLogger(TraceCallback);
    });
}
