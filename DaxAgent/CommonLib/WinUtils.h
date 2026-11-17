#pragma once
#include <string_view>
#include <cstdint>

namespace ADAX
{
    enum FILE_PATH_PARSE_OUTPUT_TYPE {
        OUTPUT_DIR_PATH = 1,
        OUTPUT_FILE_NAME,
        OUTPUT_EXTENSION,
        OUTPUT_PROCESSNAME
    };
    class WinUtils
    {
    public:
        static void SAFE_CLOSE_HANDLE(HANDLE& hnd);

        static std::string WStringToUtf8(const std::wstring& wide);

        static void ToLower(std::wstring& inStr);

     

    };
}
