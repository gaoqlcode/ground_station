/**
 * @file tcphandler.cpp
 * @brief TCP命令处理器实现
 *
 * 实现TCP客户端通信功能，包括：
 * - TCP连接管理（连接、断开、重连）
 * - 心跳机制（定时发送心跳、超时检测）
 * - 命令发送与响应处理（同步/异步）
 * - 状态机管理（Disconnected -> Connecting -> Connected -> Registered -> Active）
 * - 自动重连机制
 *
 * 状态机：
 * Disconnected -> Connecting -> Connected -> Registered -> Active
 *      ^            |              |               |          |
 *      |            +--------------+---------------+----------+
 *      +------------------------------------------------------+ (断开/超时)
 *
 * 命令协议：
 * - 文本协议，每行一条命令
 * - 支持 Ping/Register/Subscribe/Unsubscribe/Switch/List 等命令
 */

#include "tcphandler.h"

#include <QDebug>
#include <QDateTime>
#include <QEventLoop>
#include <QMutexLocker>

namespace ground_station {

// ============================================================================
// TcpHandler 实现
// ============================================================================

/**
 * @brief 默认构造函数
 *
 * 使用默认端口创建TCP处理器。
 *
 * @param parent 父对象指针
 */
TcpHandler::TcpHandler(QObject* parent)
    : QObject(parent)
    , socket_(nullptr)
    , heartbeat_timer_(nullptr)
    , heartbeat_timeout_timer_(nullptr)
    , command_timeout_timer_(nullptr)
    , reconnect_timer_(nullptr)
    , server_port_(DEFAULT_TCP_PORT)
    , state_(ConnectionState::Disconnected)
    , connect_timeout_ms_(DEFAULT_CONNECT_TIMEOUT_MS)
    , heartbeat_interval_ms_(DEFAULT_HEARTBEAT_INTERVAL_MS)
    , heartbeat_timeout_ms_(DEFAULT_HEARTBEAT_TIMEOUT_MS)
    , command_timeout_ms_(DEFAULT_CMD_TIMEOUT_MS)
    , reconnect_interval_ms_(DEFAULT_RECONNECT_INTERVAL_MS)
    , max_reconnect_count_(MAX_RECONNECT_COUNT)
    , current_reconnect_count_(0)
    , auto_reconnect_enabled_(true)
    , last_heartbeat_send_time_(0)
    , last_heartbeat_recv_time_(0)
{
    initialize();
}

/**
 * @brief 带参数构造函数
 *
 * 指定服务器地址和端口创建TCP处理器。
 *
 * @param server_ip 服务器IP地址
 * @param server_port 服务器端口
 * @param parent 父对象指针
 */
TcpHandler::TcpHandler(const QString& server_ip, int server_port, QObject* parent)
    : QObject(parent)
    , socket_(nullptr)
    , heartbeat_timer_(nullptr)
    , heartbeat_timeout_timer_(nullptr)
    , command_timeout_timer_(nullptr)
    , reconnect_timer_(nullptr)
    , server_ip_(server_ip)
    , server_port_(server_port)
    , state_(ConnectionState::Disconnected)
    , connect_timeout_ms_(DEFAULT_CONNECT_TIMEOUT_MS)
    , heartbeat_interval_ms_(DEFAULT_HEARTBEAT_INTERVAL_MS)
    , heartbeat_timeout_ms_(DEFAULT_HEARTBEAT_TIMEOUT_MS)
    , command_timeout_ms_(DEFAULT_CMD_TIMEOUT_MS)
    , reconnect_interval_ms_(DEFAULT_RECONNECT_INTERVAL_MS)
    , max_reconnect_count_(MAX_RECONNECT_COUNT)
    , current_reconnect_count_(0)
    , auto_reconnect_enabled_(true)
    , last_heartbeat_send_time_(0)
    , last_heartbeat_recv_time_(0)
{
    initialize();
}

/**
 * @brief 析构函数
 *
 * 确保正确释放所有资源，包括Socket和定时器。
 */
TcpHandler::~TcpHandler()
{
    disconnectFromServer();
    
    if (heartbeat_timer_) {
        delete heartbeat_timer_;
        heartbeat_timer_ = nullptr;
    }
    
    if (heartbeat_timeout_timer_) {
        delete heartbeat_timeout_timer_;
        heartbeat_timeout_timer_ = nullptr;
    }
    
    if (command_timeout_timer_) {
        delete command_timeout_timer_;
        command_timeout_timer_ = nullptr;
    }
    
    if (reconnect_timer_) {
        delete reconnect_timer_;
        reconnect_timer_ = nullptr;
    }
    
    if (socket_) {
        delete socket_;
        socket_ = nullptr;
    }
}

void TcpHandler::initialize()
{
    // 创建TCP Socket
    socket_ = new QTcpSocket(this);
    
    // 创建定时器
    heartbeat_timer_ = new QTimer(this);
    heartbeat_timeout_timer_ = new QTimer(this);
    command_timeout_timer_ = new QTimer(this);
    reconnect_timer_ = new QTimer(this);
    
    // 连接Socket信号
    connect(socket_, &QTcpSocket::connected,
            this, &TcpHandler::onConnected);
    
    connect(socket_, &QTcpSocket::disconnected,
            this, &TcpHandler::onDisconnected);
    
    connect(socket_, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
            this, &TcpHandler::onSocketError);
    
    connect(socket_, &QTcpSocket::readyRead,
            this, &TcpHandler::onReadyRead);
    
    // 连接定时器信号
    connect(heartbeat_timer_, &QTimer::timeout,
            this, &TcpHandler::onHeartbeatTimer);
    
    connect(heartbeat_timeout_timer_, &QTimer::timeout,
            this, &TcpHandler::onHeartbeatTimeoutCheck);
    
    connect(command_timeout_timer_, &QTimer::timeout,
            this, &TcpHandler::onCommandTimeoutCheck);
    
    connect(reconnect_timer_, &QTimer::timeout,
            this, &TcpHandler::onReconnectTimer);
    
    // 设置定时器间隔
    heartbeat_timer_->setInterval(static_cast<int>(heartbeat_interval_ms_));
    heartbeat_timeout_timer_->setInterval(static_cast<int>(heartbeat_timeout_ms_));
    command_timeout_timer_->setInterval(500);  // 每500ms检查一次命令超时
    reconnect_timer_->setInterval(static_cast<int>(reconnect_interval_ms_));
    
    // 初始化心跳接收时间
    last_heartbeat_recv_time_ = getCurrentTimestamp();
}

void TcpHandler::setServerAddress(const QString& ip, int port)
{
    server_ip_ = ip;
    server_port_ = port;
}

QString TcpHandler::getServerIp() const
{
    return server_ip_;
}

int TcpHandler::getServerPort() const
{
    return server_port_;
}

void TcpHandler::setConnectTimeout(std::uint32_t timeout_ms)
{
    connect_timeout_ms_ = timeout_ms;
}

void TcpHandler::setHeartbeatInterval(std::uint32_t interval_ms)
{
    heartbeat_interval_ms_ = interval_ms;
    heartbeat_timer_->setInterval(static_cast<int>(interval_ms));
}

void TcpHandler::setHeartbeatTimeout(std::uint32_t timeout_ms)
{
    heartbeat_timeout_ms_ = timeout_ms;
}

void TcpHandler::setCommandTimeout(std::uint32_t timeout_ms)
{
    command_timeout_ms_ = timeout_ms;
}

void TcpHandler::setReconnectInterval(std::uint32_t interval_ms)
{
    reconnect_interval_ms_ = interval_ms;
    reconnect_timer_->setInterval(static_cast<int>(interval_ms));
}

void TcpHandler::setAutoReconnectEnabled(bool enabled)
{
    auto_reconnect_enabled_ = enabled;
}

void TcpHandler::setMaxReconnectCount(std::uint32_t count)
{
    max_reconnect_count_ = count;
}

void TcpHandler::setClientId(const QString& client_id)
{
    client_id_ = client_id;
}

QString TcpHandler::getClientId() const
{
    return client_id_;
}

bool TcpHandler::connectToServer()
{
    if (state_ == ConnectionState::Connected || state_ == ConnectionState::Registered) {
        return true;
    }
    
    if (state_ == ConnectionState::Connecting) {
        return false;
    }
    
    updateState(ConnectionState::Connecting);
    
    // 重置重连计数
    current_reconnect_count_ = 0;
    
    // 连接服务器
    socket_->connectToHost(server_ip_, server_port_);
    
    // 等待连接
    if (!socket_->waitForConnected(static_cast<int>(connect_timeout_ms_))) {
        QString error_msg = QString("连接服务器超时: %1:%2")
                            .arg(server_ip_)
                            .arg(server_port_);
        emit errorOccurred(HandlerError::ConnectionError, error_msg);
        
        {
            QMutexLocker locker(&stats_mutex_);
            stats_.last_error = error_msg;
            stats_.errors++;
        }
        
        updateState(ConnectionState::Error);
        
        // 尝试重连
        if (auto_reconnect_enabled_) {
            attemptReconnect();
        }
        
        return false;
    }
    
    return true;
}

void TcpHandler::disconnectFromServer()
{
    // 停止所有定时器
    heartbeat_timer_->stop();
    heartbeat_timeout_timer_->stop();
    command_timeout_timer_->stop();
    reconnect_timer_->stop();
    
    // 断开Socket
    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->disconnectFromHost();
    }
    
    // 清空缓冲区
    recv_buffer_.clear();
    pending_commands_.clear();
    
    updateState(ConnectionState::Disconnected);
}

bool TcpHandler::isConnected() const
{
    return state_ == ConnectionState::Connected || 
           state_ == ConnectionState::Registered ||
           state_ == ConnectionState::Active;
}

bool TcpHandler::isRegistered() const
{
    return state_ == ConnectionState::Registered || state_ == ConnectionState::Active;
}

ConnectionState TcpHandler::getState() const
{
    return state_;
}

bool TcpHandler::sendCommand(const QString& command)
{
    if (!isConnected()) {
        emit errorOccurred(HandlerError::ConnectionError, "Not connected to server");
        return false;
    }
    
    QString cmd = command.trimmed();
    if (!cmd.endsWith('\n')) {
        cmd += '\n';
    }
    
    QByteArray data = cmd.toUtf8();
    
    if (!sendRawData(data)) {
        return false;
    }
    
    // 记录命令
    PendingCommand pending;
    pending.command_name = ProtocolParser::parseCommand(cmd).name;
    pending.raw_command = data;
    pending.send_time = getCurrentTimestamp();
    pending.timeout_ms = command_timeout_ms_;
    pending_commands_.append(pending);
    
    {
        QMutexLocker locker(&stats_mutex_);
        stats_.commands_sent++;
        stats_.last_command = cmd.trimmed();
    }
    
    emit commandSent(cmd.trimmed());
    
    return true;
}

bool TcpHandler::sendCommand(CommandType type, const QStringList& args)
{
    QString cmd = ProtocolParser::buildCommand(type, args);
    return sendCommand(cmd);
}

Response TcpHandler::sendCommandAndWait(const QString& command, std::uint32_t timeout_ms)
{
    Response response;
    
    if (!isConnected()) {
        response.status = ResponseStatus::Error;
        response.message = "未连接到服务器";
        return response;
    }
    
    // 发送命令
    if (!sendCommand(command)) {
        response.status = ResponseStatus::Error;
        response.message = "发送命令失败";
        return response;
    }
    
    // 等待响应
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(static_cast<int>(timeout_ms));
    
    connect(this, &TcpHandler::responseReceived, &loop, [&](const Response& resp) {
        Command cmd = ProtocolParser::parseCommand(command);
        if (resp.command_name.toLower() == cmd.name.toLower()) {
            response = resp;
            loop.quit();
        }
    });
    
    connect(&timer, &QTimer::timeout, &loop, [&]() {
        response.status = ResponseStatus::Error;
        response.message = "命令超时";
        loop.quit();
    });
    
    timer.start();
    loop.exec();
    
    return response;
}

Response TcpHandler::sendCommandAndWait(CommandType type, const QStringList& args, std::uint32_t timeout_ms)
{
    QString cmd = ProtocolParser::buildCommand(type, args);
    return sendCommandAndWait(cmd, timeout_ms);
}

bool TcpHandler::sendHeartbeat()
{
    return sendCommand(CommandType::Ping);
}

bool TcpHandler::registerClient(const QString& user_agent)
{
    QStringList args;
    if (!client_id_.isEmpty()) {
        args.append(client_id_);
    }
    args.append(user_agent);
    
    Response response = sendCommandAndWait(CommandType::StreamRegister, args);
    
    if (response.isSuccess()) {
        RegisterResponse reg_resp = ProtocolParser::parseRegisterResponse(response);
        client_id_ = reg_resp.client_id;
        updateState(ConnectionState::Registered);
        return true;
    }
    
    return false;
}

bool TcpHandler::subscribeStream(const QString& stream_id)
{
    Response response = sendCommandAndWait(CommandType::StreamSubscribe, QStringList{stream_id});
    return response.isSuccess();
}

bool TcpHandler::unsubscribeStream(const QString& stream_id)
{
    Response response = sendCommandAndWait(CommandType::StreamUnsubscribe, QStringList{stream_id});
    return response.isSuccess();
}

bool TcpHandler::switchStream(const QString& stream_id)
{
    Response response = sendCommandAndWait(CommandType::StreamSwitch, QStringList{stream_id});
    return response.isSuccess();
}

QList<StreamInfo> TcpHandler::getStreamList()
{
    Response response = sendCommandAndWait(CommandType::StreamList);
    
    if (response.isSuccess()) {
        return ProtocolParser::parseStreamList(response.data);
    }
    
    return QList<StreamInfo>();
}

QList<CameraInfo> TcpHandler::getCameraList()
{
    Response response = sendCommandAndWait(CommandType::DiagStatus);
    
    if (response.isSuccess() && response.data.contains("cameras")) {
        return ProtocolParser::parseCameraList(response.data);
    }
    
    return QList<CameraInfo>();
}

HandlerStats TcpHandler::getStats() const
{
    QMutexLocker locker(&stats_mutex_);
    return stats_;
}

void TcpHandler::resetStats()
{
    QMutexLocker locker(&stats_mutex_);
    stats_ = HandlerStats();
}

QTcpSocket* TcpHandler::getSocket()
{
    return socket_;
}

void TcpHandler::onConnected()
{
    qDebug() << "Connected to server" << server_ip_ << ":" << server_port_;
    
    updateState(ConnectionState::Connected);
    
    // 启动心跳定时器
    heartbeat_timer_->start();
    heartbeat_timeout_timer_->start();
    
    // 停止重连定时器
    reconnect_timer_->stop();
    current_reconnect_count_ = 0;
    
    // 清空接收缓冲区
    recv_buffer_.clear();
}

void TcpHandler::onDisconnected()
{
    qDebug() << "已断开服务器连接";
    
    // 停止心跳定时器
    heartbeat_timer_->stop();
    heartbeat_timeout_timer_->stop();
    
    updateState(ConnectionState::Disconnected);
    
    // 尝试重连
    if (auto_reconnect_enabled_) {
        attemptReconnect();
    }
}

void TcpHandler::onSocketError(QAbstractSocket::SocketError error)
{
    QString error_msg = QString("Socket error: %1 (%2)")
                        .arg(static_cast<int>(error))
                        .arg(socket_->errorString());
    
    emit errorOccurred(HandlerError::SocketError, error_msg);
    
    {
        QMutexLocker locker(&stats_mutex_);
        stats_.last_error = error_msg;
        stats_.errors++;
    }
}

void TcpHandler::onReadyRead()
{
    processReceivedData();
}

void TcpHandler::onHeartbeatTimer()
{
    if (isConnected()) {
        sendHeartbeat();
        
        {
            QMutexLocker locker(&stats_mutex_);
            stats_.heartbeats_sent++;
        }
        
        emit heartbeatSent();
    }
}

void TcpHandler::onHeartbeatTimeoutCheck()
{
    std::uint64_t current_time = getCurrentTimestamp();
    
    if (current_time - last_heartbeat_recv_time_ > heartbeat_timeout_ms_ * 1000) {
        QString error_msg = "Heartbeat timeout";
        
        emit errorOccurred(HandlerError::HeartbeatTimeout, error_msg);
        
        {
            QMutexLocker locker(&stats_mutex_);
            stats_.last_error = error_msg;
            stats_.errors++;
        }
        
        updateState(ConnectionState::Inactive);
        
        // 断开连接并尝试重连
        disconnectFromServer();
        
        if (auto_reconnect_enabled_) {
            attemptReconnect();
        }
    }
}

void TcpHandler::onCommandTimeoutCheck()
{
    checkCommandTimeout();
}

void TcpHandler::onReconnectTimer()
{
    if (current_reconnect_count_ < max_reconnect_count_) {
        current_reconnect_count_++;
        
        emit reconnectAttempt(current_reconnect_count_, max_reconnect_count_);
        
        {
            QMutexLocker locker(&stats_mutex_);
            stats_.reconnect_count++;
        }
        
        qDebug() << "正在尝试重连" << current_reconnect_count_ << "/" << max_reconnect_count_;
        
        socket_->connectToHost(server_ip_, server_port_);
    } else {
        reconnect_timer_->stop();
        
        QString error_msg = QString("Max reconnect attempts (%1) reached")
                            .arg(max_reconnect_count_);
        
        emit errorOccurred(HandlerError::ReconnectFailed, error_msg);
        
        {
            QMutexLocker locker(&stats_mutex_);
            stats_.last_error = error_msg;
            stats_.errors++;
        }
        
        updateState(ConnectionState::Error);
    }
}

void TcpHandler::processReceivedData()
{
    // 读取所有可用数据
    recv_buffer_.append(socket_->readAll());
    
    // 提取完整消息
    QByteArray message;
    while (ProtocolParser::extractMessage(recv_buffer_, message) > 0) {
        // 解析响应
        Response response = ProtocolParser::parseResponse(message);
        
        if (response.isValid()) {
            handleResponse(response);
        }
    }
}

void TcpHandler::handleResponse(const Response& response)
{
    {
        QMutexLocker locker(&stats_mutex_);
        stats_.responses_received++;
        
        // 计算响应时间
        for (const auto& pending : pending_commands_) {
            if (pending.command_name.toLower() == response.command_name.toLower() && pending.waiting_response) {
                std::uint64_t response_time = getCurrentTimestamp() - pending.send_time;
                stats_.avg_response_time_ms = (stats_.avg_response_time_ms * (stats_.responses_received - 1) +
                                               response_time / 1000.0) / stats_.responses_received;
                break;
            }
        }
    }
    
    // 处理心跳响应
    if (response.command_name.toLower() == "ping") {
        handleHeartbeatResponse(response);
    }
    
    // 发送响应信号
    emit responseReceived(response);
}

void TcpHandler::handleHeartbeatResponse(const Response& response)
{
    if (response.isSuccess()) {
        last_heartbeat_recv_time_ = getCurrentTimestamp();
        
        {
            QMutexLocker locker(&stats_mutex_);
            stats_.heartbeats_received++;
        }
        
        emit heartbeatReceived();
        
        // 更新状态为活跃
        if (state_ == ConnectionState::Registered) {
            updateState(ConnectionState::Active);
        }
    }
}

void TcpHandler::updateState(ConnectionState state)
{
    if (state_ != state) {
        state_ = state;
        emit stateChanged(state);
    }
}

bool TcpHandler::sendRawData(const QByteArray& data)
{
    if (socket_->state() != QAbstractSocket::ConnectedState) {
        return false;
    }
    
    qint64 written = socket_->write(data);
    
    if (written != data.size()) {
        QString error_msg = QString("Failed to write data: written %1 of %2 bytes")
                            .arg(written)
                            .arg(data.size());
        emit errorOccurred(HandlerError::SocketError, error_msg);
        return false;
    }
    
    socket_->flush();
    return true;
}

void TcpHandler::attemptReconnect()
{
    if (!reconnect_timer_->isActive()) {
        reconnect_timer_->start();
    }
}

void TcpHandler::checkCommandTimeout()
{
    std::uint64_t current_time = getCurrentTimestamp();
    
    // 检查待响应命令是否超时
    for (auto it = pending_commands_.begin(); it != pending_commands_.end();) {
        if (it->waiting_response && current_time - it->send_time > it->timeout_ms * 1000) {
            QString error_msg = QString("命令 '%1' 超时")
                                .arg(it->command_name);
            
            emit errorOccurred(HandlerError::TimeoutError, error_msg);
            
            {
                QMutexLocker locker(&stats_mutex_);
                stats_.last_error = error_msg;
                stats_.errors++;
            }
            
            it = pending_commands_.erase(it);
        } else {
            ++it;
        }
    }
}

std::uint64_t TcpHandler::getCurrentTimestamp()
{
    return static_cast<std::uint64_t>(QDateTime::currentMSecsSinceEpoch() * 1000);
}

} // namespace ground_station