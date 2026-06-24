/**
 * @file connectionwidget.cpp
 * @brief 连接管理后端实现
 *
 * 本文件实现了ConnectionWidget类，作为连接设置对话框的后端逻辑。
 * 主要功能包括：
 * - TCP/UDP连接管理
 * - 连接状态监控
 * - 数据收发统计
 * - 连接状态信号通知
 *
 * 该类是一个无界面控件，由ConnectionSettingsDialog对话框调用和操作。
 *
 * @author GroundStation Team
 * @date 2024
 */

#include "connectionwidget.h"

#include <QDateTime>
#include <QHostAddress>

namespace ground_station {

/**
 * @brief 构造函数
 *
 * 初始化连接管理控件，设置默认服务器地址和端口。
 * 该控件默认隐藏，因为它是纯后端逻辑组件。
 *
 * @param parent 父控件指针
 */
ConnectionWidget::ConnectionWidget(QWidget *parent)
    : QWidget(parent)
    , m_tcpSocket(nullptr)
    , m_udpSocket(nullptr)
    , m_serverAddress("127.0.0.1")       // 默认本地地址
    , m_serverPort(8888)                  // 默认端口
    , m_protocol("TCP")                   // 默认使用TCP协议
    , m_connected(false)                  // 连接状态标志
    , m_bytesReceived(0)                  // 累计接收字节数
    , m_bytesSent(0)                      // 累计发送字节数
    , m_connectionStartTime(0)            // 连接开始时间戳
    , m_statisticsTimer(nullptr)          // 统计更新定时器
{
    hide();                               // 隐藏控件（纯后端逻辑）
    setupConnections();                   // 建立信号槽连接
}

/**
 * @brief 析构函数
 *
 * 在销毁前停止所有连接，释放资源
 */
ConnectionWidget::~ConnectionWidget()
{
    stopConnection();
}

/**
 * @brief 建立信号槽连接
 *
 * 创建TCP和UDP Socket对象，并连接相关信号到槽函数：
 * - TCP连接状态变化（connected/disconnected）
 * - TCP数据接收（readyRead）
 * - TCP错误处理
 * - UDP数据接收
 * - 统计定时器超时
 */
void ConnectionWidget::setupConnections()
{
    // 创建Socket对象
    m_tcpSocket = new QTcpSocket(this);
    m_udpSocket = new QUdpSocket(this);
    m_statisticsTimer = new QTimer(this);

    // 连接TCP信号
    connect(m_tcpSocket, &QTcpSocket::connected, this, &ConnectionWidget::onTcpConnected);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &ConnectionWidget::onTcpDisconnected);
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &ConnectionWidget::onTcpReadyRead);
    connect(m_tcpSocket, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
            this, &ConnectionWidget::onTcpError);
    
    // 连接UDP信号
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &ConnectionWidget::onUdpReadyRead);
    
    // 连接统计定时器
    connect(m_statisticsTimer, &QTimer::timeout, this, &ConnectionWidget::updateStatistics);
}

/**
 * @brief 启动连接
 *
 * 调用内部连接函数，开始建立网络连接
 */
void ConnectionWidget::startConnection()
{
    onConnectClicked();
}

void ConnectionWidget::stopConnection()
{
    onDisconnectClicked();
}

bool ConnectionWidget::isConnected() const
{
    return m_connected;
}

void ConnectionWidget::setServerAddress(const QString &address)
{
    m_serverAddress = address;
}

void ConnectionWidget::setServerPort(int port)
{
    m_serverPort = port;
}

void ConnectionWidget::setProtocol(const QString &protocol)
{
    m_protocol = protocol;
}

QString ConnectionWidget::serverAddress() const
{
    return m_serverAddress;
}

int ConnectionWidget::serverPort() const
{
    return m_serverPort;
}

QString ConnectionWidget::protocol() const
{
    return m_protocol;
}

void ConnectionWidget::onConnectClicked()
{
    const QString address = m_serverAddress.trimmed();
    const int port = m_serverPort;

    if (address.isEmpty()) {
        emit connectionError(tr("服务器地址不能为空"));
        return;
    }

    logMessage("INFO", tr("正在连接 %1:%2 (%3)...").arg(address).arg(port).arg(m_protocol));

    if (m_protocol == "TCP") {
        m_tcpSocket->connectToHost(address, port);
    } else if (m_protocol == "UDP") {
        if (m_udpSocket->bind(QHostAddress::Any, port + 1, QUdpSocket::ShareAddress)) {
            m_connected = true;
            m_connectionStartTime = QDateTime::currentSecsSinceEpoch();
            m_statisticsTimer->start(1000);
            emit connectionStatusChanged(true);
            logMessage("INFO", tr("UDP绑定成功"));
        } else {
            emit connectionError(tr("UDP绑定失败: %1").arg(m_udpSocket->errorString()));
        }
    }
}

void ConnectionWidget::onDisconnectClicked()
{
    if (m_protocol == "TCP") {
        m_tcpSocket->disconnectFromHost();
    } else if (m_protocol == "UDP") {
        m_udpSocket->abort();
    }

    m_connected = false;
    m_statisticsTimer->stop();
    emit connectionStatusChanged(false);
    logMessage("INFO", tr("已断开连接"));
}

void ConnectionWidget::onTcpConnected()
{
    m_connected = true;
    m_connectionStartTime = QDateTime::currentSecsSinceEpoch();
    m_statisticsTimer->start(1000);
    emit connectionStatusChanged(true);
    logMessage("INFO", tr("TCP连接成功"));
}

void ConnectionWidget::onTcpDisconnected()
{
    m_connected = false;
    m_statisticsTimer->stop();
    emit connectionStatusChanged(false);
    logMessage("WARN", tr("TCP连接断开"));
}

void ConnectionWidget::onTcpReadyRead()
{
    QByteArray data = m_tcpSocket->readAll();
    m_bytesReceived += data.size();
    emit dataReceived(data);
}

void ConnectionWidget::onTcpError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    const QString errorMsg = m_tcpSocket->errorString();
    logMessage("ERROR", tr("TCP错误: %1").arg(errorMsg));
    emit connectionError(errorMsg);
}

void ConnectionWidget::onUdpReadyRead()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        m_bytesReceived += datagram.size();
        emit dataReceived(datagram);
    }
}

void ConnectionWidget::updateStatistics()
{
    emit statisticsUpdated(m_bytesReceived, m_bytesSent);
}

void ConnectionWidget::logMessage(const QString &level, const QString &message)
{
    qDebug("[%s] %s", level.toUtf8().constData(), message.toUtf8().constData());
}

} // namespace ground_station
