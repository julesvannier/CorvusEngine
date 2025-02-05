#include "Logger.h"

#include <iostream>
#include <Windows.h>
#include <filesystem>
#include <fstream>

void Logger::Log(LogType logType, const std::string& content)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    std::string logPrefix;

    switch (logType)
    {
    case LogType::Debug:
        logPrefix = "[Debug] ";
        SetConsoleTextAttribute(hConsole, 10);
        break;
    case LogType::Warning:
        logPrefix = "[Warning] ";
        SetConsoleTextAttribute(hConsole, 14);
        break;
    case LogType::Error:
        logPrefix = "[Error] ";
        SetConsoleTextAttribute(hConsole, 12);
        break;
    default:
        logPrefix = "[Debug] ";
        SetConsoleTextAttribute(hConsole, 15);
        break;
    }

    std::cout << logPrefix << content << std::endl;
    m_logMessages.push_back(logPrefix + content);
    if(m_onMessageLogged)
        m_onMessageLogged(logType, logPrefix + content);

    if(logType == LogType::Error)
    {
        WriteLogsToFile();
    }
}

void Logger::WriteLogsToFile()
{
    Log(LogType::Debug, "Writing Logs to file");

    std::filesystem::path currentDir = std::filesystem::current_path(); // filesystem is C++ 17 only
    std::filesystem::path logDir = currentDir / "Log";
    std::filesystem::create_directory(logDir);

    std::filesystem::path logFilePath = logDir / "Log.txt";
    std::ofstream file(logFilePath.string());
    if (file.is_open()) {
        for (const auto& log : m_logMessages) {
            file << log << std::endl;
        }
        file.close();
    }
    else {
        std::cerr << "Failed to open log file." << std::endl;
    }
}