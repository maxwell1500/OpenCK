#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <QString>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QRecursiveMutex>
#include <QMutexLocker>
#include <memory>

namespace OpenCK {
namespace Logging {

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Fatal = 4
};

class Logger {
public:
    static Logger& instance() {
        static Logger instance;
        return instance;
    }

    void init(const QString& logFile = QString()) {
        QMutexLocker locker(&m_mutex);
        if (m_file.isOpen()) {
            m_file.close();
        }
        if (!logFile.isEmpty()) {
            m_file.setFileName(logFile);
            if (m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                m_fileStream.setDevice(&m_file);
            }
        }
        // Flush any messages logged before init()
        for (const QString& line : m_preInitBuffer) {
            if (m_fileStream.device()) {
                m_fileStream << line << "\n";
            }
        }
        m_preInitBuffer.clear();
        m_initialized = true;
        log(LogLevel::Info, "Logging system initialized");
    }

    void log(LogLevel level, const QString& message) {
        QMutexLocker locker(&m_mutex);

        if (static_cast<int>(level) < static_cast<int>(m_minLevel)) {
            return;
        }

        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        QString levelStr = levelString(level);

        QString logLine = QString("[%1] [%2] %3")
            .arg(timestamp)
            .arg(levelStr)
            .arg(message);

        if (m_initialized && m_fileStream.device()) {
            m_fileStream << logLine << "\n";
            m_fileStream.flush();
        }
        else if (!m_initialized) {
            // Buffer messages logged before init() so they aren't silently dropped.
            m_preInitBuffer.append(logLine);
        }

        if (m_initialized) {
            if (level <= LogLevel::Info) {
                printf("%s\n", logLine.toUtf8().constData());
            }
            else {
                fprintf(stderr, "%s\n", logLine.toUtf8().constData());
            }
        }
    }

    void setMinLevel(LogLevel level) {
        QMutexLocker locker(&m_mutex);
        m_minLevel = level;
    }

    LogLevel minLevel() const {
        QMutexLocker locker(&m_mutex);
        return m_minLevel;
    }

    // Path of the active log file, or empty if logging to nowhere.
    QString logFilePath() const {
        QMutexLocker locker(&m_mutex);
        return m_initialized && m_file.isOpen() ? m_file.fileName() : QString();
    }

    ~Logger() {
        if (m_file.isOpen()) {
            m_file.close();
        }
    }

private:
    Logger() : m_minLevel(LogLevel::Debug), m_initialized(false) {}

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static QString levelString(LogLevel level) {
        switch (level) {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info: return "INFO";
            case LogLevel::Warning: return "WARNING";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Fatal: return "FATAL";
            default: return "UNKNOWN";
        }
    }

    mutable QRecursiveMutex m_mutex;
    QFile m_file;
    QTextStream m_fileStream;
    LogLevel m_minLevel;
    QStringList m_preInitBuffer;
    bool m_initialized;
};

// Convenience macros
#define LOG_DEBUG(msg) OpenCK::Logging::Logger::instance().log(OpenCK::Logging::LogLevel::Debug, msg)
#define LOG_INFO(msg) OpenCK::Logging::Logger::instance().log(OpenCK::Logging::LogLevel::Info, msg)
#define LOG_WARNING(msg) OpenCK::Logging::Logger::instance().log(OpenCK::Logging::LogLevel::Warning, msg)
#define LOG_ERROR(msg) OpenCK::Logging::Logger::instance().log(OpenCK::Logging::LogLevel::Error, msg)
#define LOG_FATAL(msg) OpenCK::Logging::Logger::instance().log(OpenCK::Logging::LogLevel::Fatal, msg)

} // namespace Logging
} // namespace OpenCK

#endif // LOGGER_HPP
