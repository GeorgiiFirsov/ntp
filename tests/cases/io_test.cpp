#include "test_config.hpp"
#include "utils.hpp"


TEST(Io, Submit)
{
    //
    // Create temporary file to write in
    //

    test::details::TempFileName file_name;
    ATL::CAtlFile file;

    HRESULT hr = file.Create(file_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
        CREATE_ALWAYS, FILE_FLAG_OVERLAPPED);

    EXPECT_TRUE(SUCCEEDED(hr));

    //
    // Now submit an IO callback
    //

    ATL::CEvent event(TRUE, FALSE);
    size_t bytes_written = 0;

    ntp::SystemThreadPool pool;
    pool.SubmitIo(file, [&bytes_written, &event](PTP_CALLBACK_INSTANCE instance, LPVOID /*overlapped*/, ULONG /*result*/, ULONG_PTR bytes_transferred) {
        SetEventWhenCallbackReturns(instance, event);
        bytes_written = static_cast<size_t>(bytes_transferred);
    });

    //
    // And start to write a big amount of data into a file asyncronously
    //

    std::vector<unsigned char> buffer(static_cast<size_t>(-1) / 10, 0);

    OVERLAPPED ovl = {};
    hr             = file.Write(buffer.data(), static_cast<DWORD>(buffer.size()), &ovl);

    EXPECT_EQ(hr, HRESULT_FROM_WIN32(ERROR_IO_PENDING));

    WaitForSingleObject(event, INFINITE);

    //
    // Check if all bytes are written into a file
    //

    EXPECT_EQ(bytes_written, buffer.size());
}
