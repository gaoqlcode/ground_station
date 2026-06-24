/**
 * @file streamclient.cpp
 * @brief 流客户端核心类实现
 */

#include "streamclient.h"
#include <QDataStream>
#include <QCoreApplication>

namespace ground_station {

/**
 * @brief 构造函数
 *
 * 初始化客户端组件，包括TCP/UDP Socket、定时器和状态机
 *
 * @param parent 父对象指针
 */
StreamClient::StreamClient(QObject* parent)
    : QObject(parent)
    , m_tcpSocket(new QTcpSocket(this))
    , m_udpSocket(new QUdpSocket(this))
    , m_heartbeatTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
    , m_totalBytesReceived(0)
    , m_totalBytesSent(0)
    , m_reconnectAttempts(0)
    , m_initialized(false)
    , m_running(false) {

    initializeModules();
    setupSignalConnections();
}

/**
 * @brief 析构函数
 *
 * 停止客户端并清理资源
 */
StreamClient::~StreamClient() {
    stop();
    LOG_INFO("StreamClient", "StreamClient 已销毁");
}

/**
 * @brief 初始化模块
 *
 * 创建状态机和流管理器实例
 */
void StreamClient::initializeModules() {
    // 创建状态机
    m_stateMachine = std::make_unique<StateMachine>(this);

    // 创建流管理器
    m_streamManager = std::make_unique<StreamManager>(m_streamConfig.maxStreamCount, this);

    LOG_INFO("StreamClient", "模块初始化完成");
}

/**
 * @brief 设置信号连接
 *
 * 建立TCP/UDP Socket、状态机、流管理器和定时器之间的信号连接
 */
void StreamClient::setupSignalConnections() {
    // TCP连接信号
    connect(m_tcpSocket, &QTcpSocket::connected, this, &StreamClient::onTcpConnected);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &StreamClient::onTcpDisconnected);
    connect(m_tcpSocket, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
            this, &StreamClient::onTcpError);
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &StreamClient::onTcpDataReceived);

    // UDP数据接收信号
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &StreamClient::onUdpDataReceived);

    // 状态机信号
    connect(m_stateMachine.get(), &StateMachine::stateChanged,
            this, &StreamClient::onStateChanged);
    connect(m_stateMachine.get(), &StateMachine::stateTimeout,
            this, &StreamClient::onStateTimeout);

    // 流管理器信号
    connect(m_streamManager.get(), &StreamManager::streamRegistered,
            this, &StreamClient::streamRegistered);
    connect(m_streamManager.get(), &StreamManager::streamSwitched,
            this, [this](int oldId, int newId) {
                emit streamSwitched(newId);
            });

    // 定时器信号
    connect(m_heartbeatTimer, &QTimer::timeout, this, &StreamClient::onHeartbeatTimer);
    connect(m_reconnectTimer, &QTimer::timeout, this, &StreamClient::onReconnectTimer);

    LOG_INFO("StreamClient", "信号连接已建立");
}

bool StreamClient::initialize(const QString& configFile) {
    if (m_initialized) {
        LOG_WARN("StreamClient", "客户端已初始化");
        return true;
    }

    // 加载配置
    if (!configFile.isEmpty()) {
        Config::instance().loadFromFile(configFile);
    }

    m_serverConfig = Config::instance().getServerConfig();
    m_bufferConfig = Config::instance().getBufferConfig();
    m_streamConfig = Config::instance().getStreamConfig();

    // 初始化日志系统
    LogConfig logConfig = Config::instance().getLogConfig();
    Logger::instance().initialize(logConfig.logFilePath,
                                   static_cast<LogLevel>(logConfig.logLevel),
                                   logConfig.enableConsole);

    // 设置状态机超时
    m_stateMachine->setStateTimeout(ClientState::Connecting, m_serverConfig.connectionTimeout);
    m_stateMachine->setStateTimeout(ClientState::Registering, m_streamConfig.registrationTimeout);

    // 设置流管理器参数
    m_streamManager->setStreamTimeout(m_streamConfig.streamTimeout);

    m_initialized = true;
    LOG_INFO("StreamClient", "客户端初始化成功");

    return true;
}

bool StreamClient::start() {
    if (!m_initialized) {
        LOG_ERROR("StreamClient", "客户端未初始化");
        return false;
    }

    if (m_running) {
        LOG_WARN("StreamClient", "客户端已在运行");
        return true;
    }

    m_running = true;
    m_stateMachine->start();

    LOG_INFO("StreamClient", "客户端已启动");
    return true;
}

void StreamClient::stop() {
    if (!m_running) {
        return;
    }

    disconnectFromServer();
    stopReconnect();

    m_heartbeatTimer->stop();
    m_streamManager->clearAllStreams();
    m_stateMachine->stop();

    m_running = false;
    LOG_INFO("StreamClient", "客户端已停止");
}

/**
 * @brief 连接到服务器
 *
 * 发起TCP连接到服务器
 *
 * @return 成功发起连接返回true，失败返回false
 */
bool StreamClient::connectToServer() {
    if (!m_running) {
        LOG_ERROR("StreamClient", "客户端未运行");
        return false;
    }

    if (!m_stateMachine->requestStateTransition(ClientState::Connecting)) {
        return false;
    }

    LOG_INFO("StreamClient", QString("正在连接服务器: %1:%2")
                                .arg(m_serverConfig.serverAddress)
                                .arg(m_serverConfig.commandPort));

    m_tcpSocket->connectToHost(m_serverConfig.serverAddress, m_serverConfig.commandPort);

    return true;
}

/**
 * @brief 断开与服务器的连接
 *
 * 关闭TCP和UDP连接，状态转换为断开
 */
void StreamClient::disconnectFromServer() {
    if (m_tcpSocket->state() != QAbstractSocket::UnconnectedState) {
        m_tcpSocket->disconnectFromHost();
    }

    if (m_udpSocket->state() != QAbstractSocket::UnconnectedState) {
        m_udpSocket->close();
    }

    m_stateMachine->requestStateTransition(ClientState::Disconnected);
    LOG_INFO("StreamClient", "已断开服务器连接");
}

/**
 * @brief 注册视频流
 *
 * 向服务器注册一个新的视频流
 *
 * @param streamName 流名称
 * @return 成功返回流ID，失败返回-1
 */
int StreamClient::registerStream(const QString& streamName) {
    if (getCurrentState() != ClientState::Connected &&
        getCurrentState() != ClientState::Streaming) {
        LOG_ERROR("StreamClient", "无法注册流: 未连接");
        return -1;
    }

    // 请求状态转换到Registering
    if (!m_stateMachine->requestStateTransition(ClientState::Registering)) {
        return -1;
    }

    // 注册流
    int streamId = m_streamManager->registerStream(streamName);
    if (streamId < 0) {
        m_stateMachine->requestStateTransition(ClientState::Connected);
        return -1;
    }

    // 发送注册请求
    sendRegistrationRequest(streamName);

    LOG_INFO("StreamClient", QString("流注册请求已发送: ID=%1, 名称=%2")
                                .arg(streamId)
                                .arg(streamName));

    return streamId;
}

/**
 * @brief 注销视频流
 *
 * 从服务器注销指定的视频流
 *
 * @param streamId 流ID
 * @return 成功返回true，失败返回false
 */
bool StreamClient::unregisterStream(int streamId) {
    if (!m_streamManager->isStreamValid(streamId)) {
        LOG_ERROR("StreamClient", QString("无效的流ID: %1").arg(streamId));
        return false;
    }

    bool result = m_streamManager->unregisterStream(streamId);

    if (result) {
        LOG_INFO("StreamClient", QString("流已注销: ID=%1").arg(streamId));
    }

    return result;
}

/**
 * @brief 切换视频流
 *
 * 切换到指定的视频流
 *
 * @param streamId 目标流ID
 * @return 成功返回true，失败返回false
 */
bool StreamClient::switchStream(int streamId) {
    if (!m_streamManager->isStreamValid(streamId)) {
        LOG_ERROR("StreamClient", QString("无效的流ID: %1").arg(streamId));
        return false;
    }

    return m_streamManager->switchToStream(streamId);
}

ClientState StreamClient::getCurrentState() const {
    return m_stateMachine->getCurrentState();
}

/**
 * @brief 获取当前流ID
 *
 * @return 当前活动流的ID
 */
int StreamClient::getCurrentStreamId() const {
    return m_streamManager->getCurrentStreamId();
}

bool StreamClient::sendData(const QByteArray& data) {
    if (m_tcpSocket->state() != QAbstractSocket::ConnectedState) {
        LOG_ERROR("StreamClient", "无法发送数据: 未连接");
        return false;
    }

    qint64 bytesWritten = m_tcpSocket->write(data);
    if (bytesWritten == -1) {
        LOG_ERROR("StreamClient", "数据发送失败");
        return false;
    }

    m_totalBytesSent += bytesWritten;
    LOG_DEBUG("StreamClient", QString("已发送数据: %1 字节").arg(bytesWritten));

    return true;
}

QString StreamClient::getStatistics() const {
    return QString("State: %1, Current Stream: %2, "
                   "Bytes Received: %3, Bytes Sent: %4, "
                   "Reconnect Attempts: %5")
        .arg(m_stateMachine->getStateName(getCurrentState()))
        .arg(getCurrentStreamId())
        .arg(m_totalBytesReceived)
        .arg(m_totalBytesSent)
        .arg(m_reconnectAttempts);
}

/**
 * @brief 重连服务器
 *
 * 尝试重新连接到服务器
 *
 * @return 成功发起重连返回true，失败返回false
 */
bool StreamClient::reconnect() {
    if (m_reconnectAttempts >= m_serverConfig.maxReconnectAttempts) {
        LOG_ERROR("StreamClient", "已达到最大重连次数");
        return false;
    }

    m_reconnectAttempts++;
    LOG_INFO("StreamClient", QString("正在尝试重连 (%1/%2)")
                                .arg(m_reconnectAttempts)
                                .arg(m_serverConfig.maxReconnectAttempts));

    return connectToServer();
}

/**
 * @brief TCP连接成功槽函数
 *
 * 处理TCP连接建立成功事件
 */
void StreamClient::onTcpConnected() {
    LOG_INFO("StreamClient", "TCP连接已建立");

    // 状态转换到Connected
    m_stateMachine->requestStateTransition(ClientState::Connected);

    // 绑定UDP socket
    if (!m_udpSocket->bind(QHostAddress::Any, m_serverConfig.dataPort)) {
        LOG_ERROR("StreamClient", "UDP Socket绑定失败");
    } else {
        LOG_INFO("StreamClient", QString("UDP Socket已绑定到端口 %1")
                                    .arg(m_serverConfig.dataPort));
    }

    // 启动心跳定时器
    m_heartbeatTimer->start(m_streamConfig.heartbeatInterval);

    // 重置重连计数
    m_reconnectAttempts = 0;
    stopReconnect();

    emit connected();
}

/**
 * @brief TCP断开连接槽函数
 *
 * 处理TCP连接断开事件，启动重连机制
 */
void StreamClient::onTcpDisconnected() {
    LOG_WARN("StreamClient", "TCP连接已断开");

    // 停止心跳
    m_heartbeatTimer->stop();

    // 状态转换到Disconnected
    m_stateMachine->requestStateTransition(ClientState::Disconnected);

    // 清除所有流
    m_streamManager->clearAllStreams();

    emit disconnected();

    // 开始重连
    if (m_running) {
        startReconnect();
    }
}

/**
 * @brief TCP错误槽函数
 *
 * 处理TCP Socket错误
 *
 * @param error Socket错误类型
 */
void StreamClient::onTcpError(QAbstractSocket::SocketError error) {
    QString errorMsg = QString("TCP socket error: %1 - %2")
                           .arg(error)
                           .arg(m_tcpSocket->errorString());

    LOG_ERROR("StreamClient", errorMsg);

    m_stateMachine->requestStateTransition(ClientState::Error);
    emit connectionFailed(errorMsg);
    emit errorOccurred(errorMsg);
}

/**
 * @brief TCP数据接收槽函数
 *
 * 处理接收到的TCP数据
 */
void StreamClient::onTcpDataReceived() {
    QByteArray data = m_tcpSocket->readAll();
    m_totalBytesReceived += data.size();

    LOG_DEBUG("StreamClient", QString("接收到TCP数据: %1 字节").arg(data.size()));

    handleCommand(data);
}

/**
 * @brief UDP数据接收槽函数
 *
 * 处理接收到的UDP视频流数据
 */
void StreamClient::onUdpDataReceived() {
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray data;
        data.resize(m_udpSocket->pendingDatagramSize());

        QHostAddress sender;
        quint16 senderPort;

        m_udpSocket->readDatagram(data.data(), data.size(), &sender, &senderPort);

        m_totalBytesReceived += data.size();

        LOG_DEBUG("StreamClient", QString("接收到UDP数据: %1 字节 from %2:%3")
                                      .arg(data.size())
                                      .arg(sender.toString())
                                      .arg(senderPort));

        handleStreamData(data);
    }
}

/**
 * @brief 状态变更槽函数
 *
 * 处理状态机状态变更事件
 *
 * @param oldState 旧状态
 * @param newState 新状态
 */
void StreamClient::onStateChanged(ClientState oldState, ClientState newState) {
    LOG_INFO("StreamClient", QString("状态变更: %1 -> %2")
                                .arg(m_stateMachine->getStateName(oldState))
                                .arg(m_stateMachine->getStateName(newState)));

    emit stateChanged(newState);
}

/**
 * @brief 状态超时槽函数
 *
 * 处理状态机状态超时事件
 *
 * @param state 超时的状态
 */
void StreamClient::onStateTimeout(ClientState state) {
    LOG_WARN("StreamClient", QString("状态超时: %1")
                                  .arg(m_stateMachine->getStateName(state)));

    if (state == ClientState::Connecting) {
        emit connectionFailed("连接超时");
        startReconnect();
    } else if (state == ClientState::Registering) {
        emit errorOccurred("注册超时");
    }
}

/**
 * @brief 心跳定时器槽函数
 *
 * 定期发送心跳包保持连接
 */
void StreamClient::onHeartbeatTimer() {
    sendHeartbeat();
}

/**
 * @brief 重连定时器槽函数
 *
 * 定期尝试重新连接服务器
 */
void StreamClient::onReconnectTimer() {
    reconnect();
}

void StreamClient::sendRegistrationRequest(const QString& streamName) {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    // 命令类型：注册请求
    stream << quint8(0x01);  // CMD_REGISTER
    stream << streamName;

    sendData(data);

    LOG_DEBUG("StreamClient", QString("注册请求已发送: %1").arg(streamName));
}

void StreamClient::sendHeartbeat() {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    // 命令类型：心跳
    stream << quint8(0x00);  // CMD_HEARTBEAT

    sendData(data);

    LOG_DEBUG("StreamClient", "心跳已发送");
}

/**
 * @brief 处理命令
 *
 * 解析并处理从服务器接收到的命令
 *
 * @param data 命令数据
 */
void StreamClient::handleCommand(const QByteArray& data) {
    QDataStream stream(data);

    quint8 cmdType;
    stream >> cmdType;

    switch (cmdType) {
        case 0x02:  // CMD_REGISTER_RESPONSE
        {
            quint8 result;
            stream >> result;

            if (result == 0x00) {  // 注册成功
                LOG_INFO("StreamClient", "流注册成功");

                // 激活流
                int streamId = m_streamManager->getCurrentStreamId();
                if (streamId >= 0) {
                    m_streamManager->activateStream(streamId);
                }

                // 状态转换到Streaming
                m_stateMachine->requestStateTransition(ClientState::Streaming);
            } else {
                LOG_ERROR("StreamClient", "流注册失败");
                m_stateMachine->requestStateTransition(ClientState::Connected);
            }
            break;
        }

        case 0x03:  // CMD_STREAM_SWITCH
        {
            int streamId;
            stream >> streamId;

            LOG_INFO("StreamClient", QString("收到流切换命令: ID=%1").arg(streamId));

            if (m_streamManager->switchToStream(streamId)) {
                // 发送确认
                QByteArray ack;
                QDataStream ackStream(&ack, QIODevice::WriteOnly);
                ackStream << quint8(0x04);  // CMD_STREAM_SWITCH_ACK
                ackStream << streamId;
                sendData(ack);
            }
            break;
        }

        default:
            LOG_WARN("StreamClient", QString("未知命令类型: %1").arg(cmdType));
            break;
    }
}

/**
 * @brief 处理流数据
 *
 * 处理接收到的UDP视频流数据
 *
 * @param data 流数据
 */
void StreamClient::handleStreamData(const QByteArray& data) {
    int currentStreamId = m_streamManager->getCurrentStreamId();

    if (currentStreamId < 0) {
        LOG_WARN("StreamClient", "无活动流接收数据");
        return;
    }

    // 更新流数据统计
    m_streamManager->updateStreamData(currentStreamId, data.size());

    // 发送数据接收信号
    emit dataReceived(currentStreamId, data);
}

void StreamClient::startReconnect() {
    if (m_reconnectTimer->isActive()) {
        return;
    }

    LOG_INFO("StreamClient", QString("启动重连定时器 (间隔: %1ms)")
                                .arg(m_serverConfig.reconnectInterval));

    m_reconnectTimer->start(m_serverConfig.reconnectInterval);
}

/**
 * @brief 停止重连
 *
 * 停止重连定时器
 */
void StreamClient::stopReconnect() {
    if (m_reconnectTimer->isActive()) {
        m_reconnectTimer->stop();
        LOG_INFO("StreamClient", "重连定时器已停止");
    }
}

}  // namespace ground_station