/**
 * @file mainwindow.h
 * @brief 地面站客户端主窗口类
 *
 * 主窗口是地面站客户端的核心界面，整合了所有功能模块：
 * - 相机列表管理
 * - 双路视频显示
 * - 连接状态管理
 * - 相机参数设置
 * - 日志显示
 * - 统计信息展示
 *
 * @author GroundStation Team
 * @date 2024
 */

#ifndef GROUND_STATION_MAINWINDOW_H
#define GROUND_STATION_MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QStatusBar>
#include <QMenuBar>
#include <QSplitter>
#include <QTextEdit>
#include <QTimer>

#include "cameralistwidget.h"
#include "videodisplaywidget.h"
#include "connectionwidget.h"
#include "camerparamswidget.h"
#include "connectionsettingsdialog.h"
#include "network/streamclient.h"

namespace Ui {
class MainWindow;
}

namespace ground_station {

/**
 * @class MainWindow
 * @brief 主窗口类
 *
 * 继承自QMainWindow，实现地面站客户端的主界面布局和功能控制。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~MainWindow();

protected:
    /**
     * @brief 关闭事件处理
     * @param event 关闭事件
     */
    void closeEvent(QCloseEvent *event) override;

    /**
     * @brief 窗口大小改变事件处理
     * @param event 大小改变事件
     */
    void resizeEvent(QResizeEvent *event) override;
    
    /**
     * @brief 鼠标按下事件处理（用于拖拽窗口）
     * @param event 鼠标事件
     */
    void mousePressEvent(QMouseEvent *event) override;
    
    /**
     * @brief 鼠标移动事件处理（用于拖拽窗口）
     * @param event 鼠标事件
     */
    void mouseMoveEvent(QMouseEvent *event) override;
    
    /**
     * @brief 鼠标释放事件处理（重置拖拽状态）
     * @param event 鼠标事件
     */
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    /**
     * @brief 相机选中处理
     * @param cameraId 相机ID
     */
    void onCameraSelected(const QString &cameraId);

    /**
     * @brief 相机双击处理
     * @param cameraId 相机ID
     */
    void onCameraDoubleClicked(const QString &cameraId);

    /**
     * @brief 连接按钮点击处理
     */
    void onConnectClicked();

    /**
     * @brief 断开连接按钮点击处理
     */
    void onDisconnectClicked();

    /**
     * @brief 连接设置按钮点击处理
     */
    void onConnectionSettingsClicked();

    /**
     * @brief 关于按钮点击处理
     */
    void onAboutClicked();

    /**
     * @brief 更新统计信息
     */
    void updateStatistics();

    /**
     * @brief 连接状态改变处理
     * @param connected 是否连接
     */
    void onConnectionStatusChanged(bool connected);

    /**
     * @brief 视频帧接收处理
     * @param displayIndex 显示索引（0或1）
     * @param frame 视频帧图像
     */
    void onVideoFrameReceived(int displayIndex, const QImage &frame);

    /**
     * @brief 客户端连接成功处理
     */
    void onClientConnected();

    /**
     * @brief 客户端连接失败处理
     * @param errorMsg 错误消息
     */
    void onClientConnectionFailed(const QString& errorMsg);

    /**
     * @brief 客户端断开连接处理
     */
    void onClientDisconnected();

    /**
     * @brief 客户端状态改变处理
     * @param state 新状态
     */
    void onClientStateChanged(ClientState state);

    /**
     * @brief 流注册成功处理
     * @param streamId 流ID
     * @param streamName 流名称
     */
    void onStreamRegistered(int streamId, const QString& streamName);

    /**
     * @brief 流切换处理
     * @param streamId 流ID
     */
    void onStreamSwitched(int streamId);

    /**
     * @brief 客户端数据接收处理
     * @param streamId 流ID
     * @param data 数据
     */
    void onClientDataReceived(int streamId, const QByteArray& data);

    /**
     * @brief 客户端错误处理
     * @param errorMsg 错误消息
     */
    void onClientErrorOccurred(const QString& errorMsg);

    /**
     * @brief 打印客户端统计信息
     */
    void printClientStatistics();

private:
    /**
     * @brief 设置用户界面
     */
    void setupUi();

    /**
     * @brief 设置菜单栏
     */
    void setupMenuBar();

    /**
     * @brief 设置状态栏
     */
    void setupStatusBar();

    /**
     * @brief 设置信号槽连接
     */
    void setupConnections();

    /**
     * @brief 设置流客户端
     */
    void setupStreamClient();

    /**
     * @brief 加载设置
     */
    void loadSettings();

    /**
     * @brief 保存设置
     */
    void saveSettings();

    /**
     * @brief 记录日志消息
     * @param level 日志级别
     * @param message 消息内容
     */
    void logMessage(const QString &level, const QString &message);
    
    /**
     * @brief 切换最大化/还原窗口
     */
    void toggleMaximize();
    
    /**
     * @brief 显示文件菜单
     */
    void showFileMenu();
    
    /**
     * @brief 显示设置菜单
     */
    void showSettingsMenu();
    
    /**
     * @brief 显示帮助菜单
     */
    void showHelpMenu();

private:
    Ui::MainWindow *ui;                           // UI设计器对象

    QSplitter *m_mainSplitter;                    // 主分割器
    QSplitter *m_videoSplitter;                   // 视频区域分割器
    QSplitter *m_leftSplitter;                    // 左侧区域分割器

    CameraListWidget *m_cameraListWidget;         // 相机列表控件
    VideoDisplayWidget *m_videoDisplay1;          // 视频显示1
    VideoDisplayWidget *m_videoDisplay2;          // 视频显示2

    ConnectionWidget *m_connectionWidget;         // 连接管理控件
    CameraParamsWidget *m_cameraParamsWidget;     // 相机参数控件
    ConnectionSettingsDialog *m_connectionSettingsDialog; // 连接设置对话框

    QTextEdit *m_logTextEdit;                     // 日志显示控件

    QLabel *m_statusLabel;                        // 状态标签
    QLabel *m_fpsLabel;                           // 帧率标签
    QLabel *m_bitrateLabel;                       // 比特率标签

    QTimer *m_statisticsTimer;                    // 统计更新定时器
    QTimer *m_clientStatsTimer;                   // 客户端统计定时器

    int m_frameCount1;                            // 视频1帧计数
    int m_frameCount2;                            // 视频2帧计数
    qint64 m_totalBytes;                          // 总字节数

    StreamClient *m_streamClient;                 // 流客户端
    
    QWidget *m_titleBar;                          // 自定义标题栏
    QPoint m_dragPos;                             // 窗口拖拽位置
};

} // namespace ground_station

#endif // GROUND_STATION_MAINWINDOW_H
