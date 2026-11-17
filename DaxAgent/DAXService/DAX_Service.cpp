#pragma region Includes
#include "stdafx.h"
#include "Logger.h"
#include "DAX_Service.h"
#include "WinUtils.h"
#include "SvcCommon.h"

#pragma endregion

using namespace ADAX;



DAXService::DAXService(PWSTR pszServiceName,
    BOOL fCanStop,
    BOOL fCanShutdown,
    BOOL fCanPauseContinue)
    : DAXServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue)
{
    m_fStopping = FALSE;
    m_hSvcThread = NULL;

    // Create a manual-reset event that is not signaled at first to indicate 
    // the stopped signal of the service.
    m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_hStoppedEvent == NULL)
    {
        throw GetLastError();
    }

}


DAXService::~DAXService(void)
{
    WinUtils::SAFE_CLOSE_HANDLE(m_hStoppedEvent);
    WinUtils::SAFE_CLOSE_HANDLE(m_hSvcThread);
}


void DAXService::OnStart(DWORD dwArgc, LPWSTR* lpszArgv)
{
    
    DWORD threadId;
    m_hSvcThread = CreateThread(
        nullptr,           // security attributes
        0,                 // stack size
        DAXService::ServiceWorkerThread,  // Thread function
        this,       // Parameter to the thread function
        0,                 // creation flags
        &threadId          // thread ID reference
    );
    ADAX::Logger::Info("OnSTart: Service starting...");

    if (m_hSvcThread == NULL)
    {
        // If thread creation failed, throw the error so caller can handle it.
        throw GetLastError();
    }
}


DWORD DAXService::ServiceWorkerThread(LPVOID lpParam)
{
    DAXService* pData = static_cast<DAXService*>(lpParam);



    // Call the member function
    return pData->ServiceOnStartWorkerThread();
}

DWORD DAXService::ServiceOnStartWorkerThread()
{

    while (!m_fStopping.load(std::memory_order_acquire))
    {
        ::Sleep(1000);
    }

    // --- SHUTDOWN SEQUENCE ---

    SetEvent(m_hStoppedEvent);
    return ERROR_SUCCESS;
}

void DAXService::OnStop()
{
    ADAX::Logger::Info("OnStop: signaling stop");

    m_fStopping.store(true, std::memory_order_release);

    SetServiceStatus(SERVICE_STOP_PENDING);

}
