/**
 * @file logger.h
 * @brief 地面站客户端日志系统
 *
 * 提供分级日志功能，支持Debug/Info/Warning/Error四个级别
 * 支持文件输出和控制台输出，线程安全
 */

#ifndef GROUND_STATION_LOGGER_H
#define GROUND_STATION_LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>
#include <memory>

namespace ground_station {

/**
 * @enum LogLevel
 * @brief 日志级别枚举
 */
enum class LogLevel {
    Debug = 0,      // 调试级别
    Info = 1,       // 信息级别
    Warning = 2,    // 警告级别
    Error = 3       // 错误级别
};

/**
 * @class Logger
 * @brief 日志记录器类
 *
 * 单例模式，提供线程安全的日志记录功能
 * 支持文件输出和控制台输出
 */
class Logger {
public:
    /**
     * @brief 获取Logger单例实例
     * @return Logger实例引用
     */
    static Logger& instance();

    /**
     * @brief 初始化日志系统
     * @param logFilePath 日志文件路径
     * @param level 最低日志级别
     * @param enableConsole 是否启用控制台输出
     * @return 初始化是否成功
     */
    bool initialize(const QString& logFilePath,
                    LogLevel level = LogLevel::Info,
                    bool enableConsole = true);

    /**
     * @brief 关闭日志系统
     */
    void shutdown();

    /**
     * @brief 设置最低日志级别
     * @param level 日志级别
     */
    void setLogLevel(LogLevel level);

    /**
     * @brief 记录调试日志
     * @param tag 日志标签
     * @param message 日志消息
     */
    void debug(const QString& tag, const QString& message);

    /**
     * @brief 记录信息日志
     * @param tag 日志标签
     * @param message 日志消息
     */
    void info(const QString& tag, const QString& message);

    /**
     * @brief 记录警告日志
     * @param tag 日志标签
     * @param message 日志消息
     */
    void warning(const QString& tag, const QString& message);

    /**
     * @brief 记录错误日志
     * @param tag 日志标签
     * @param message 日志消息
     */
    void error(const QString& tag, const QString& message);

    /**
     * @brief 通用日志记录接口
     * @param level 日志级别
     * @param tag 日志标签
     * @param message 日志消息
     */
    void log(LogLevel level, const QString& tag, const QString& message);

    // 禁用拷贝和赋值
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger();
    ~Logger();

    /**
     * @brief 获取日志级别字符串
     * @param level 日志级别
     * @return 级别字符串
     */
    QString levelToString(LogLevel level) const;

    /**
     * @brief 格式化日志消息
     * @param level 日志级别
     * @param tag 日志标签
     * @param message 日志消息
     * @return 格式化后的日志字符串
     */
    QString formatMessage(LogLevel level, const QString& tag, const QString& message);

    /**
     * @brief 写入日志
     * @param formattedMessage 格式化后的消息
     */
    void writeLog(const QString& formattedMessage);

    QMutex m_mutex;                     // 互斥锁，保证线程安全
    std::unique_ptr<QFile> m_logFile;   // 日志文件
    QTextStream* m_textStream;          // 文本流
    LogLevel m_minLevel;                // 最低日志级别
    bool m_enableConsole;               // 是否启用控制台输出
    bool m_initialized;                 // 是否已初始化
};

// 便捷宏定义
#define LOG_DEBUG(tag, msg)   ground_station::Logger::instance().debug(tag, msg)
#define LOG_INFO(tag, msg)    ground_station::Logger::instance().info(tag, msg)
#define LOG_WARN(tag, msg)    ground_station::Logger::instance().warning(tag, msg)
#define LOG_ERROR(tag, msg)   ground_station::Logger::instance().error(tag, msg)

}  // namespace ground_station

#endif  // GROUND_STATION_LOGGER_H