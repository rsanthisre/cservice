#include "stdafx.h"
#include "Logger.h"
#include <windows.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <cstdio> // _wrename

using namespace ADAX;
using namespace std;

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

void Logger::Init(const std::wstring& baseLogFile,
    LogLevel level,
    size_t maxSize,
    uint64_t rotationSec)
{
    Instance()._Init(baseLogFile, level, maxSize, rotationSec);
}

void Logger::_Init(const std::wstring& baseLogFile,
    LogLevel level,
    size_t maxSize,
    uint64_t rotationSec)
{
    maxLevel = level;
    basePath = baseLogFile;
    maxSizeBytes = maxSize;
    rotationIntervalSec = rotationSec;

    OpenNewFile();

    running = true;
    worker = std::thread(&Logger::WorkerThread, this);
}

void Logger::Shutdown() {
    Instance()._Shutdown();
}

void Logger::_Shutdown() {
    running = false;
    cv.notify_all();

    if (worker.joinable())
        worker.join();

    if (file.is_open())
        file.close();
}

Logger::~Logger() {
    Shutdown();
}

// -------------------------------------------------------------
// Internal: open (or reopen) base log file and reset counters
// -------------------------------------------------------------
void Logger::OpenNewFile() {
    if (file.is_open())
        file.close();

    file.open(basePath, std::ios::out | std::ios::app);
    if (!file.is_open()) {
        throw std::runtime_error("Logger: Failed to open log file");
    }

    // get current size (append mode)
    file.seekp(0, std::ios::end);
    currentSizeBytes = static_cast<size_t>(file.tellp());

    // set next time-based rotation
    if (rotationIntervalSec > 0) {
        uint64_t now = NowFileTime();
        // FILETIME is 100ns units, convert seconds -> 100ns
        uint64_t delta = rotationIntervalSec * 10'000'000ULL;
        nextRotationFileTime = now + delta;
    }
    else {
        nextRotationFileTime = 0;
    }
}

// -------------------------------------------------------------
// Hash-time-based rotated filename: base + ".YYYYMMDD_HHMMSS.log"
// e.g. agent_20251121_201533.log
// -------------------------------------------------------------
std::wstring Logger::BuildRotatedName(uint64_t nowFileTime) {
    FILETIME ft;
    ft.dwLowDateTime = static_cast<DWORD>(nowFileTime & 0xFFFFFFFFULL);
    ft.dwHighDateTime = static_cast<DWORD>((nowFileTime >> 32) & 0xFFFFFFFFULL);

    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);

    // derive base without extension
    std::wstring name = basePath;
    auto dotPos = name.find_last_of(L'.');
    std::wstring stem = (dotPos == std::wstring::npos) ? name : name.substr(0, dotPos);
    std::wstring ext = (dotPos == std::wstring::npos) ? L"" : name.substr(dotPos);

    std::wstringstream ss;
    ss << stem << L"_"
        << st.wYear
        << std::setw(2) << std::setfill(L'0') << st.wMonth
        << std::setw(2) << std::setfill(L'0') << st.wDay
        << L"_"
        << std::setw(2) << std::setfill(L'0') << st.wHour
        << std::setw(2) << std::setfill(L'0') << st.wMinute
        << std::setw(2) << std::setfill(L'0') << st.wSecond
        << ext;

    return ss.str();
}

// -------------------------------------------------------------
// Check if we need to rotate, based on size/time
// Called ONLY by WorkerThread (single consumer)
// -------------------------------------------------------------
void Logger::RotateIfNeeded(uint64_t nowFileTime, size_t newBytes) {
    bool needRotate = false;

    if (maxSizeBytes > 0 && (currentSizeBytes + newBytes) > maxSizeBytes) {
        needRotate = true;
    }
    else if (nextRotationFileTime > 0 && nowFileTime >= nextRotationFileTime) {
        needRotate = true;
    }

    if (!needRotate)
        return;

    // Close current file, rename, reopen base file
    file.flush();
    file.close();

    std::wstring rotated = BuildRotatedName(nowFileTime);

    // If target exists, best effort: delete it
    _wremove(rotated.c_str());
    if (_wrename(basePath.c_str(), rotated.c_str()) != 0)
    {
        // Best-effort: if the rename fails (e.g. target locked by another
        // process), OpenNewFile() below just reopens/appends to the existing
        // base file instead of a fresh rotated one - no data is lost.
        std::string narrowRotated;
        narrowRotated.reserve(rotated.size());
        for (wchar_t wc : rotated)
        {
            narrowRotated.push_back(static_cast<char>(wc));
        }
        Log(LogLevel::WARNING_LEVEL, "Logger: failed to rotate log file to " + narrowRotated);
    }

    // Open new base file and reset counters
    OpenNewFile();
}

// -------------------------------------------------------------
// PRODUCER: Push log message into queue (thread-safe)
// -------------------------------------------------------------
void Logger::Log(LogLevel level, const std::string& msg) {
    if (level > maxLevel) return;

    LogMessage lm;
    lm.level = level;
    lm.timestamp = NowFileTime();
    lm.text = msg;

    {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(std::move(lm));
    }
    cv.notify_one();
}

// convenience
void Logger::Debug(const std::string& msg) { Instance().Log(LogLevel::DEBUG_LEVEL, msg); }
void Logger::Info(const std::string& msg) { Instance().Log(LogLevel::INFO_LEVEL, msg); }
void Logger::Warn(const std::string& msg) { Instance().Log(LogLevel::WARNING_LEVEL, msg); }
void Logger::Error(const std::string& msg) { Instance().Log(LogLevel::ERROR_LEVEL, msg); }
void Logger::Verbose(const std::string& msg) { Instance().Log(LogLevel::VERBOSE_LEVEL, msg); }

// -------------------------------------------------------------
// BACKGROUND THREAD: drains queue and writes to file
// -------------------------------------------------------------
void Logger::WorkerThread() {
    while (running) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return !queue.empty() || !running; });

        while (!queue.empty()) {
            LogMessage msg = std::move(queue.front());
            queue.pop();

            lock.unlock();

            // time breakdown for printing
            FILETIME ft;
            ft.dwLowDateTime = static_cast<DWORD>(msg.timestamp & 0xFFFFFFFFULL);
            ft.dwHighDateTime = static_cast<DWORD>((msg.timestamp >> 32) & 0xFFFFFFFFULL);

            SYSTEMTIME st;
            FileTimeToSystemTime(&ft, &st);

            std::wstringstream line;
            line << st.wYear << L"-"
                << std::setw(2) << std::setfill(L'0') << st.wMonth << L"-"
                << std::setw(2) << std::setfill(L'0') << st.wDay << L" "
                << std::setw(2) << std::setfill(L'0') << st.wHour << L":"
                << std::setw(2) << std::setfill(L'0') << st.wMinute << L":"
                << std::setw(2) << std::setfill(L'0') << st.wSecond << L"."
                << std::setw(3) << std::setfill(L'0') << st.wMilliseconds
                << L" [" << LevelToString(msg.level).c_str() << L"] "
                << msg.text.c_str()
                << L"\n";

            std::wstring wline = line.str();
            size_t bytes = wline.size() * sizeof(wchar_t);

            // check rotation BEFORE writing
            RotateIfNeeded(msg.timestamp, bytes);

            // write
            file << wline;
            currentSizeBytes += bytes;

            lock.lock();
        }

        file.flush();
    }

    // drain any remaining logs before exit
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        if (queue.empty())
            break;
        LogMessage msg = std::move(queue.front());
        queue.pop();
        lock.unlock();

        // simple write, ignoring rotation at shutdown
        file << msg.text.c_str() << L"\n";
    }

    file.flush();
}

// -------------------------------------------------------------
// Time helper: current FILETIME (100ns since 1601)
// -------------------------------------------------------------
uint64_t Logger::NowFileTime() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;
    return ull.QuadPart;
}

std::string Logger::LevelToString(LogLevel lvl) {
    switch (lvl) {
    case LogLevel::DEBUG_LEVEL: return "D";
    case LogLevel::INFO_LEVEL:  return "I";
    case LogLevel::WARNING_LEVEL:  return "W";
    case LogLevel::ERROR_LEVEL: return "E";
    case LogLevel::VERBOSE_LEVEL: return "V";

    default:              return "?";
    }
}
