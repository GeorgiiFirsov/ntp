#include "config.hpp"


namespace test::details {

class TempFileName final
{
    static constexpr auto kPrefix = L"~ntp";

public:
    explicit TempFileName(bool delete_file = true)
        : file_name_(MAX_PATH, 0)
        , delete_ { delete_file }
    {
        std::array<wchar_t, MAX_PATH> temp_path;
        GetTempPath(static_cast<DWORD>(temp_path.size()), temp_path.data());

        GetTempFileName(temp_path.data(), kPrefix, 0, file_name_.data());
        DeleteFile(file_name_.c_str());
    }

    ~TempFileName()
    {
        if (delete_)
        {
            DeleteFile(file_name_.c_str());
        }
    }

    operator const wchar_t*() const noexcept { return file_name_.c_str(); }
    operator const std::wstring&() const noexcept { return file_name_; }

private:
    std::wstring file_name_;
    bool delete_;
};

}  // namespace test::details
