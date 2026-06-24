/**
 * @file streamclient.h
 * @brief 流客户端核心类
 *
 * 整合所有模块的主类，提供完整的客户端功能
 */

#ifndef GROUND_STATION_STREAMCLIENT_H
#define GROUND_STATION_STREAMCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QByteArray>
#include <QHostAddress>
#include <memory>
#include "config.h"
#include "statemachine.h"
#include "streammanager.h"
#include "logger.h"

namespace ground_station {

/**
 * @class StreamClient
 * @brief 流客户端核心类
 *
 * 整合配置、状态机、流管理器等模块
 * 实现完整的客户端连接、注册、流传输流程
 */
class StreamClient : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit StreamClient(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~StreamClient() override;

    /**
     * @brief 初始化客户端
     * @param configFile 配置文件路径（可选）
     * @return 是否成功
     */
    bool initialize(const QString& configFile = "");

    /**
     * @brief 启动客户端
     * @return 是否成功
     */
    bool start();

    /**
     * @brief 停止客户端
     */
    void stop();

    /**
     * @brief 连接到服务器
     * @return 是否成功发起连接
     */
    bool connectToServer();

    /**
     * @brief 断开连接
     */
    void disconnectFromServer();

    /**
     * @brief 注册流
     * @param streamName 流名称
     * @return 流ID，-1表示失败
     */
    int registerStream(const QString& streamName);

    /**
     * @brief 注销流
     * @param streamId 流ID
     * @return 是否成功
     */
    bool unregisterStream(int streamId);

    /**
     * @brief 切换流
     * @param streamId 流ID
     * @return 是否成功
     */
    bool switchStream(int streamId);

    /**
     * @brief 获取当前状态
     * @return 当前状态
     */
    ClientState getCurrentState() const;

    /**
     * @brief 获取当前激活的流ID
     * @return 流ID
     */
    int getCurrentStreamId() const;

    /**
     * @brief 发送数据
     * @param data 数据内容
     * @return 是否成功
     */
    bool sendData(const QByteArray& data);

    /**
     * @brief 获取统计信息
     * @return 统计信息字符串
     */
    QString getStatistics() const;

    /**
     * @brief 重连到服务器
     * @return 是否成功发起重连
     */
    bool reconnect();

signals:
    /**
     * @brief 连接成功信号
     */
    void connected();

    /**
     * @brief 连接失败信号
     * @param errorMessage 错误消息
     */
    void connectionFailed(const QString& errorMessage);

    /**
     * @brief 断开连接信号
     */
    void disconnected();

    /**
     * @brief 状态变更信号
     * @param state 新状态
     */
    void stateChanged(ClientState state);

    /**
     * @brief 流注册成功信号
     * @param streamId 流ID
     * @param streamName 流名称
     */
    void streamRegistered(int streamId, const QString& streamName);

    /**
     * @brief 流切换信号
     * @param streamId 流ID
     */
    void streamSwitched(int streamId);

    /**
     * @brief 数据接收信号
     * @param streamId 流ID
     * @param data 接收的数据
     */
    void dataReceived(int streamId, const QByteArray& data);

    /**
     * @brief 错误信号
     * @param errorMessage 错误消息
     */
    void errorOccurred(const QString& errorMessage);

private slots:
    /**
     * @brief 处理TCP连接成功
     */
    void onTcpConnected();

    /**
     * @brief 处理TCP连接断开
     */
    void onTcpDisconnected();

    /**
     * @brief 处理TCP错误
     */
    void onTcpError(QAbstractSocket::SocketError error);

    /**
     * @brief 处理TCP数据接收
     */
    void onTcpDataReceived();

    /**
     * @brief 处理UDP数据接收
     */
    void onUdpDataReceived();

    /**
     * @brief 处理状态变更
     */
    void onStateChanged(ClientState oldState, ClientState newState);

    /**
     * @brief 处理状态超时
     */
    void onStateTimeout(ClientState state);

    /**
     * @brief 处理心跳定时器
     */
    void onHeartbeatTimer();

    /**
     * @brief 处理重连定时器
     */
    void onReconnectTimer();

private:
    /**
     * @brief 初始化内部模块
     */
    void initializeModules();

    /**
     * @brief 设置信号连接
     */
    void setupSignalConnections();

    /**
     * @brief 发送注册请求
     * @param streamName 流名称
     */
    void sendRegistrationRequest(const QString& streamName);

    /**
     * @brief 发送心跳包
     */
    void sendHeartbeat();

    /**
     * @brief 处理接收到的命令
     * @param data 命令数据
     */
    void handleCommand(const QByteArray& data);

    /**
     * @brief 处理接收到的流数据
     * @param data 流数据
     */
    void handleStreamData(const QByteArray& data);

    /**
     * @brief 开始重连流程
     */
    void startReconnect();

    /**
     * @brief 停止重连流程
     */
    void stopReconnect();

    // 核心模块
    std::unique_ptr<StateMachine> m_stateMachine;
    std::unique_ptr<StreamManager> m_streamManager;

    // 网络组件
    QTcpSocket* m_tcpSocket;
    QUdpSocket* m_udpSocket;

    // 定时器
    QTimer* m_heartbeatTimer;
    QTimer* m_reconnectTimer;

    // 配置参数
    ServerConfig m_serverConfig;
    BufferConfig m_bufferConfig;
    StreamConfig m_streamConfig;

    // 统计信息
    qint64 m_totalBytesReceived;
    qint64 m_totalBytesSent;
    int m_reconnectAttempts;

    // 运行状态
    bool m_initialized;
    bool m_running;
};

}  // namespace ground_station

#endif  // GROUND_STATION_STREAMCLIENT_H