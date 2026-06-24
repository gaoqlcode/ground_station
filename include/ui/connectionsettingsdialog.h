/**
 * @file connectionsettingsdialog.h
 * @brief 连接设置对话框头文件
 *
 * 提供服务器连接配置界面，支持地址、端口、协议设置和连接状态显示。
 *
 * @author GroundStation Team
 * @date 2024
 */

#ifndef GROUND_STATION_CONNECTIONSETTINGSDIALOG_H
#define GROUND_STATION_CONNECTIONSETTINGSDIALOG_H

#include <QDialog>

class QLabel;
class QLineEdit;
class QSpinBox;
class QComboBox;
class QPushButton;

namespace ground_station {

class ConnectionWidget;

/**
 * @class ConnectionSettingsDialog
 * @brief 连接设置对话框类
 *
 * 提供服务器连接配置界面，支持地址、端口、协议选择，显示连接状态和统计信息。
 */
class ConnectionSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param connectionWidget 连接管理后端指针
     * @param parent 父窗口指针
     */
    explicit ConnectionSettingsDialog(ConnectionWidget *connectionWidget, QWidget *parent = nullptr);

    /**
     * @brief 从后端刷新配置
     */
    void refreshFromBackend();

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
     * @brief 连接状态改变处理
     * @param connected 是否连接
     */
    void onConnectionStatusChanged(bool connected);

private:
    /**
     * @brief 设置用户界面
     */
    void setupUi();

    /**
     * @brief 更新界面状态
     */
    void updateUiState();

    ConnectionWidget *m_connectionWidget;   // 连接管理后端

    QLineEdit *m_addressEdit;               // 服务器地址输入框
    QSpinBox *m_portSpinBox;                // 端口号输入框
    QComboBox *m_protocolCombo;             // 协议选择下拉框
    QPushButton *m_connectBtn;              // 连接按钮
    QPushButton *m_disconnectBtn;           // 断开按钮
    QLabel *m_statusLabel;                  // 连接状态标签
    QLabel *m_statusIndicator;              // 连接状态指示灯
    QLabel *m_bytesReceivedLabel;           // 接收字节数标签
    QLabel *m_bytesSentLabel;               // 发送字节数标签
    QLabel *m_durationLabel;                // 连接时长标签
};

} // namespace ground_station

#endif // GROUND_STATION_CONNECTIONSETTINGSDIALOG_H
