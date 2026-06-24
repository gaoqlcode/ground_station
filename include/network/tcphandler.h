/**
 * @file tcphandler.h
 * @brief TCP命令处理器头文件
 *
 * 提供TCP命令通信功能，包括：
 * - TCP连接管理
 * - 命令发送
 * - 响应接收
 * - 心跳保活
 * - 重连机制
 */

#ifndef GROUND_STATION_TCPHANDLER_H
#define GROUND_STATION_TCPHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QTimer>
#include <QMutex>
#include <QString>
#include <QStringList>
#include <cstdint>

#include "protocolparser.h"

namespace ground_station {

// ============================================================================
// 常量定义
// ============================================================================

/// 默认TCP服务器端口
constexpr int DEFAULT_TCP_PORT = 8888;

/// 默认连接超时（毫秒）
constexpr std::uint32_t DEFAULT_CONNECT_TIMEOUT_MS = 5000;

/// 默认心跳间隔（毫秒）
constexpr std::uint32_t DEFAULT_HEARTBEAT_INTERVAL_MS = 10000;

/// 默认心跳超时（毫秒）
constexpr std::uint32_t DEFAULT_HEARTBEAT_TIMEOUT_MS = 30000;

/// 默认重连间隔（毫秒）
constexpr std::uint32_t DEFAULT_RECONNECT_INTERVAL_MS = 5000;

/// 最大重连次数
constexpr std::uint32_t MAX_RECONNECT_COUNT = 10;

/// 最大命令等待时间（毫秒）
constexpr std::uint32_t DEFAULT_CMD_TIMEOUT_MS = 5000;

// ============================================================================
// 枚举类型
// ============================================================================

/**
 * @enum ConnectionState
 * @brief 连接状态枚举
 */
enum class ConnectionState {
    Disconnected,   ///< 未连接
    Connecting,     ///< 连接中
    Connected,      ///< 已连接
    Registered,     ///< 已注册
    Active,         ///< 活跃（心跳正常）
    Inactive,       ///< 非活跃（心跳超时）
    Error           ///< 错误
};

/**
 * @enum HandlerError
 * @brief 处理器错误类型
 */
enum class HandlerError {
    None = 0,               ///< 无错误
    SocketError,            ///< Socket错误
    ConnectionError,        ///< 连接错误
    TimeoutError,           ///< 超时错误
    ProtocolError,          ///< 协议错误
    CommandError,           ///< 命令错误
    HeartbeatTimeout,       ///< 心跳超时
    ReconnectFailed         ///< 重连失败
};

// ============================================================================
// 结构体定义
// ============================================================================

/**
 * @struct HandlerStats
 * @brief 处理器统计信息
 */
struct HandlerStats {
    std::uint64_t commands_sent = 0;        ///< 发送命令数
    std::uint64_t responses_received = 0;   ///< 接收响应数
    std::uint64_t heartbeats_sent = 0;      ///< 发送心跳数
    std::uint64_t heartbeats_received = 0;  ///< 接收心跳数
    std::uint64_t reconnect_count = 0;      ///< 重连次数
    std::uint64_t errors = 0;               ///< 错误数
    
    double avg_response_time_ms = 0.0;      ///< 平均响应时间
    double avg_latency_ms = 0.0;            ///< 平均延迟
    
    QString last_error;                     ///< 最后错误信息
    QString last_command;                   ///< 最后发送的命令
};

/**
 * @struct PendingCommand
 * @brief 待响应命令结构
 */
struct PendingCommand {
    QString command_name;                   ///< 命令名称
    QByteArray raw_command;                 ///< 原始命令数据
    std::uint64_t send_time;                ///< 发送时间（微秒）
    std::uint32_t timeout_ms;               ///< 超时时间（毫秒）
    bool waiting_response = true;           ///< 是否等待响应
};

// ============================================================================
// TcpHandler 类
// ============================================================================

/**
 * @class TcpHandler
 * @brief TCP命令处理器类
 *
 * 实现TCP命令通信功能，包括：
 * - TCP连接管理
 * - 命令发送与响应接收
 * - 心跳保活
 * - 自动重连
 *
 * 使用方式：
 * 1. 创建 TcpHandler 实例
 * 2. 连接相关信号
 * 3. 调用 connectToServer() 连接服务器
 * 4. 调用 sendCommand() 发送命令
 * 5. 调用 disconnectFromServer() 断开连接
 */
class TcpHandler : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit TcpHandler(QObject* parent = nullptr);

    /**
     * @brief 构造函数
     * @param server_ip 服务器IP
     * @param server_port 服务器端口
     * @param parent 父对象
     */
    explicit TcpHandler(const QString& server_ip, int server_port, QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~TcpHandler() override;

    // 禁用拷贝
    TcpHandler(const TcpHandler&) = delete;
    TcpHandler& operator=(const TcpHandler&) = delete;

    // ========================================================================
    // 配置接口
    // ========================================================================

    /**
     * @brief 设置服务器地址
     * @param ip IP地址
     * @param port 端口号
     */
    void setServerAddress(const QString& ip, int port);

    /**
     * @brief 获取服务器IP
     * @return IP地址
     */
    QString getServerIp() const;

    /**
     * @brief 获取服务器端口
     * @return 端口号
     */
    int getServerPort() const;

    /**
     * @brief 设置连接超时
     * @param timeout_ms 超时时间（毫秒）
     */
    void setConnectTimeout(std::uint32_t timeout_ms);

    /**
     * @brief 设置心跳间隔
     * @param interval_ms 间隔（毫秒）
     */
    void setHeartbeatInterval(std::uint32_t interval_ms);

    /**
     * @brief 设置心跳超时
     * @param timeout_ms 超时时间（毫秒）
     */
    void setHeartbeatTimeout(std::uint32_t timeout_ms);

    /**
     * @brief 设置命令超时
     * @param timeout_ms 超时时间（毫秒）
     */
    void setCommandTimeout(std::uint32_t timeout_ms);

    /**
     * @brief 设置重连间隔
     * @param interval_ms 间隔（毫秒）
     */
    void setReconnectInterval(std::uint32_t interval_ms);

    /**
     * @brief 启用/禁用自动重连
     * @param enabled 是否启用
     */
    void setAutoReconnectEnabled(bool enabled);

    /**
     * @brief 设置最大重连次数
     * @param count 最大次数
     */
    void setMaxReconnectCount(std::uint32_t count);

    /**
     * @brief 设置客户端ID
     * @param client_id 客户端ID
     */
    void setClientId(const QString& client_id);

    /**
     * @brief 获取客户端ID
     * @return 客户端ID
     */
    QString getClientId() const;

    // ========================================================================
    // 控制接口
    // ========================================================================

    /**
     * @brief 连接到服务器
     * @return true 成功开始连接；false 失败
     */
    bool connectToServer();

    /**
     * @brief 断开与服务器连接
     */
    void disconnectFromServer();

    /**
     * @brief 检查是否已连接
     * @return true 已连接；false 未连接
     */
    bool isConnected() const;

    /**
     * @brief 检查是否已注册
     * @return true 已注册；false 未注册
     */
    bool isRegistered() const;

    /**
     * @brief 获取当前连接状态
     * @return 连接状态
     */
    ConnectionState getState() const;

    /**
     * @brief 发送命令
     * @param command 命令字符串
     * @return true 发送成功；false 失败
     */
    bool sendCommand(const QString& command);

    /**
     * @brief 发送命令
     * @param type 命令类型
     * @param args 参数列表
     * @return true 发送成功；false 失败
     */
    bool sendCommand(CommandType type, const QStringList& args = QStringList());

    /**
     * @brief 发送命令并等待响应
     * @param command 命令字符串
     * @param timeout_ms 超时时间（毫秒）
     * @return 响应数据，失败返回空
     */
    Response sendCommandAndWait(const QString& command, std::uint32_t timeout_ms = DEFAULT_CMD_TIMEOUT_MS);

    /**
     * @brief 发送命令并等待响应
     * @param type 命令类型
     * @param args 参数列表
     * @param timeout_ms 超时时间（毫秒）
     * @return 响应数据，失败返回空
     */
    Response sendCommandAndWait(CommandType type, const QStringList& args = QStringList(),
                                 std::uint32_t timeout_ms = DEFAULT_CMD_TIMEOUT_MS);

    /**
     * @brief 发送心跳
     * @return true 发送成功；false 失败
     */
    bool sendHeartbeat();

    /**
     * @brief 注册客户端
     * @param user_agent 用户代理信息
     * @return true 注册成功；false 失败
     */
    bool registerClient(const QString& user_agent = "GroundStation-Qt");

    /**
     * @brief 订阅流
     * @param stream_id 流ID
     * @return true 成功；false 失败
     */
    bool subscribeStream(const QString& stream_id);

    /**
     * @brief 取消订阅流
     * @param stream_id 流ID
     * @return true 成功；false 失败
     */
    bool unsubscribeStream(const QString& stream_id);

    /**
     * @brief 切换流
     * @param stream_id 流ID
     * @return true 成功；false 失败
     */
    bool switchStream(const QString& stream_id);

    /**
     * @brief 获取流列表
     * @return 流信息列表
     */
    QList<StreamInfo> getStreamList();

    /**
     * @brief 获取相机列表
     * @return 相机信息列表
     */
    QList<CameraInfo> getCameraList();

    /**
     * @brief 获取统计信息
     * @return 统计信息
     */
    HandlerStats getStats() const;

    /**
     * @brief 重置统计信息
     */
    void resetStats();

    /**
     * @brief 获取TCP Socket
     * @return Socket指针
     */
    QTcpSocket* getSocket();

signals:
    /**
     * @brief 连接状态变更信号
     * @param state 新状态
     */
    void stateChanged(ConnectionState state);

    /**
     * @brief 响应接收信号
     * @param response 响应数据
     */
    void responseReceived(const Response& response);

    /**
     * @brief 命令发送信号
     * @param command 命令字符串
     */
    void commandSent(const QString& command);

    /**
     * @brief 心跳发送信号
     */
    void heartbeatSent();

    /**
     * @brief 心跳接收信号
     */
    void heartbeatReceived();

    /**
     * @brief 错误发生信号
     * @param error 错误类型
     * @param message 错误消息
     */
    void errorOccurred(HandlerError error, const QString& message);

    /**
     * @brief 重连尝试信号
     * @param attempt 当前尝试次数
     * @param max_attempts 最大尝试次数
     */
    void reconnectAttempt(std::uint32_t attempt, std::uint32_t max_attempts);

    /**
     * @brief 统计更新信号
     * @param stats 统计信息
     */
    void statsUpdated(const HandlerStats& stats);

private slots:
    /**
     * @brief Socket连接成功槽函数
     */
    void onConnected();

    /**
     * @brief Socket断开连接槽函数
     */
    void onDisconnected();

    /**
     * @brief Socket错误槽函数
     */
    void onSocketError(QAbstractSocket::SocketError error);

    /**
     * @brief 数据就绪槽函数
     */
    void onReadyRead();

    /**
     * @brief 心跳定时器槽函数
     */
    void onHeartbeatTimer();

    /**
     * @brief 心跳超时检测槽函数
     */
    void onHeartbeatTimeoutCheck();

    /**
     * @brief 命令超时检测槽函数
     */
    void onCommandTimeoutCheck();

    /**
     * @brief 重连定时器槽函数
     */
    void onReconnectTimer();

private:
    /**
     * @brief 初始化
     */
    void initialize();

    /**
     * @brief 处理接收到的数据
     */
    void processReceivedData();

    /**
     * @brief 处理响应
     * @param response 响应数据
     */
    void handleResponse(const Response& response);

    /**
     * @brief 处理心跳响应
     * @param response 响应数据
     */
    void handleHeartbeatResponse(const Response& response);

    /**
     * @brief 更新状态
     * @param state 新状态
     */
    void updateState(ConnectionState state);

    /**
     * @brief 发送原始数据
     * @param data 数据
     * @return true 成功；false 失败
     */
    bool sendRawData(const QByteArray& data);

    /**
     * @brief 尝试重连
     */
    void attemptReconnect();

    /**
     * @brief 检查命令超时
     */
    void checkCommandTimeout();

    /**
     * @brief 获取当前时间戳（微秒）
     * @return 时间戳
     */
    static std::uint64_t getCurrentTimestamp();

    // 成员变量
    QTcpSocket* socket_;                    ///< TCP Socket
    QTimer* heartbeat_timer_;               ///< 心跳发送定时器
    QTimer* heartbeat_timeout_timer_;       ///< 心跳超时检测定时器
    QTimer* command_timeout_timer_;         ///< 命令超时检测定时器
    QTimer* reconnect_timer_;               ///< 重连定时器

    QString server_ip_;                     ///< 服务器IP
    int server_port_;                       ///< 服务器端口
    QString client_id_;                     ///< 客户端ID

    ConnectionState state_;                 ///< 当前状态

    std::uint32_t connect_timeout_ms_;      ///< 连接超时
    std::uint32_t heartbeat_interval_ms_;   ///< 心跳间隔
    std::uint32_t heartbeat_timeout_ms_;    ///< 心跳超时
    std::uint32_t command_timeout_ms_;      ///< 命令超时
    std::uint32_t reconnect_interval_ms_;   ///< 重连间隔
    std::uint32_t max_reconnect_count_;     ///< 最大重连次数
    std::uint32_t current_reconnect_count_; ///< 当前重连次数
    bool auto_reconnect_enabled_;           ///< 自动重连启用标志

    QByteArray recv_buffer_;                ///< 接收缓冲区

    mutable QMutex stats_mutex_;            ///< 统计互斥锁
    HandlerStats stats_;                    ///< 统计信息

    std::uint64_t last_heartbeat_send_time_; ///< 最后心跳发送时间
    std::uint64_t last_heartbeat_recv_time_; ///< 最后心跳接收时间

    // 待响应命令列表
    QList<PendingCommand> pending_commands_;
};

} // namespace ground_station

#endif // GROUND_STATION_TCPHANDLER_H