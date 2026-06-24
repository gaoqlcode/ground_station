/**
 * @file config.h
 * @brief 地面站客户端配置管理
 *
 * 管理客户端配置参数，包括服务器地址、端口、缓冲区大小等
 */

#ifndef GROUND_STATION_CONFIG_H
#define GROUND_STATION_CONFIG_H

#include <QString>
#include <QSettings>
#include <QHostAddress>
#include <QMutex>
#include <memory>

namespace ground_station {

/**
 * @struct ServerConfig
 * @brief 服务器配置参数
 */
struct ServerConfig {
    QString serverAddress;      // 服务器IP地址
    quint16 commandPort;        // 命令端口
    quint16 dataPort;           // 数据端口
    int connectionTimeout;      // 连接超时（毫秒）
    int reconnectInterval;      // 重连间隔（毫秒）
    int maxReconnectAttempts;   // 最大重连次数

    ServerConfig()
        : serverAddress("127.0.0.1")
        , commandPort(8888)
        , dataPort(8889)
        , connectionTimeout(5000)
        , reconnectInterval(3000)
        , maxReconnectAttempts(5) {
    }
};

/**
 * @struct BufferConfig
 * @brief 缓冲区配置参数
 */
struct BufferConfig {
    int receiveBufferSize;      // 接收缓冲区大小（字节）
    int sendBufferSize;         // 发送缓冲区大小（字节）
    int frameBufferSize;        // 帧缓冲区大小（帧数）
    int maxQueueSize;           // 最大队列大小

    BufferConfig()
        : receiveBufferSize(1024 * 1024)      // 1MB
        , sendBufferSize(512 * 1024)           // 512KB
        , frameBufferSize(30)                  // 30帧
        , maxQueueSize(100) {
    }
};

/**
 * @struct StreamConfig
 * @brief 流配置参数
 */
struct StreamConfig {
    int maxStreamCount;         // 最大流数量
    int streamTimeout;          // 流超时时间（毫秒）
    int heartbeatInterval;      // 心跳间隔（毫秒）
    int registrationTimeout;    // 注册超时（毫秒）

    StreamConfig()
        : maxStreamCount(2)
        , streamTimeout(10000)
        , heartbeatInterval(5000)
        , registrationTimeout(5000) {
    }
};

/**
 * @struct LogConfig
 * @brief 日志配置参数
 */
struct LogConfig {
    QString logFilePath;        // 日志文件路径
    int logLevel;               // 日志级别 (0=Debug, 1=Info, 2=Warning, 3=Error)
    bool enableConsole;        // 是否启用控制台输出
    int maxLogFileSize;         // 最大日志文件大小（字节）
    int maxLogFiles;            // 最大日志文件数量

    LogConfig()
        : logFilePath("logs/ground_station.log")
        , logLevel(1)           // Info级别
        , enableConsole(true)
        , maxLogFileSize(10 * 1024 * 1024)    // 10MB
        , maxLogFiles(5) {
    }
};

/**
 * @class Config
 * @brief 配置管理类
 *
 * 单例模式，提供配置的读取、保存和管理功能
 * 支持从INI文件加载配置
 */
class Config {
public:
    /**
     * @brief 获取Config单例实例
     * @return Config实例引用
     */
    static Config& instance();

    /**
     * @brief 从文件加载配置
     * @param configFile 配置文件路径
     * @return 是否加载成功
     */
    bool loadFromFile(const QString& configFile);

    /**
     * @brief 保存配置到文件
     * @param configFile 配置文件路径
     * @return 是否保存成功
     */
    bool saveToFile(const QString& configFile);

    /**
     * @brief 获取服务器配置
     * @return 服务器配置
     */
    ServerConfig getServerConfig() const;

    /**
     * @brief 设置服务器配置
     * @param config 服务器配置
     */
    void setServerConfig(const ServerConfig& config);

    /**
     * @brief 获取缓冲区配置
     * @return 缓冲区配置
     */
    BufferConfig getBufferConfig() const;

    /**
     * @brief 设置缓冲区配置
     * @param config 缓冲区配置
     */
    void setBufferConfig(const BufferConfig& config);

    /**
     * @brief 获取流配置
     * @return 流配置
     */
    StreamConfig getStreamConfig() const;

    /**
     * @brief 设置流配置
     * @param config 流配置
     */
    void setStreamConfig(const StreamConfig& config);

    /**
     * @brief 获取日志配置
     * @return 日志配置
     */
    LogConfig getLogConfig() const;

    /**
     * @brief 设置日志配置
     * @param config 日志配置
     */
    void setLogConfig(const LogConfig& config);

    /**
     * @brief 重置为默认配置
     */
    void resetToDefault();

    // 禁用拷贝和赋值
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

private:
    Config();
    ~Config() = default;

    ServerConfig m_serverConfig;
    BufferConfig m_bufferConfig;
    StreamConfig m_streamConfig;
    LogConfig m_logConfig;
    mutable QMutex m_mutex;
};

}  // namespace ground_station

#endif  // GROUND_STATION_CONFIG_H