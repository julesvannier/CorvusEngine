#pragma once

#include <chrono>
#include <vector>
#include <functional>

enum class LogType
{
    Debug,
    Warning,
    Error
};

class Logger {
public:
    static void Log(LogType type, const std::string& content);
    static void WriteLogsToFile();
    static void DefineOnMessageLogged(const std::function<void(LogType type, const std::string& content)>& onLogFunc) { m_onMessageLogged = onLogFunc; }

private:
    inline static std::vector<std::string> m_logMessages;
    inline static std::function<void(LogType type, const std::string& content)> m_onMessageLogged;
};

#define LOG(type, content) Logger::Log(LogType::type, content)

#define PROFILE_BEGIN() auto profile_start_time = std::chrono::high_resolution_clock::now()
#define PROFILE_END_AND_LOG(type, content) \
auto profile_end_time = std::chrono::high_resolution_clock::now(); \
auto profile_duration = std::chrono::duration_cast<std::chrono::microseconds>(profile_end_time - profile_start_time).count(); \
LOG(type, std::string(content) + " Duration: " + std::to_string(profile_duration) + " microseconds")
