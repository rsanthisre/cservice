#pragma region Includes
#include "stdafx.h"
#include "Logger.h"
#include "ServiceBase.h"
#include "WinUtils.h"
#include <strsafe.h>
#include <stdexcept>
#pragma endregion

using namespace ADAX;

#pragma region Static Members

// Initialize the singleton service instance.
DAXServiceBase* DAXServiceBase::s_service = NULL;


//
//   FUNCTION: DAXServiceBase::Run(DAXServiceBase &)
//
//   PURPOSE: Register the executable for a service with the Service Control 
//   Manager (SCM). After you call Run(ServiceBase), the SCM issues a Start 
//   command, which results in a call to the OnStart method in the service. 
//   This method blocks until the service has stopped.
//
//   PARAMETERS:
//   * service - the reference to a DAXServiceBase object. It will become the 
//     singleton service instance of this service application.
//
//   RETURN VALUE: If the function succeeds, the return value is TRUE. If the 
//   function fails, the return value is FALSE. To get extended error 
//   information, call GetLastError.
//
BOOL DAXServiceBase::Run(DAXServiceBase& service)
{
    s_service = &service;

    SERVICE_TABLE_ENTRY serviceTable[] =
    {
        { service.m_name, ServiceMain },
        { NULL, NULL }
    };

    // Connects the main thread of a service process to the service control 
    // manager, which causes the thread to be the service control dispatcher 
    // thread for the calling process. This call returns when the service has 
    // stopped. The process should simply terminate when the call returns.
    return StartServiceCtrlDispatcher(serviceTable);
}


//
//   FUNCTION: DAXServiceBase::ServiceMain(DWORD, PWSTR *)
//
//   PURPOSE: Entry point for the service. It registers the handler function 
//   for the service and starts the service.
//
//   PARAMETERS:
//   * dwArgc   - number of command line arguments
//   * lpszArgv - array of command line arguments
//
void WINAPI DAXServiceBase::ServiceMain(DWORD dwArgc, LPWSTR* lpszArgv)
{
    if (s_service == NULL) {
        Logger::Error("Service Object is NULL");
        return; // Prevent dereferencing NULL pointer
    }

    try
    {
        // Register the handler function for the service
        s_service->m_statusHandle = RegisterServiceCtrlHandler(
            s_service->m_name, ServiceCtrlHandler);
        if (s_service->m_statusHandle == NULL)
        {
            throw GetLastError();
        }

        // Start the service.
        s_service->Start(dwArgc, lpszArgv);
    }
    catch (DWORD dwError)
    {
        Logger::Error("ServiceMain: RegisterServiceCtrlHandler failed, GetLastError=" + std::to_string(dwError));
    }
    catch (const std::exception& ex)
    {
        Logger::Error(std::string("ServiceMain: unexpected exception: ") + ex.what());
    }
    catch (...)
    {
        Logger::Error("ServiceMain: unknown exception");
    }
}


//
//   FUNCTION: DAXServiceBase::ServiceCtrlHandler(DWORD)
//
//   PURPOSE: The function is called by the SCM whenever a control code is 
//   sent to the service. 
//
void WINAPI DAXServiceBase::ServiceCtrlHandler(DWORD dwCtrl)
{
    switch (dwCtrl)
    {
    case SERVICE_CONTROL_STOP: s_service->Stop(); break;
    case SERVICE_CONTROL_PAUSE: s_service->Pause(); break;
    case SERVICE_CONTROL_CONTINUE: s_service->Continue(); break;
    case SERVICE_CONTROL_SHUTDOWN: s_service->Shutdown(); break;
    case SERVICE_CONTROL_INTERROGATE: break;
    default: break;
    }
}

#pragma endregion


#pragma region Service Constructor and Destructor

DAXServiceBase::DAXServiceBase(PWSTR pszServiceName,
    BOOL fCanStop,
    BOOL fCanShutdown,
    BOOL fCanPauseContinue)
{
    // Service name must be a valid string and cannot be NULL.
    m_name = (pszServiceName == NULL) ? (PWSTR)L"" : pszServiceName;

    m_statusHandle = NULL;

    // The service runs in its own process.
    m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

    // The service is starting.
    m_status.dwCurrentState = SERVICE_START_PENDING;

    // The accepted commands of the service.
    DWORD dwControlsAccepted = 0;
    if (fCanStop)
        dwControlsAccepted |= SERVICE_ACCEPT_STOP;
    if (fCanShutdown)
        dwControlsAccepted |= SERVICE_ACCEPT_SHUTDOWN;
    if (fCanPauseContinue)
        dwControlsAccepted |= SERVICE_ACCEPT_PAUSE_CONTINUE;
    m_status.dwControlsAccepted = dwControlsAccepted;

    m_status.dwWin32ExitCode = NO_ERROR;
    m_status.dwServiceSpecificExitCode = 0;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;
}


//
//   FUNCTION: DAXServiceBase::~DAXServiceBase()
//
//   PURPOSE: The virtual destructor of DAXServiceBase. 
//
DAXServiceBase::~DAXServiceBase(void)
{
}

#pragma endregion


#pragma region Service Start, Stop, Pause, Continue, and Shutdown


void DAXServiceBase::Start(DWORD dwArgc, PWSTR* pszArgv)
{
    try
    {
        // Tell SCM that the service is starting.
        SetServiceStatus(SERVICE_START_PENDING);

        // Perform service-specific initialization.
        OnStart(dwArgc, pszArgv);

        // Tell SCM that the service is started.
        SetServiceStatus(SERVICE_RUNNING);
    }
    catch (DWORD dwError)
    {
        // Log the error.
        Logger::Error("Service Start failed with error code: " + std::to_string(dwError));

        // Set the service status to be stopped.
        SetServiceStatus(SERVICE_STOPPED, dwError);
    }
    catch (...)
    {
        // Log the error.
        Logger::Error("Service failed to start.");

        // Set the service status to be stopped.
        SetServiceStatus(SERVICE_STOPPED);
    }
}


//
//   FUNCTION: DAXServiceBase::OnStart(DWORD, PWSTR *)
//
//   PURPOSE: When implemented in a derived class, executes when a Start 
//   command is sent to the service by the SCM or when the operating system 
//   starts (for a service that starts automatically). Specifies actions to 
//   take when the service starts. Be sure to periodically call 
//   DAXServiceBase::SetServiceStatus() with SERVICE_START_PENDING if the 
//   procedure is going to take long time. You may also consider spawning a 
//   new thread in OnStart to perform time-consuming initialization tasks.
//
//   PARAMETERS:
//   * dwArgc   - number of command line arguments
//   * lpszArgv - array of command line arguments
//
void DAXServiceBase::OnStart(DWORD dwArgc, PWSTR* pszArgv)
{
}


//
//   FUNCTION: DAXServiceBase::Stop()
//
//   PURPOSE: The function stops the service. It calls the OnStop virtual 
//   function in which you can specify the actions to take when the service 
//   stops. If an error occurs, the error will be logged in the Application 
//   event log, and the service will be restored to the original state.
//
void DAXServiceBase::Stop()
{
    DWORD dwOriginalState = m_status.dwCurrentState;
    try
    {
        // Tell SCM that the service is stopping.
        SetServiceStatus(SERVICE_STOP_PENDING);

        // Perform service-specific stop operations.
        OnStop();

        // Tell SCM that the service is stopped.
        SetServiceStatus(SERVICE_STOPPED);
    }
    catch (DWORD dwError)
    {
        // Log the error.
        Logger::Error("Service Stop failed with error code: " + std::to_string(dwError));

        // Set the orginal service status.
        SetServiceStatus(dwOriginalState);
    }
    catch (...)
    {
        // Log the error.
        Logger::Error("Service failed to stop.");

        // Set the orginal service status.
        SetServiceStatus(dwOriginalState);
    }
}


//
//   FUNCTION: DAXServiceBase::OnStop()
//
//   PURPOSE: When implemented in a derived class, executes when a Stop 
//   command is sent to the service by the SCM. Specifies actions to take 
//   when a service stops running. Be sure to periodically call 
//   DAXServiceBase::SetServiceStatus() with SERVICE_STOP_PENDING if the 
//   procedure is going to take long time. 
//
void DAXServiceBase::OnStop()
{
}


//
//   FUNCTION: DAXServiceBase::Pause()
//
//   PURPOSE: The function pauses the service if the service supports pause 
//   and continue. It calls the OnPause virtual function in which you can 
//   specify the actions to take when the service pauses. If an error occurs, 
//   the error will be logged in the Application event log, and the service 
//   will become running.
//
void DAXServiceBase::Pause()
{
    try
    {
        // Tell SCM that the service is pausing.
        SetServiceStatus(SERVICE_PAUSE_PENDING);

        // Perform service-specific pause operations.
        OnPause();

        // Tell SCM that the service is paused.
        SetServiceStatus(SERVICE_PAUSED);
    }
    catch (DWORD dwError)
    {
        // Log the error.
        Logger::Error("Service Pause failed with error code: " + std::to_string(dwError));

        // Tell SCM that the service is still running.
        SetServiceStatus(SERVICE_RUNNING);
    }
    catch (...)
    {
        // Log the error.
        Logger::Error("Service failed to pause.");

        // Tell SCM that the service is still running.
        SetServiceStatus(SERVICE_RUNNING);
    }
}


//
//   FUNCTION: DAXServiceBase::OnPause()
//
//   PURPOSE: When implemented in a derived class, executes when a Pause 
//   command is sent to the service by the SCM. Specifies actions to take 
//   when a service pauses.
//
void DAXServiceBase::OnPause()
{
}


//
//   FUNCTION: DAXServiceBase::Continue()
//
//   PURPOSE: The function resumes normal functioning after being paused if
//   the service supports pause and continue. It calls the OnContinue virtual 
//   function in which you can specify the actions to take when the service 
//   continues. If an error occurs, the error will be logged in the 
//   Application event log, and the service will still be paused.
//
void DAXServiceBase::Continue()
{
    try
    {
        // Tell SCM that the service is resuming.
        SetServiceStatus(SERVICE_CONTINUE_PENDING);

        // Perform service-specific continue operations.
        OnContinue();

        // Tell SCM that the service is running.
        SetServiceStatus(SERVICE_RUNNING);
    }
    catch (DWORD dwError)
    {
        // Log the error.
        Logger::Error("Service Continue failed with error code: " + std::to_string(dwError));

        // Tell SCM that the service is still paused.
        SetServiceStatus(SERVICE_PAUSED);
    }
    catch (...)
    {
        // Log the error.
        Logger::Error("Service failed to continue.");

        // Tell SCM that the service is still paused.
        SetServiceStatus(SERVICE_PAUSED);
    }
}

void DAXServiceBase::OnContinue()
{
}


//
//   FUNCTION: DAXServiceBase::Shutdown()
//
//   PURPOSE: The function executes when the system is shutting down. It 
//   calls the OnShutdown virtual function in which you can specify what 
//   should occur immediately prior to the system shutting down. If an error 
//   occurs, the error will be logged in the Application event log.
//
void DAXServiceBase::Shutdown()
{
    try
    {
        // Perform service-specific shutdown operations.
        OnShutdown();

        // Tell SCM that the service is stopped.
        SetServiceStatus(SERVICE_STOPPED);
    }
    catch (DWORD dwError)
    {
        // Log the error.
        Logger::Error("Service Shutdown failed with error code: " + std::to_string(dwError));
    }
    catch (...)
    {
        // Log the error.
        Logger::Error("Service failed to shut down.");
    }
}

void DAXServiceBase::OnShutdown()
{
}

#pragma endregion


#pragma region Helper Functions

//
//   FUNCTION: DAXServiceBase::SetServiceStatus(DWORD, DWORD, DWORD)
//
//   PURPOSE: The function sets the service status and reports the status to 
//   the SCM.
//
//   PARAMETERS:
//   * dwCurrentState - the state of the service
//   * dwWin32ExitCode - error code to report
//   * dwWaitHint - estimated time for pending operation, in milliseconds
//
void DAXServiceBase::SetServiceStatus(DWORD dwCurrentState,
    DWORD dwWin32ExitCode,
    DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;

    // Fill in the SERVICE_STATUS structure of the service.

    m_status.dwCurrentState = dwCurrentState;
    m_status.dwWin32ExitCode = dwWin32ExitCode;
    m_status.dwWaitHint = dwWaitHint;

    m_status.dwCheckPoint =
        ((dwCurrentState == SERVICE_RUNNING) ||
            (dwCurrentState == SERVICE_STOPPED)) ?
        0 : dwCheckPoint++;

    // Report the status of the service to the SCM.
    if (!::SetServiceStatus(m_statusHandle, &m_status))
    {
        Logger::Error("DAXServiceBase::SetServiceStatus: SetServiceStatus failed, GetLastError=" +
            std::to_string(GetLastError()));
    }
}

#pragma endregion