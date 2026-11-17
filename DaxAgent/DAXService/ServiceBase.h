#pragma once

#include <windows.h>
#include <string>


class DAXServiceBase
{
public:

  
    static BOOL Run(DAXServiceBase& service);

    DAXServiceBase(PWSTR pszServiceName,
        BOOL fCanStop = TRUE,
        BOOL fCanShutdown = TRUE,
        BOOL fCanPauseContinue = FALSE);

    virtual ~DAXServiceBase(void);

    void Stop();

protected:

    virtual void OnStart(DWORD dwArgc, PWSTR* pszArgv);
    virtual void OnStop();
    virtual void OnPause();
    virtual void OnContinue();
    virtual void OnShutdown();
    void SetServiceStatus(DWORD dwCurrentState,
        DWORD dwWin32ExitCode = NO_ERROR,
        DWORD dwWaitHint = 0);

private:

    static void WINAPI ServiceMain(DWORD dwArgc, LPWSTR* lpszArgv);

    static void WINAPI ServiceCtrlHandler(DWORD dwCtrl);

    void Start(DWORD dwArgc, PWSTR* pszArgv);

    void Pause();

    void Continue();

    void Shutdown();

    // The singleton service instance.
    static DAXServiceBase* s_service;

    PWSTR m_name;

    SERVICE_STATUS m_status;

    SERVICE_STATUS_HANDLE m_statusHandle;
};