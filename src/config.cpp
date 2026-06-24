/**
 * @file config.cpp
 * @brief 地面站客户端配置管理实现文件
 *
 * 本文件实现地面站客户端配置管理功能
 * 核心功能：
 * - 单例模式全局配置管理
 * - 从INI配置文件加载参数
 * - 将当前配置保存至INI文件
 * - 多线程互斥安全读写配置项
 *
 * 配置分类：
 * - 服务端连接配置（地址、端口、超时重连等）
 * - 数据缓冲区配置（收发缓冲区、帧缓存队列大小）
 * - 视频流传输配置（最大并发流、心跳、注册超时）
 * - 日志系统配置（存储路径、日志等级、滚动分割规则）
 *
 * @author 地面站开发组
 * @date 2024
 */

#include "config.h"
#include "logger.h"
#include <QFile>
#include <QDir>

namespace ground_station {

/**
 * @brief 获取全局配置单例实例
 * @return 配置对象静态引用
 *
 * 采用局部静态变量实现C++11标准线程安全单例
 */
Config& Config::instance() {
    static Config instance;
    return instance;
}

/**
 * @brief 构造函数，初始化默认配置
 *
 * 调用resetToDefault()将所有参数恢复出厂默认值
 */
Config::Config() {
    // Initialize with default configuration
    resetToDefault();
}

/**
 * @brief 从INI配置文件加载参数
 * @param configFile 配置文件完整路径
 * @return 文件存在并读取成功返回true，文件不存在返回false（自动使用默认配置）
 *
 * INI配置文件分为4个分组：
 * - [Server] 服务端通信参数
 * - [Buffer] 缓冲区缓存参数
 * - [Stream] 视频流传输参数
 * - [Log] 日志输出存储参数
 */
bool Config::loadFromFile(const QString& configFile) {
    QMutexLocker locker(&m_mutex);

    // 判断配置文件是否存在
    if (!QFile::exists(configFile)) {
        LOG_WARN("Config", QString("配置文件不存在: %1，使用默认配置").arg(configFile));
        return false;
    }

    QSettings settings(configFile, QSettings::IniFormat);

    // 读取服务端连接配置
    settings.beginGroup("Server");
    m_serverConfig.serverAddress = settings.value("Address", m_serverConfig.serverAddress).toString();
    m_serverConfig.commandPort = settings.value("CommandPort", m_serverConfig.commandPort).toUInt();
    m_serverConfig.dataPort = settings.value("DataPort", m_serverConfig.dataPort).toUInt();
    m_serverConfig.connectionTimeout = settings.value("ConnectionTimeout", m_serverConfig.connectionTimeout).toInt();
    m_serverConfig.reconnectInterval = settings.value("ReconnectInterval", m_serverConfig.reconnectInterval).toInt();
    m_serverConfig.maxReconnectAttempts = settings.value("MaxReconnectAttempts", m_serverConfig.maxReconnectAttempts).toInt();
    settings.endGroup();

    // 读取缓冲区配置
    settings.beginGroup("Buffer");
    m_bufferConfig.receiveBufferSize = settings.value("ReceiveBufferSize", m_bufferConfig.receiveBufferSize).toInt();
    m_bufferConfig.sendBufferSize = settings.value("SendBufferSize", m_bufferConfig.sendBufferSize).toInt();
    m_bufferConfig.frameBufferSize = settings.value("FrameBufferSize", m_bufferConfig.frameBufferSize).toInt();
    m_bufferConfig.maxQueueSize = settings.value("MaxQueueSize", m_bufferConfig.maxQueueSize).toInt();
    settings.endGroup();

    // 读取视频流传输配置
    settings.beginGroup("Stream");
    m_streamConfig.maxStreamCount = settings.value("MaxStreamCount", m_streamConfig.maxStreamCount).toInt();
    m_streamConfig.streamTimeout = settings.value("StreamTimeout", m_streamConfig.streamTimeout).toInt();
    m_streamConfig.heartbeatInterval = settings.value("HeartbeatInterval", m_streamConfig.heartbeatInterval).toInt();
    m_streamConfig.registrationTimeout = settings.value("RegistrationTimeout", m_streamConfig.registrationTimeout).toInt();
    settings.endGroup();

    // 读取日志系统配置
    settings.beginGroup("Log");
    m_logConfig.logFilePath = settings.value("FilePath", m_logConfig.logFilePath).toString();
    m_logConfig.logLevel = settings.value("Level", m_logConfig.logLevel).toInt();
    m_logConfig.enableConsole = settings.value("EnableConsole", m_logConfig.enableConsole).toBool();
    m_logConfig.maxLogFileSize = settings.value("MaxFileSize", m_logConfig.maxLogFileSize).toInt();
    m_logConfig.maxLogFiles = settings.value("MaxLogFiles", m_logConfig.maxLogFiles).toInt();
    settings.endGroup();

    LOG_INFO("Config", QString("配置文件加载完成，文件路径：%1").arg(configFile));
    return true;
}

/**
 * @brief 将当前所有配置写入INI文件保存
 * @param configFile 目标配置文件路径
 * @return 写入成功返回true，目录创建/文件写入失败返回false
 *
 * 若文件所在目录不存在，函数会自动逐级创建目录
 */
bool Config::saveToFile(const QString& configFile) {
    QMutexLocker locker(&m_mutex);

    // 校验并创建文件存储目录
    QFileInfo fileInfo(configFile);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            LOG_ERROR("Config", QString("配置目录创建失败，路径：%1").arg(dir.absolutePath()));
            return false;
        }
    }

    QSettings settings(configFile, QSettings::IniFormat);

    settings.beginGroup("Server");
    settings.setValue("Address", m_serverConfig.serverAddress);
    settings.setValue("CommandPort", m_serverConfig.commandPort);
    settings.setValue("DataPort", m_serverConfig.dataPort);
    settings.setValue("ConnectionTimeout", m_serverConfig.connectionTimeout);
    settings.setValue("ReconnectInterval", m_serverConfig.reconnectInterval);
    settings.setValue("MaxReconnectAttempts", m_serverConfig.maxReconnectAttempts);
    settings.endGroup();

    settings.beginGroup("Buffer");
    settings.setValue("ReceiveBufferSize", m_bufferConfig.receiveBufferSize);
    settings.setValue("SendBufferSize", m_bufferConfig.sendBufferSize);
    settings.setValue("FrameBufferSize", m_bufferConfig.frameBufferSize);
    settings.setValue("MaxQueueSize", m_bufferConfig.maxQueueSize);
    settings.endGroup();

    settings.beginGroup("Stream");
    settings.setValue("MaxStreamCount", m_streamConfig.maxStreamCount);
    settings.setValue("StreamTimeout", m_streamConfig.streamTimeout);
    settings.setValue("HeartbeatInterval", m_streamConfig.heartbeatInterval);
    settings.setValue("RegistrationTimeout", m_streamConfig.registrationTimeout);
    settings.endGroup();

    settings.beginGroup("Log");
    settings.setValue("FilePath", m_logConfig.logFilePath);
    settings.setValue("Level", m_logConfig.logLevel);
    settings.setValue("EnableConsole", m_logConfig.enableConsole);
    settings.setValue("MaxFileSize", m_logConfig.maxLogFileSize);
    settings.setValue("MaxLogFiles", m_logConfig.maxLogFiles);
    settings.endGroup();

    return true;
}

ServerConfig Config::getServerConfig() const {
    QMutexLocker locker(&const_cast<QMutex&>(m_mutex));
    return m_serverConfig;
}

void Config::setServerConfig(const ServerConfig& config) {
    QMutexLocker locker(&m_mutex);
    m_serverConfig = config;
}

BufferConfig Config::getBufferConfig() const {
    QMutexLocker locker(&const_cast<QMutex&>(m_mutex));
    return m_bufferConfig;
}

void Config::setBufferConfig(const BufferConfig& config) {
    QMutexLocker locker(&m_mutex);
    m_bufferConfig = config;
}

StreamConfig Config::getStreamConfig() const {
    QMutexLocker locker(&const_cast<QMutex&>(m_mutex));
    return m_streamConfig;
}

void Config::setStreamConfig(const StreamConfig& config) {
    QMutexLocker locker(&m_mutex);
    m_streamConfig = config;
}

LogConfig Config::getLogConfig() const {
    QMutexLocker locker(&const_cast<QMutex&>(m_mutex));
    return m_logConfig;
}

void Config::setLogConfig(const LogConfig& config) {
    QMutexLocker locker(&m_mutex);
    m_logConfig = config;
}

/**
 * @brief 重置全部参数为程序出厂默认值
 */
void Config::resetToDefault() {
    QMutexLocker locker(&m_mutex);
    m_serverConfig = ServerConfig();
    m_bufferConfig = BufferConfig();
    m_streamConfig = StreamConfig();
    m_logConfig = LogConfig();
    LOG_INFO("Config", "所有配置项已恢复默认参数");
}

} // namespace ground_station
