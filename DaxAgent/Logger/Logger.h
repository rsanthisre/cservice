#pragma once
#include <string>
#include <atomic>
#include <thread>
#include <fstream>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace ADAX {
    enum class LogLevel {
        ERROR_LEVEL = 1, 
        WARNING_LEVEL = 2,
        INFO_LEVEL = 3,
        DEBUG_LEVEL = 4,
        VERBOSE_LEVEL = 5
    };

    struct LogMessage {
        LogLevel level{ LogLevel::INFO_LEVEL };
        uint64_t timestamp{ 0 };    // FILETIME 100ns units
        std::string text;
    };

    class Logger {
    public:
        static Logger& Instance();

        static void Init(const std::wstring& baseLogFile,
            LogLevel maxLevel,
            size_t maxSizeBytes = 10 * 1024 * 1024,   // 10 MB
            uint64_t rotationIntervalSec = 24 * 60 * 60); // daily

        static void Shutdown();

        static void Debug(const std::string& msg);
        static void Info(const std::string& msg);
        static void Warn(const std::string& msg);
        static void Error(const std::string& msg);
        static void Verbose(const std::string& msg);

    private:
        Logger() = default;
        ~Logger();

        void Log(LogLevel level, const std::string& msg);
        void WorkerThread();
        uint64_t NowFileTime(); // 100ns since 1601
        std::string LevelToString(LogLevel lvl);

        // All these functions will called in static with Instance() for better usability

        void _Init(const std::wstring& baseLogFile,
            LogLevel maxLevel,
            size_t maxSizeBytes = 10 * 1024 * 1024,   // 10 MB
            uint64_t rotationIntervalSec = 24 * 60 * 60); // daily

        void _Shutdown();

        // rotation helpers
        void OpenNewFile();
        void RotateIfNeeded(uint64_t nowFileTime, size_t newBytes);
        std::wstring BuildRotatedName(uint64_t nowFileTime);

    private:
        std::atomic<bool> running{ false };
        std::thread worker;

        std::queue<LogMessage> queue;
        std::mutex mtx;
        std::condition_variable cv;

        std::wofstream file;
        LogLevel maxLevel = LogLevel::INFO_LEVEL;

        // rotation config
        std::wstring basePath;          // e.g. C:\ProgramData\ADAX\agent.log
        size_t maxSizeBytes = 0;
        uint64_t rotationIntervalSec = 0;

        // rotation state
        size_t currentSizeBytes = 0;
        uint64_t nextRotationFileTime = 0; // FILETIME units
    };
};