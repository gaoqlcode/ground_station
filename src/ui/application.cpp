/**
 * @file application.cpp
 * @brief 主应用程序类实现
 *
 * 封装完整的客户端交互流程演示
 */

#include "application.h"

#include <QCoreApplication>

namespace ground_station {

Application::Application(QObject* parent)
    : QObject(parent)
    , m_client(new StreamClient(this))
    , m_demoTimer(new QTimer(this)) {
    setupClientSignals();
}

Application::~Application() {
    if (m_client) {
        m_client->stop();
    }
}

void Application::run() {
    if (!m_client->initialize("client.ini")) {
        LOG_ERROR("Application", "客户端初始化失败");
        qApp->quit();
        return;
    }

    if (!m_client->start()) {
        LOG_ERROR("Application", "客户端启动失败");
        qApp->quit();
        return;
    }

    LOG_INFO("Application", "客户端启动成功");

    if (!m_client->connectToServer()) {
        LOG_ERROR("Application", "连接服务器失败");
        qApp->quit();
        return;
    }

    connect(m_demoTimer, &QTimer::timeout, this, &Application::printStatistics);
    m_demoTimer->start(5000);
}

void Application::onConnected() {
    LOG_INFO("Application", "成功连接到服务器");

    int streamId1 = m_client->registerStream("Camera1");
    if (streamId1 >= 0) {
        LOG_INFO("Application", QString("流1已注册: ID=%1").arg(streamId1));
    }

    QTimer::singleShot(2000, this, [this]() {
        int streamId2 = m_client->registerStream("Camera2");
        if (streamId2 >= 0) {
            LOG_INFO("Application", QString("流2已注册: ID=%1").arg(streamId2));
        }
    });
}

void Application::onConnectionFailed(const QString& errorMsg) {
    LOG_ERROR("Application", QString("连接失败: %1").arg(errorMsg));
}

void Application::onDisconnected() {
    LOG_WARN("Application", "已断开与服务器的连接");
}

void Application::onStateChanged(ClientState state) {
    QString stateName;
    switch (state) {
        case ClientState::Disconnected:
            stateName = "已断开";
            break;
        case ClientState::Connecting:
            stateName = "连接中";
            break;
        case ClientState::Connected:
            stateName = "已连接";
            break;
        case ClientState::Registering:
            stateName = "注册中";
            break;
        case ClientState::Streaming:
            stateName = "流传输中";
            break;
        case ClientState::Error:
            stateName = "错误";
            break;
    }

    LOG_INFO("Application", QString("客户端状态变更: %1").arg(stateName));
}

void Application::onStreamRegistered(int streamId, const QString& streamName) {
    LOG_INFO("Application", QString("流已注册: ID=%1, 名称=%2")
                                .arg(streamId)
                                .arg(streamName));
}

void Application::onStreamSwitched(int streamId) {
    LOG_INFO("Application", QString("流已切换至: ID=%1").arg(streamId));
}

void Application::onDataReceived(int streamId, const QByteArray& data) {
    LOG_DEBUG("Application", QString("接收到来自流%1的数据: %2字节")
                                  .arg(streamId)
                                  .arg(data.size()));
}

void Application::onErrorOccurred(const QString& errorMsg) {
    LOG_ERROR("Application", QString("错误: %1").arg(errorMsg));
}

void Application::printStatistics() {
    QString stats = m_client->getStatistics();
    LOG_INFO("Application", QString("统计信息: %1").arg(stats));
}

void Application::setupClientSignals() {
    connect(m_client, &StreamClient::connected, this, &Application::onConnected);
    connect(m_client, &StreamClient::connectionFailed, this, &Application::onConnectionFailed);
    connect(m_client, &StreamClient::disconnected, this, &Application::onDisconnected);
    connect(m_client, &StreamClient::stateChanged, this, &Application::onStateChanged);
    connect(m_client, &StreamClient::streamRegistered, this, &Application::onStreamRegistered);
    connect(m_client, &StreamClient::streamSwitched, this, &Application::onStreamSwitched);
    connect(m_client, &StreamClient::dataReceived, this, &Application::onDataReceived);
    connect(m_client, &StreamClient::errorOccurred, this, &Application::onErrorOccurred);
}

}  // namespace ground_station
