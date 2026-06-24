/**
 * @file application.h
 * @brief 主应用程序类
 *
 * 封装完整的客户端交互流程演示
 */

#ifndef GROUND_STATION_APPLICATION_H
#define GROUND_STATION_APPLICATION_H

#include <QObject>
#include <QTimer>
#include "streamclient.h"
#include "logger.h"

namespace ground_station {

/**
 * @brief 主应用程序类
 *
 * 封装完整的客户端交互流程演示
 */
class Application : public QObject {
    Q_OBJECT

public:
    explicit Application(QObject* parent = nullptr);
    ~Application() override;

    /**
     * @brief 运行应用程序
     */
    void run();

private slots:
    /**
     * @brief 处理连接成功
     */
    void onConnected();

    /**
     * @brief 处理连接失败
     */
    void onConnectionFailed(const QString& errorMsg);

    /**
     * @brief 处理断开连接
     */
    void onDisconnected();

    /**
     * @brief 处理状态变更
     */
    void onStateChanged(ClientState state);

    /**
     * @brief 处理流注册成功
     */
    void onStreamRegistered(int streamId, const QString& streamName);

    /**
     * @brief 处理流切换
     */
    void onStreamSwitched(int streamId);

    /**
     * @brief 处理数据接收
     */
    void onDataReceived(int streamId, const QByteArray& data);

    /**
     * @brief 处理错误
     */
    void onErrorOccurred(const QString& errorMsg);

    /**
     * @brief 输出统计信息
     */
    void printStatistics();

private:
    /**
     * @brief 设置客户端信号连接
     *
     * 连接客户端的各种信号到对应的槽函数
     */
    void setupClientSignals();

    StreamClient* m_client;
    QTimer* m_demoTimer;
};

}  // namespace ground_station

#endif  // GROUND_STATION_APPLICATION_H
