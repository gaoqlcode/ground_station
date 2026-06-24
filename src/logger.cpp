/**
 * @file logger.cpp
 * @brief 地面站客户端日志系统实现
 *
 * 本文件实现了Logger类，提供统一的日志记录功能。
 * 主要功能包括：
 * - 日志文件的创建和管理
 * - 多级别日志记录（调试、信息、警告、错误）
 * - 控制台输出支持
 * - 线程安全的日志写入
 *
 * @author GroundStation Team
 * @version 1.0
 * @date 2024
 */

#include "logger.h"
#include <QCoreApplication>
#include <QDir>
#include <QTextCodec>
#include <iostream>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace ground_station {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger()
    : m_textStream(nullptr)
    , m_minLevel(LogLevel::Info)
    , m_enableConsole(true)
    , m_initialized(false) {
}

Logger::~Logger() {
    shutdown();
}

/**
 * @brief 初始化日志系统
 *
 * 创建日志文件目录，打开日志文件，设置日志级别和控制台输出
 *
 * @param logFilePath 日志文件路径
 * @param level 最低日志级别
 * @param enableConsole 是否启用控制台输出
 * @return 成功返回true，失败返回false
 */
bool Logger::initialize(const QString& logFilePath,
                        LogLevel level,
                        bool enableConsole) {
    QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        return true;
    }

    m_minLevel = level;
    m_enableConsole = enableConsole;

    // 创建日志文件目录
    QFileInfo fileInfo(logFilePath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            std::cerr << "创建日志目录失败: "
                      << QDir::toNativeSeparators(dir.absolutePath()).toStdString() << std::endl;
            return false;
        }
    }

    // 打开日志文件
    m_logFile = std::make_unique<QFile>(logFilePath);
    if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        std::cerr << "打开日志文件失败: "
                  << QDir::toNativeSeparators(logFilePath).toStdString() << std::endl;
        m_logFile.reset();
        return false;
    }

    m_textStream = new QTextStream(m_logFile.get());
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QTextCodec* codec = QTextCodec::codecForName("UTF-8");
    if (codec) {
        m_textStream->setCodec(codec);
    }
#endif

    m_initialized = true;

    // 记录初始化日志
    QString initMsg = QString("日志系统已初始化。日志文件: %1, 最低级别: %2, 控制台: %3")
                          .arg(QDir::toNativeSeparators(logFilePath))
                          .arg(levelToString(m_minLevel))
                          .arg(m_enableConsole ? "已启用" : "已禁用");
    writeLog(formatMessage(LogLevel::Info, "Logger", initMsg));

    return true;
}

void Logger::shutdown() {
    QMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        return;
    }

    writeLog(formatMessage(LogLevel::Info, "Logger", "日志系统正在关闭"));

    if (m_textStream) {
        delete m_textStream;
        m_textStream = nullptr;
    }

    if (m_logFile && m_logFile->isOpen()) {
        m_logFile->close();
        m_logFile.reset();
    }

    m_initialized = false;
}

void Logger::setLogLevel(LogLevel level) {
    QMutexLocker locker(&m_mutex);
    m_minLevel = level;
}

void Logger::debug(const QString& tag, const QString& message) {
    log(LogLevel::Debug, tag, message);
}

void Logger::info(const QString& tag, const QString& message) {
    log(LogLevel::Info, tag, message);
}

/**
 * @brief 记录警告日志
 *
 * @param tag 日志标签
 * @param message 日志消息
 */
void Logger::warning(const QString& tag, const QString& message) {
    log(LogLevel::Warning, tag, message);
}

/**
 * @brief 记录错误日志
 *
 * @param tag 日志标签
 * @param message 日志消息
 */
void Logger::error(const QString& tag, const QString& message) {
    log(LogLevel::Error, tag, message);
}

void Logger::log(LogLevel level, const QString& tag, const QString& message) {
    QMutexLocker locker(&m_mutex);

    // 检查日志级别
    if (static_cast<int>(level) < static_cast<int>(m_minLevel)) {
        return;
    }

    if (!m_initialized) {
        // 如果未初始化，直接输出到控制台（使用正确的编码）
        QString msg = formatMessage(level, tag, message);
#ifdef Q_OS_WIN
        static bool consoleInitialized = false;
        if (!consoleInitialized) {
            SetConsoleOutputCP(CP_UTF8);
            consoleInitialized = true;
        }
        // 使用 WriteConsoleW 直接写入控制台，避免 std::wcout 的问题
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole != INVALID_HANDLE_VALUE) {
            std::wstring wmsg = msg.toStdWString();
            wmsg += L"\n";
            DWORD written;
            WriteConsoleW(hConsole, wmsg.c_str(), wmsg.length(), &written, nullptr);
        } else {
            // 回退到标准输出
            std::wcout << msg.toStdWString() << std::endl;
        }
#else
        std::cout << msg.toStdString() << std::endl;
#endif
        return;
    }

    writeLog(formatMessage(level, tag, message));
}

QString Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        default:
            return "未知";
    }
}

QString Logger::formatMessage(LogLevel level, const QString& tag, const QString& message) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy年MM月dd日 hh:mm:ss");
    QString levelStr = levelToString(level);

    // 格式: [时间戳] [级别] [标签] 消息
    return QString("[%1] [%2] [%3] %4")
        .arg(timestamp)
        .arg(levelStr, -5)  // 左对齐，宽度5
        .arg(tag)
        .arg(message);
}

/**
 * @brief 写入日志
 *
 * 将格式化的日志消息写入文件和控制台
 *
 * @param formattedMessage 格式化后的日志消息
 */
void Logger::writeLog(const QString& formattedMessage) {
    if (m_textStream) {
        *m_textStream << formattedMessage << "\n";
        m_textStream->flush();
    }

    if (m_enableConsole) {
#ifdef Q_OS_WIN
        // Windows 控制台特殊处理：使用 WriteConsoleW 输出宽字符
        static bool consoleInitialized = false;
        if (!consoleInitialized) {
            SetConsoleOutputCP(CP_UTF8);
            consoleInitialized = true;
        }
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole != INVALID_HANDLE_VALUE) {
            std::wstring wmsg = formattedMessage.toStdWString();
            wmsg += L"\n";
            DWORD written;
            WriteConsoleW(hConsole, wmsg.c_str(), wmsg.length(), &written, nullptr);
        } else {
            std::wcout << formattedMessage.toStdWString() << std::endl;
        }
#else
        std::cout << formattedMessage.toStdString() << std::endl;
#endif
    }
}

}  // namespace ground_station
