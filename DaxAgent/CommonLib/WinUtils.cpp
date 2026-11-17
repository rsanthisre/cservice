#include "stdafx.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <Shlwapi.h>
#include "WinUtils.h"
#include "Logger.h"
#include <fstream>
#include <vector>
#include <algorithm>


using namespace std;
namespace ADAX
{
    void WinUtils::SAFE_CLOSE_HANDLE(HANDLE& hnd)
    {
        if (hnd != NULL) {
            CloseHandle(hnd);
            hnd = NULL;
        }
    }

    void WinUtils::ToLower(std::wstring& inStr)
    {
        std::transform(inStr.begin(), inStr.end(), inStr.begin(), ::tolower);
    }

    std::string WinUtils::WStringToUtf8(const std::wstring& wide)
    {
        if (wide.empty())
        {
            return {};
        }
        const int len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()),
            nullptr, 0, nullptr, nullptr);
        if (len <= 0)
        {
            Logger::Error("WinUtils::WStringToUtf8: WideCharToMultiByte (size query) failed, GetLastError=" +
                std::to_string(GetLastError()));
            return {};
        }
        std::string out(len, '\0');
        if (WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()),
            &out[0], len, nullptr, nullptr) <= 0)
        {
            Logger::Error("WinUtils::WStringToUtf8: WideCharToMultiByte (convert) failed, GetLastError=" +
                std::to_string(GetLastError()));
            return {};
        }
        return out;
    }

   

}