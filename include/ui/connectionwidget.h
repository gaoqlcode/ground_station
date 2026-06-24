/**
 * @file connectionwidget.h
 * @brief 连接管理控件头文件
 *
 * 提供TCP/UDP连接管理功能，支持连接状态监控和数据统计。
 * 该控件由连接设置对话框调用，作为后端逻辑组件。
 *
 * @author GroundStation Team
 * @date 2024
 */

#ifndef GROUND_STATION_CONNECTIONWIDGET_H
#define GROUND_STATION_CONNECTIONWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QTcpSocket>
#include <QUdpSocket>

namespace ground_station {

/**
 * @class ConnectionWidget
 * @brief 连接管理控件类
 *
 * 实现TCP/UDP连接的建立、断开、数据收发和统计功能。
 * 该类是无界面控件，由ConnectionSettingsDialog操作。
 */
class ConnectionWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父控件指针
     */
    explicit ConnectionWidget(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ConnectionWidget();

    /**
     * @brief 启动连接
     */
    void startConnection();

    /**
     * @brief 停止连接
     */
    void stopConnection();

    /**
     * @brief 获取连接状态
     * @return 是否已连接
     */
    bool isConnected() const;

    /**
     * @brief 设置服务器地址
     * @param address 服务器地址
     */
    void setServerAddress(const QString &address);

    /**
     * @brief 设置服务器端口
     * @param port 端口号
     */
    void setServerPort(int port);

    /**
     * @brief 设置协议类型
     * @param protocol 协议类型（TCP或UDP）
     */
    void setProtocol(const QString &protocol);

    /**
     * @brief 获取服务器地址
     * @return 服务器地址
     */
    QString serverAddress() const;

    /**
     * @brief 获取服务器端口
     * @return 端口号
     */
    int serverPort() const;

    /**
     * @brief 获取协议类型
     * @return 协议类型
     */
    QString protocol() const;

signals:
    /**
     * @brief 连接状态改变信号
     * @param connected 是否连接
     */
    void connectionStatusChanged(bool connected);

    /**
     * @brief 连接错误信号
     * @param error 错误消息
     */
    void connectionError(const QString &error);

    /**
     * @brief 数据接收信号
     * @param data 接收到的数据
     */
    void dataReceived(const QByteArray &data);

    /**
     * @brief 统计信息更新信号
     * @param bytesReceived 接收字节数
     * @param bytesSent 发送字节数
     */
    void statisticsUpdated(qint64 bytesReceived, qint64 bytesSent);

private slots:
    /**
     * @brief 连接按钮点击处理
     */
    void onConnectClicked();

    /**
     * @brief 断开连接按钮点击处理
     */
    void onDisconnectClicked();

    /**
     * @brief TCP连接成功处理
     */
    void onTcpConnected();

    /**
     * @brief TCP连接断开处理
     */
    void onTcpDisconnected();

    /**
     * @brief TCP数据接收处理
     */
    void onTcpReadyRead();

    /**
     * @brief TCP错误处理
     * @param error 错误类型
     */
    void onTcpError(QAbstractSocket::SocketError error);

    /**
     * @brief UDP数据接收处理
     */
    void onUdpReadyRead();

    /**
     * @brief 更新统计信息
     */
    void updateStatistics();

private:
    /**
     * @brief 设置信号槽连接
     */
    void setupConnections();

    /**
     * @brief 记录日志消息
     * @param level 日志级别
     * @param message 消息内容
     */
    void logMessage(const QString &level, const QString &message);

private:
    QTcpSocket *m_tcpSocket;       // TCP Socket
    QUdpSocket *m_udpSocket;       // UDP Socket

    QString m_serverAddress;        // 服务器地址
    int m_serverPort;               // 服务器端口
    QString m_protocol;             // 协议类型

    bool m_connected;               // 连接状态
    qint64 m_bytesReceived;         // 接收字节数
    qint64 m_bytesSent;             // 发送字节数
    qint64 m_connectionStartTime;   // 连接开始时间

    QTimer *m_statisticsTimer;      // 统计更新定时器
};

} // namespace ground_station

#endif // GROUND_STATION_CONNECTIONWIDGET_H
