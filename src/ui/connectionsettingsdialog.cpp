/**
 * @file connectionsettingsdialog.cpp
 * @brief 连接设置对话框实现
 *
 * 本文件实现了ConnectionSettingsDialog类，提供服务器连接配置界面。
 * 主要功能包括：
 * - 服务器地址和端口配置
 * - 协议选择（TCP/UDP）
 * - 连接状态实时显示
 * - 数据收发统计展示
 * - 连接/断开操作按钮
 *
 * @author GroundStation Team
 * @date 2024
 */

#include "connectionsettingsdialog.h"
#include "connectionwidget.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>

namespace ground_station {

/**
 * @brief 构造函数
 *
 * 初始化连接设置对话框，创建界面控件并连接信号槽。
 *
 * @param connectionWidget 连接管理后端指针
 * @param parent 父窗口指针
 */
ConnectionSettingsDialog::ConnectionSettingsDialog(ConnectionWidget *connectionWidget, QWidget *parent)
    : QDialog(parent)
    , m_connectionWidget(connectionWidget)  // 连接管理后端
    , m_addressEdit(nullptr)                // 服务器地址输入框
    , m_portSpinBox(nullptr)                // 端口号输入框
    , m_protocolCombo(nullptr)              // 协议选择下拉框
    , m_connectBtn(nullptr)                 // 连接按钮
    , m_disconnectBtn(nullptr)              // 断开按钮
    , m_statusLabel(nullptr)                // 连接状态标签
    , m_statusIndicator(nullptr)            // 连接状态指示灯
    , m_bytesReceivedLabel(nullptr)         // 接收字节数标签
    , m_bytesSentLabel(nullptr)             // 发送字节数标签
    , m_durationLabel(nullptr)              // 连接时长标签
{
    setupUi();                              // 创建界面控件
    refreshFromBackend();                   // 从后端刷新配置

    // 连接后端信号
    if (m_connectionWidget) {
        // 连接状态变化信号
        connect(m_connectionWidget, &ConnectionWidget::connectionStatusChanged,
                this, &ConnectionSettingsDialog::onConnectionStatusChanged);
        // 统计数据更新信号
        connect(m_connectionWidget, &ConnectionWidget::statisticsUpdated,
                this, [this](qint64 received, qint64 sent) {
            QString receivedStr;
            if (received < 1024) {
                receivedStr = tr("%1 B").arg(received);
            } else if (received < 1024 * 1024) {
                receivedStr = tr("%1 KB").arg(received / 1024.0, 0, 'f', 1);
            } else {
                receivedStr = tr("%1 MB").arg(received / (1024.0 * 1024.0), 0, 'f', 2);
            }

            QString sentStr;
            if (sent < 1024) {
                sentStr = tr("%1 B").arg(sent);
            } else if (sent < 1024 * 1024) {
                sentStr = tr("%1 KB").arg(sent / 1024.0, 0, 'f', 1);
            } else {
                sentStr = tr("%1 MB").arg(sent / (1024.0 * 1024.0), 0, 'f', 2);
            }

            m_bytesReceivedLabel->setText(tr("接收: %1").arg(receivedStr));
            m_bytesSentLabel->setText(tr("发送: %1").arg(sentStr));
        });
    }
}

void ConnectionSettingsDialog::setupUi()
{
    setWindowTitle(tr("连接设置"));
    setMinimumWidth(520);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QGroupBox *group = new QGroupBox(tr("服务器连接"), this);
    QGridLayout *groupLayout = new QGridLayout(group);
    groupLayout->setContentsMargins(12, 16, 12, 12);
    groupLayout->setSpacing(10);
    groupLayout->setColumnStretch(1, 2);

    groupLayout->addWidget(new QLabel(tr("服务器地址:"), this), 0, 0);
    m_addressEdit = new QLineEdit(this);
    m_addressEdit->setPlaceholderText(tr("例如: 192.168.1.100"));
    groupLayout->addWidget(m_addressEdit, 0, 1, 1, 3);

    groupLayout->addWidget(new QLabel(tr("端口:"), this), 1, 0);
    m_portSpinBox = new QSpinBox(this);
    m_portSpinBox->setRange(1, 65535);
    m_portSpinBox->setValue(8888);
    groupLayout->addWidget(m_portSpinBox, 1, 1);

    groupLayout->addWidget(new QLabel(tr("协议:"), this), 1, 2);
    m_protocolCombo = new QComboBox(this);
    m_protocolCombo->addItem(tr("TCP"), "TCP");
    m_protocolCombo->addItem(tr("UDP"), "UDP");
    groupLayout->addWidget(m_protocolCombo, 1, 3);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_connectBtn = new QPushButton(tr("连接"), this);
    m_connectBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; }");
    m_disconnectBtn = new QPushButton(tr("断开"), this);
    m_disconnectBtn->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; }");
    btnLayout->addWidget(m_connectBtn);
    btnLayout->addWidget(m_disconnectBtn);
    btnLayout->addStretch();
    groupLayout->addLayout(btnLayout, 2, 0, 1, 4);

    QHBoxLayout *statusLayout = new QHBoxLayout();
    m_statusLabel = new QLabel(tr("状态: 未连接"), this);
    m_statusIndicator = new QLabel(this);
    m_statusIndicator->setFixedSize(16, 16);
    m_statusIndicator->setStyleSheet("background-color: #f44336; border-radius: 8px;");
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addWidget(m_statusIndicator);
    statusLayout->addStretch();
    groupLayout->addLayout(statusLayout, 3, 0, 1, 4);

    m_bytesReceivedLabel = new QLabel(tr("接收: 0 B"), this);
    m_bytesSentLabel = new QLabel(tr("发送: 0 B"), this);
    m_durationLabel = new QLabel(tr("时长: 00:00:00"), this);
    groupLayout->addWidget(m_bytesReceivedLabel, 4, 0, 1, 2);
    groupLayout->addWidget(m_bytesSentLabel, 4, 2);
    groupLayout->addWidget(m_durationLabel, 4, 3);

    mainLayout->addWidget(group);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    connect(m_connectBtn, &QPushButton::clicked, this, &ConnectionSettingsDialog::onConnectClicked);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &ConnectionSettingsDialog::onDisconnectClicked);
}

void ConnectionSettingsDialog::refreshFromBackend()
{
    if (!m_connectionWidget) {
        return;
    }

    m_addressEdit->setText(m_connectionWidget->serverAddress());
    m_portSpinBox->setValue(m_connectionWidget->serverPort());
    m_protocolCombo->setCurrentIndex(m_connectionWidget->protocol() == "UDP" ? 1 : 0);
    updateUiState();
}

void ConnectionSettingsDialog::onConnectClicked()
{
    if (!m_connectionWidget) {
        return;
    }

    m_connectionWidget->setServerAddress(m_addressEdit->text().trimmed());
    m_connectionWidget->setServerPort(m_portSpinBox->value());
    m_connectionWidget->setProtocol(m_protocolCombo->currentData().toString());
    m_connectionWidget->startConnection();
    updateUiState();
}

void ConnectionSettingsDialog::onDisconnectClicked()
{
    if (!m_connectionWidget) {
        return;
    }

    m_connectionWidget->stopConnection();
    updateUiState();
}

void ConnectionSettingsDialog::onConnectionStatusChanged(bool connected)
{
    Q_UNUSED(connected);
    updateUiState();
}

void ConnectionSettingsDialog::updateUiState()
{
    const bool connected = m_connectionWidget && m_connectionWidget->isConnected();

    m_connectBtn->setEnabled(!connected);
    m_disconnectBtn->setEnabled(connected);
    m_addressEdit->setEnabled(!connected);
    m_portSpinBox->setEnabled(!connected);
    m_protocolCombo->setEnabled(!connected);

    if (connected) {
        m_statusLabel->setText(tr("状态: 已连接"));
        m_statusLabel->setStyleSheet("color: #4CAF50; font-weight: bold;");
        m_statusIndicator->setStyleSheet("background-color: #4CAF50; border-radius: 8px;");
    } else {
        m_statusLabel->setText(tr("状态: 未连接"));
        m_statusLabel->setStyleSheet("color: #f44336; font-weight: bold;");
        m_statusIndicator->setStyleSheet("background-color: #f44336; border-radius: 8px;");
    }
}

} // namespace ground_station
