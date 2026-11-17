#pragma once
#include "stdafx.h"
#include "ServiceBase.h"
#include <deque>

class DAXService : public DAXServiceBase
{
public:

    DAXService(PWSTR pszServiceName,
        BOOL fCanStop = TRUE,
        BOOL fCanShutdown = TRUE,
        BOOL fCanPauseContinue = FALSE);
    virtual ~DAXService(void);
    static DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);
    

protected:

    virtual void OnStart(DWORD dwArgc, LPWSTR* pszArgv);
    virtual void OnStop();
private:


    DWORD WINAPI ServiceOnStartWorkerThread();




private:

    std::atomic<bool> m_fStopping{ false };

    HANDLE m_hStoppedEvent;

    HANDLE m_hSvcThread;

};

