#pragma once
#ifndef SVC_COMMON_H
#define SVC_COMMON_H

#include <Windows.h>
#include <string>

// Internal name of the service
inline const std::wstring SERVICE_NAME = L"DAXService";

// Displayed name of the service
inline const std::wstring SERVICE_DISPLAY_NAME = L"Windows Advanced AI Observability Service";

// Service start options.
#define SERVICE_START_TYPE  SERVICE_DEMAND_START

// The name of the account under which the service should run
inline const std::wstring SERVICE_ACCOUNT = L"NT AUTHORITY\\System";

// The password to the service account name (empty by default)
inline std::wstring SERVICE_PASSWORD = L"";

// Service dependencies (MULTI_SZ: each name NUL-terminated, whole list double-NUL terminated)
// Use a wide-character array so the embedded nulls are preserved when passing to CreateService
inline const wchar_t SERVICE_DEPENDENCIES[] = L"";

inline const std::wstring LOG_FILE_PATH = L"C:\\ProgramData\\ADAX\\Log.txt";

inline const std::wstring LOG_DIR_PATH = L"C:\\ProgramData\\ADAX";


#endif // SVC_COMMON_H