#include"stdafx.h"
#include "Logger.h"
#include "Install.h"
#include "SvcCommon.h"
#include "WinUtils.h"
#include "DAX_Service.h"
#include <stdexcept>

using namespace ADAX;

void SvcInstaller::InstallService()
{
    InstallService(
        SERVICE_NAME,               // Name of service
        SERVICE_DISPLAY_NAME,       // Name to display
        SERVICE_START_TYPE,         // Service start type
        SERVICE_DEPENDENCIES,       // Dependencies
        SERVICE_ACCOUNT,            // Service running account
        SERVICE_PASSWORD            // Password of the account
    );
}
void SvcInstaller::UninstallService()
{
    // Uninstall the service when the command is 
           // "-remove" or "/remove".
    UninstallService((PWSTR)SERVICE_NAME.c_str());
}

// Install the service using CreateServiceW
void SvcInstaller::InstallService(std::wstring wsServiceName,
    std::wstring wsDisplayName,
    DWORD dwStartType,
    std::wstring wsDependencies,
    std::wstring wsAccount,
    std::wstring wsPassword)
{
    wchar_t szPath[MAX_PATH];
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;

    if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)) == 0)
    {
        const DWORD err = GetLastError();
        wprintf(L"GetModuleFileName failed w/err 0x%08lx\n", err);
        Logger::Error("InstallService: GetModuleFileName failed, GetLastError=" + std::to_string(err));
        goto Cleanup;
    }

    // Open the local default service control manager database
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT |
        SC_MANAGER_CREATE_SERVICE);
    if (schSCManager == NULL)
    {
        const DWORD err = GetLastError();
        wprintf(L"OpenSCManager failed w/err 0x%08lx\n", err);
        Logger::Error("InstallService: OpenSCManager failed, GetLastError=" + std::to_string(err));
        goto Cleanup;
    }

   
    schService = CreateService(
        schSCManager,                   // SCManager database
        wsServiceName.c_str(),                 // Name of service
        wsDisplayName.c_str(),                 // Name to display
        SERVICE_QUERY_STATUS,           // Desired access
        SERVICE_WIN32_OWN_PROCESS,      // Service type
        dwStartType,                    // Service start type
        SERVICE_ERROR_NORMAL,           // Error control type
        szPath,                         // Service's binary
        NULL,                           // No load ordering group
        NULL,                           // No tag identifier
        SERVICE_DEPENDENCIES,                // Dependencies
        wsAccount.c_str(),                     // Service running account
        wsPassword.empty() ? NULL : wsPassword.c_str() // Password of the account
    );
    if (schService == NULL)
    {
        const DWORD err = GetLastError();
        wprintf(L"CreateService failed w/err 0x%08lx\n", err);
        Logger::Error("InstallService: CreateService failed, GetLastError=" + std::to_string(err));
        goto Cleanup;
    }
    Logger::Info("Service installed successfully.");
    wprintf(L"%s is installed.\n", wsServiceName.c_str());

Cleanup:
    // Centralized cleanup for all allocated resources.
    if (schSCManager)
    {
        CloseServiceHandle(schSCManager);
        schSCManager = NULL;
    }
    if (schService)
    {
        CloseServiceHandle(schService);
        schService = NULL;
    }
}


// Uninstall the service
void SvcInstaller::UninstallService(std::wstring wsServiceName)
{
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;
    SERVICE_STATUS ssSvcStatus = {};

    // Open the local default service control manager database
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL)
    {
        const DWORD err = GetLastError();
        wprintf(L"OpenSCManager failed w/err 0x%08lx\n", err);
        Logger::Error("UninstallService: OpenSCManager failed, GetLastError=" + std::to_string(err));
        goto Cleanup;
    }

    // Open the service with delete, stop, and query status permissions
    schService = OpenService(schSCManager, wsServiceName.c_str(), SERVICE_STOP |
        SERVICE_QUERY_STATUS | DELETE);
    if (schService == NULL)
    {
        const DWORD err = GetLastError();
        wprintf(L"OpenService failed w/err 0x%08lx\n", err);
        Logger::Error("UninstallService: OpenService failed, GetLastError=" + std::to_string(err));
        goto Cleanup;
    }

    // Try to stop the service
    if (ControlService(schService, SERVICE_CONTROL_STOP, &ssSvcStatus))
    {
        wprintf(L"Stopping %s.", wsServiceName.c_str());
        Sleep(1000);

        // Check QueryServiceStatus's own return at each call, rather than
        // polling GetLastError() after the loop - a successful call doesn't
        // reset the last-error, so that would misattribute a stale error from
        // something unrelated as if QueryServiceStatus itself had just failed.
        for (;;)
        {
            if (!QueryServiceStatus(schService, &ssSvcStatus))
            {
                Logger::Error("UninstallService: QueryServiceStatus failed while waiting for stop, "
                    "GetLastError=" + std::to_string(GetLastError()));
                break;
            }
            if (ssSvcStatus.dwCurrentState != SERVICE_STOP_PENDING)
            {
                break;
            }
            wprintf(L".");
            Sleep(1000);
        }

        if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED)
        {
            wprintf(L"\n%s is stopped.\n", wsServiceName.c_str());
        }
        else
        {
            wprintf(L"\n%s failed to stop.\n", wsServiceName.c_str());
        }
    }
    else
    {
        // ERROR_SERVICE_NOT_ACTIVE just means it was already stopped - benign
        // and expected; anything else is worth logging.
        const DWORD err = GetLastError();
        if (err != ERROR_SERVICE_NOT_ACTIVE)
        {
            Logger::Warn("UninstallService: ControlService(STOP) failed, GetLastError=" + std::to_string(err));
        }
    }

    // Now remove the service by calling DeleteService.
    if (!DeleteService(schService))
    {
        const DWORD err = GetLastError();
        wprintf(L"DeleteService failed w/err 0x%08lx\n", err);
        Logger::Error("UninstallService: DeleteService failed, GetLastError=" + std::to_string(err));
        goto Cleanup;
    }
    Logger::Info("Service uninstalled successfully.");
    wprintf(L"%s is removed.\n", wsServiceName.c_str());

Cleanup:
    // Centralized cleanup for all allocated resources.
    if (schSCManager)
    {
        CloseServiceHandle(schSCManager);
        schSCManager = NULL;
    }
    if (schService)
    {
        CloseServiceHandle(schService);
        schService = NULL;
    }
}

void SvcInstaller::RunService() {
    try
    {
        DAXService wfSvc((PWSTR)SERVICE_NAME.c_str());
        if (!DAXService::Run(wfSvc))
        {
            Logger::Error("Service failed to run, GetLastError=" + std::to_string(GetLastError()));
        }
    }
    catch (DWORD dwError)
    {
        Logger::Error("Service failed to initialize, GetLastError=" + std::to_string(dwError));
    }
    catch (const std::exception& ex)
    {
        Logger::Error(std::string("Service failed to initialize: ") + ex.what());
    }
}