#include <stdio.h>
#include "stdafx.h"
#include "SvcCommon.h"
#include "Logger.h"
#include "Install.h"
#include "CLIParser.h"
#include "WinUtils.h"
#include <stdexcept>
#pragma comment(lib, "advapi32.lib")

using namespace ADAX;

int wmain(int argc, wchar_t* argv[])
{
    SvcInstaller mInstaller;
    CLIParser cParser(argc, argv);



    const BOOL created = CreateDirectoryW(LOG_DIR_PATH.c_str(), NULL);
    const DWORD err = created ? ERROR_SUCCESS : GetLastError();

    if (!created && err != ERROR_ALREADY_EXISTS)
    {
        wprintf(L"Failed to create log directory: %s (GetLastError=%lu)\n",
            LOG_DIR_PATH.c_str(), GetLastError());
        return 1;
    }

    try
    {
        Logger::Init(
            LOG_FILE_PATH,
            LogLevel::DEBUG_LEVEL,
            10 * 1024 * 1024,       // 10 MB
            24 * 60 * 60            // 24h
        );
    }
    catch (const std::exception& ex)
    {
        wprintf(L"Failed to initialize logger: %hs\n", ex.what());
        return 1;
    }



    //while (!IsDebuggerPresent()) {
    //    Sleep(100);
    //}
    //DebugBreak();

    if (cParser.HasSwitch(L"-install")) {
        mInstaller.InstallService();
    }
    else if (cParser.HasSwitch(L"-remove")) {
        mInstaller.UninstallService();
    }
    else if (cParser.HasSwitch(L"-help")) {
        //Help
        wprintf(L"Parameters:\n");
        wprintf(L" -install  to install the service.\n");
        wprintf(L" -remove   to remove the service.\n");
        wprintf(L" -help     to print CMD Arguments Help.\n");

    }
    else {
        mInstaller.RunService();
    }

    return 0;
}

