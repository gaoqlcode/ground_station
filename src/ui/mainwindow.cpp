/**
 * @file mainwindow.cpp
 * @brief 主窗口实现
 *
 * 实现地面站客户端主窗口，包括：
 * - 界面布局管理（相机列表、视频显示、连接状态、日志）
 * - 菜单栏和工具栏
 * - 状态栏统计信息显示
 * - 设置保存与加载
 * - 窗口关闭确认
 *
 * 界面布局：
 * ┌─────────────────────────────────────────────────────────────────┐
 * │ 菜单栏（文件、设置、帮助）                                       │
 * ├──────────────────┬─────────────────────────────────────────────┤
 * │  相机列表        │  视频显示区域（双窗口，占主要空间）            │
 * │  运行日志        │  相机参数设置栏                              │
 * └──────────────────┴─────────────────────────────────────────────┘
 * │ 状态栏（连接状态、FPS、码率）                                    │
 * └─────────────────────────────────────────────────────────────────┘
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSettings>
#include <QDateTime>
#include <QCloseEvent>

namespace ground_station {

/**
 * @brief 构造函数
 * @param parent 父窗口指针
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_mainSplitter(nullptr)
    , m_videoSplitter(nullptr)
    , m_leftSplitter(nullptr)
    , m_cameraListWidget(nullptr)
    , m_videoDisplay1(nullptr)
    , m_videoDisplay2(nullptr)
    , m_connectionWidget(nullptr)
    , m_cameraParamsWidget(nullptr)
    , m_connectionSettingsDialog(nullptr)
    , m_logTextEdit(nullptr)
    , m_statusLabel(nullptr)
    , m_fpsLabel(nullptr)
    , m_bitrateLabel(nullptr)
    , m_statisticsTimer(nullptr)
    , m_clientStatsTimer(nullptr)
    , m_frameCount1(0)
    , m_frameCount2(0)
    , m_totalBytes(0)
    , m_streamClient(nullptr)
    , m_titleBar(nullptr)
    , m_dragPos(QPoint(0, 0))
{
    // 移除系统标题栏，使用自定义标题栏
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
    
    ui->setupUi(this);
    
    // 连接后端（无界面）
    m_connectionWidget = new ConnectionWidget(this);

    // 设置界面布局
    setupUi();

    // 设置菜单栏
    setupMenuBar();

    // 设置状态栏
    setupStatusBar();

    // 连接信号槽
    setupConnections();

    // 设置流客户端
    setupStreamClient();

    // 加载保存的设置
    loadSettings();

    // 启动统计定时器（每秒更新一次统计信息）
    m_statisticsTimer = new QTimer(this);
    connect(m_statisticsTimer, &QTimer::timeout, this, &MainWindow::updateStatistics);
    m_statisticsTimer->start(1000);

    logMessage("INFO", tr("地面站客户端已启动"));
}

/**
 * @brief 析构函数
 */
MainWindow::~MainWindow()
{
    // 停止客户端
    if (m_streamClient) {
        m_streamClient->stop();
    }

    // 保存当前设置
    saveSettings();
    delete ui;
}

/**
 * @brief 设置主界面布局
 *
 * 创建完整的界面布局，包括：
 * - 主分割器（左侧相机列表，右侧视频区域）
 * - 视频分割器（上半部分视频显示，下半部分状态和日志）
 * - 双视频显示窗口
 * - 连接状态控件
 * - 日志显示区域
 */
void MainWindow::setupUi()
{
    setWindowTitle(tr("地面站客户端"));
    setMinimumSize(1400, 900);
    resize(1600, 1000);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 自定义标题栏（使用集中样式）
    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName("customTitleBar");
    m_titleBar->setFixedHeight(28);
    m_titleBar->setMouseTracking(true);
    
    QHBoxLayout *titleBarLayout = new QHBoxLayout(m_titleBar);
    titleBarLayout->setContentsMargins(10, 2, 2, 2);
    titleBarLayout->setSpacing(0);
    
    // 标题栏图标
    QLabel *titleIcon = new QLabel(m_titleBar);
    titleIcon->setObjectName("titleIcon");
    titleIcon->setText("GQL");
    titleBarLayout->addWidget(titleIcon);
    
    // 标题栏文字
    QLabel *titleText = new QLabel(tr("地面站客户端"), m_titleBar);
    titleText->setStyleSheet("padding-left: 8px;");
    titleBarLayout->addWidget(titleText);
    
    // 弹性空间
    titleBarLayout->addStretch();
    
    // 窗口控制按钮容器
    QWidget *btnContainer = new QWidget(m_titleBar);
    QHBoxLayout *btnLayout = new QHBoxLayout(btnContainer);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(0);
    
    // 最小化按钮
    QPushButton *minBtn = new QPushButton(btnContainer);
    minBtn->setObjectName("windowMinBtn");
    minBtn->setText(QChar(0x2013)); // 标准短横线
    connect(minBtn, &QPushButton::clicked, this, &MainWindow::showMinimized);
    btnLayout->addWidget(minBtn);
    
    // 最大化按钮
    QPushButton *maxBtn = new QPushButton(btnContainer);
    maxBtn->setObjectName("windowMaxBtn");
    maxBtn->setText(QChar(0x25A1)); // 标准空心方块
    connect(maxBtn, &QPushButton::clicked, this, &MainWindow::toggleMaximize);
    btnLayout->addWidget(maxBtn);
    
    // 关闭按钮
    QPushButton *closeBtn = new QPushButton(btnContainer);
    closeBtn->setObjectName("windowCloseBtn");
    closeBtn->setText(QChar(0x2715)); // 标准乘号（关闭符号）
    connect(closeBtn, &QPushButton::clicked, this, &MainWindow::close);
    btnLayout->addWidget(closeBtn);
    
    titleBarLayout->addWidget(btnContainer);
    
    mainLayout->addWidget(m_titleBar);
    
    // 隐藏默认菜单栏，使用自定义菜单栏（使用集中样式）
    menuBar()->hide();
    
    // 自定义菜单栏
    QWidget *menuBarWidget = new QWidget(this);
    menuBarWidget->setObjectName("customMenuBar");
    menuBarWidget->setFixedHeight(24);
    
    QHBoxLayout *menuBarLayout = new QHBoxLayout(menuBarWidget);
    menuBarLayout->setContentsMargins(8, 0, 8, 0);
    menuBarLayout->setSpacing(0);
    
    // 文件菜单按钮
    QPushButton *fileMenuBtn = new QPushButton("文件(F)", menuBarWidget);
    connect(fileMenuBtn, &QPushButton::clicked, this, &MainWindow::showFileMenu);
    menuBarLayout->addWidget(fileMenuBtn);
    
    // 分隔线
    QLabel *sep1 = new QLabel(" | ", menuBarWidget);
    menuBarLayout->addWidget(sep1);
    
    // 设置菜单按钮
    QPushButton *settingsMenuBtn = new QPushButton("设置(S)", menuBarWidget);
    connect(settingsMenuBtn, &QPushButton::clicked, this, &MainWindow::showSettingsMenu);
    menuBarLayout->addWidget(settingsMenuBtn);
    
    // 分隔线
    QLabel *sep2 = new QLabel(" | ", menuBarWidget);
    menuBarLayout->addWidget(sep2);
    
    // 帮助菜单按钮
    QPushButton *helpMenuBtn = new QPushButton("帮助(H)", menuBarWidget);
    connect(helpMenuBtn, &QPushButton::clicked, this, &MainWindow::showHelpMenu);
    menuBarLayout->addWidget(helpMenuBtn);
    
    menuBarLayout->addStretch();
    
    mainLayout->addWidget(menuBarWidget);
    
    // 主内容区域
    QWidget *contentWidget = new QWidget(this);
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(8, 8, 8, 8);
    contentLayout->setSpacing(8);

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);

    // 左侧面板：相机列表 + 运行日志
    QWidget *leftPanel = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    m_leftSplitter = new QSplitter(Qt::Vertical, leftPanel);
    m_cameraListWidget = new CameraListWidget(this);
    m_cameraListWidget->setMinimumWidth(280);
    m_cameraListWidget->setMaximumWidth(450);

    QWidget *logPanel = new QWidget(this);
    QVBoxLayout *logLayout = new QVBoxLayout(logPanel);
    logLayout->setContentsMargins(0, 0, 0, 0);
    logLayout->setSpacing(4);

    QLabel *logTitle = new QLabel(tr("运行日志"), this);
    logTitle->setProperty("heading", true);
    logLayout->addWidget(logTitle);

    m_logTextEdit = new QTextEdit(this);
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setMinimumHeight(120);
    m_logTextEdit->setStyleSheet(
        "background-color: #1e1e1e; color: #d4d4d4; "
        "font-family: 'Consolas', 'Monaco', 'Microsoft YaHei', monospace; "
        "font-size: 13pt;");
    logLayout->addWidget(m_logTextEdit);

    m_leftSplitter->addWidget(m_cameraListWidget);
    m_leftSplitter->addWidget(logPanel);
    m_leftSplitter->setStretchFactor(0, 3);
    m_leftSplitter->setStretchFactor(1, 2);

    leftLayout->addWidget(m_leftSplitter);

    // 右侧：视频 + 相机参数
    QWidget *rightWidget = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(5);

    m_videoSplitter = new QSplitter(Qt::Vertical, this);

    QWidget *videoWidget = new QWidget(this);
    QHBoxLayout *videoLayout = new QHBoxLayout(videoWidget);
    videoLayout->setContentsMargins(0, 0, 0, 0);
    videoLayout->setSpacing(5);

    m_videoDisplay1 = new VideoDisplayWidget(tr("视频1"), this);
    m_videoDisplay2 = new VideoDisplayWidget(tr("视频2"), this);
    videoLayout->addWidget(m_videoDisplay1);
    videoLayout->addWidget(m_videoDisplay2);

    m_cameraParamsWidget = new CameraParamsWidget(this);
    m_cameraParamsWidget->setMaximumHeight(140);

    m_videoSplitter->addWidget(videoWidget);
    m_videoSplitter->addWidget(m_cameraParamsWidget);
    m_videoSplitter->setStretchFactor(0, 9);
    m_videoSplitter->setStretchFactor(1, 1);

    rightLayout->addWidget(m_videoSplitter);

    m_mainSplitter->addWidget(leftPanel);
    m_mainSplitter->addWidget(rightWidget);
    m_mainSplitter->setStretchFactor(0, 2);
    m_mainSplitter->setStretchFactor(1, 8);

    contentLayout->addWidget(m_mainSplitter);
    
    // 将内容区域添加到主布局
    mainLayout->addWidget(contentWidget);

    m_connectionSettingsDialog = new ConnectionSettingsDialog(m_connectionWidget, this);
}

void MainWindow::showFileMenu()
{
    QMenu menu(this);
    menu.addAction(tr("连接(&C)"), this, &MainWindow::onConnectClicked);
    menu.addAction(tr("断开(&D)"), this, &MainWindow::onDisconnectClicked);
    menu.addSeparator();
    menu.addAction(tr("退出(&X)"), this, &QMainWindow::close);
    menu.exec(QCursor::pos());
}

void MainWindow::showSettingsMenu()
{
    QMenu menu(this);
    menu.addAction(tr("连接设置(&C)"), this, &MainWindow::onConnectionSettingsClicked);
    menu.exec(QCursor::pos());
}

void MainWindow::showHelpMenu()
{
    QMenu menu(this);
    menu.addAction(tr("关于(&A)"), this, &MainWindow::onAboutClicked);
    menu.exec(QCursor::pos());
}

void MainWindow::setupMenuBar()
{
    menuBar()->clear();

    // 文件菜单
    QMenu *fileMenu = menuBar()->addMenu(tr("文件(&F)"));

    QAction *connectAction = new QAction(tr("连接(&C)"), this);
    connectAction->setShortcut(QKeySequence::New);
    connect(connectAction, &QAction::triggered, this, &MainWindow::onConnectClicked);
    fileMenu->addAction(connectAction);

    QAction *disconnectAction = new QAction(tr("断开(&D)"), this);
    disconnectAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
    connect(disconnectAction, &QAction::triggered, this, &MainWindow::onDisconnectClicked);
    fileMenu->addAction(disconnectAction);

    fileMenu->addSeparator();

    QAction *exitAction = new QAction(tr("退出(&X)"), this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(exitAction);

    // 设置菜单
    QMenu *settingsMenu = menuBar()->addMenu(tr("设置(&S)"));

    QAction *connectionSettingsAction = new QAction(tr("连接设置(&C)"), this);
    connect(connectionSettingsAction, &QAction::triggered, this, &MainWindow::onConnectionSettingsClicked);
    settingsMenu->addAction(connectionSettingsAction);

    // 帮助菜单
    QMenu *helpMenu = menuBar()->addMenu(tr("帮助(&H)"));

    QAction *aboutAction = new QAction(tr("关于(&A)"), this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAboutClicked);
    helpMenu->addAction(aboutAction);
}

void MainWindow::onConnectionSettingsClicked()
{
    if (m_connectionSettingsDialog) {
        m_connectionSettingsDialog->refreshFromBackend();
        m_connectionSettingsDialog->show();
        m_connectionSettingsDialog->raise();
        m_connectionSettingsDialog->activateWindow();
    }
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel(tr("未连接"), this);
    m_fpsLabel = new QLabel(tr("FPS: 0"), this);
    m_bitrateLabel = new QLabel(tr("码率: 0 KB/s"), this);

    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->addPermanentWidget(m_fpsLabel);
    statusBar()->addPermanentWidget(m_bitrateLabel);
}

void MainWindow::setupConnections()
{
    // 相机列表信号
    connect(m_cameraListWidget, &CameraListWidget::cameraSelected,
            this, &MainWindow::onCameraSelected);
    connect(m_cameraListWidget, &CameraListWidget::cameraDoubleClicked,
            this, &MainWindow::onCameraDoubleClicked);

    // 连接状态信号
    connect(m_connectionWidget, &ConnectionWidget::connectionStatusChanged,
            this, &MainWindow::onConnectionStatusChanged);
}

void MainWindow::setupStreamClient()
{
    m_streamClient = new StreamClient(this);

    // 连接 StreamClient 信号
    connect(m_streamClient, &StreamClient::connected,
            this, &MainWindow::onClientConnected);
    connect(m_streamClient, &StreamClient::connectionFailed,
            this, &MainWindow::onClientConnectionFailed);
    connect(m_streamClient, &StreamClient::disconnected,
            this, &MainWindow::onClientDisconnected);
    connect(m_streamClient, &StreamClient::stateChanged,
            this, &MainWindow::onClientStateChanged);
    connect(m_streamClient, &StreamClient::streamRegistered,
            this, &MainWindow::onStreamRegistered);
    connect(m_streamClient, &StreamClient::streamSwitched,
            this, &MainWindow::onStreamSwitched);
    connect(m_streamClient, &StreamClient::dataReceived,
            this, &MainWindow::onClientDataReceived);
    connect(m_streamClient, &StreamClient::errorOccurred,
            this, &MainWindow::onClientErrorOccurred);

    // 初始化客户端
    if (!m_streamClient->initialize("client.ini")) {
        logMessage("ERROR", tr("流客户端初始化失败"));
    } else {
        logMessage("INFO", tr("流客户端已初始化"));
    }

    // 统计信息定时器
    m_clientStatsTimer = new QTimer(this);
    connect(m_clientStatsTimer, &QTimer::timeout,
            this, &MainWindow::printClientStatistics);
}

void MainWindow::onClientConnected()
{
    logMessage("INFO", tr("成功连接到服务器"));

    int streamId1 = m_streamClient->registerStream("Camera1");
    if (streamId1 >= 0) {
        logMessage("INFO", tr("流1已注册: ID=%1").arg(streamId1));
    }

    QTimer::singleShot(2000, this, [this]() {
        int streamId2 = m_streamClient->registerStream("Camera2");
        if (streamId2 >= 0) {
            logMessage("INFO", tr("流2已注册: ID=%1").arg(streamId2));
        }
    });
}

void MainWindow::onClientConnectionFailed(const QString& errorMsg)
{
    logMessage("ERROR", tr("连接失败: %1").arg(errorMsg));
}

void MainWindow::onClientDisconnected()
{
    logMessage("WARN", tr("已断开与服务器的连接"));
}

void MainWindow::onClientStateChanged(ClientState state)
{
    QString stateName;
    switch (state) {
        case ClientState::Disconnected:
            stateName = tr("已断开");
            break;
        case ClientState::Connecting:
            stateName = tr("连接中");
            break;
        case ClientState::Connected:
            stateName = tr("已连接");
            break;
        case ClientState::Registering:
            stateName = tr("注册中");
            break;
        case ClientState::Streaming:
            stateName = tr("流传输中");
            break;
        case ClientState::Error:
            stateName = tr("错误");
            break;
    }

    logMessage("INFO", tr("客户端状态变更: %1").arg(stateName));
}

void MainWindow::onStreamRegistered(int streamId, const QString& streamName)
{
    logMessage("INFO", tr("流已注册: ID=%1, 名称=%2")
                    .arg(streamId)
                    .arg(streamName));
}

void MainWindow::onStreamSwitched(int streamId)
{
    logMessage("INFO", tr("流已切换至: ID=%1").arg(streamId));
}

void MainWindow::onClientDataReceived(int streamId, const QByteArray& data)
{
    logMessage("DEBUG", tr("接收到来自流%1的数据: %2字节")
                                  .arg(streamId)
                                  .arg(data.size()));
}

void MainWindow::onClientErrorOccurred(const QString& errorMsg)
{
    logMessage("ERROR", tr("错误: %1").arg(errorMsg));
}

void MainWindow::printClientStatistics()
{
    if (m_streamClient) {
        QString stats = m_streamClient->getStatistics();
        logMessage("INFO", tr("统计信息: %1").arg(stats));
    }
}

void MainWindow::loadSettings()
{
    QSettings settings("GqlSystem", "GroundStation");

    // 窗口几何
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    // 分割器状态
    if (settings.contains("mainSplitter")) {
        m_mainSplitter->restoreState(settings.value("mainSplitter").toByteArray());
    }
    if (settings.contains("videoSplitter")) {
        m_videoSplitter->restoreState(settings.value("videoSplitter").toByteArray());
    }
    if (settings.contains("leftSplitter")) {
        m_leftSplitter->restoreState(settings.value("leftSplitter").toByteArray());
    }
}

void MainWindow::saveSettings()
{
    QSettings settings("GqlSystem", "GroundStation");

    // 窗口几何
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());

    // 分割器状态
    settings.setValue("mainSplitter", m_mainSplitter->saveState());
    settings.setValue("videoSplitter", m_videoSplitter->saveState());
    settings.setValue("leftSplitter", m_leftSplitter->saveState());
}

void MainWindow::logMessage(const QString &level, const QString &message)
{
    // 设置颜色
    QColor textColor;
    if (level == "INFO") {
        textColor = QColor(Qt::green);
    } else if (level == "ERROR") {
        textColor = QColor(Qt::red);
    } else if (level == "WARN") {
        textColor = QColor(Qt::yellow);
    } else {
        textColor = QColor(Qt::white);
    }

    // 保存当前格式
    QTextCharFormat format = m_logTextEdit->currentCharFormat();
    QColor originalColor = format.foreground().color();

    // 设置新颜色
    m_logTextEdit->setTextColor(textColor);

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString logEntry = QString("[%1] [%2] %3").arg(timestamp, level, message);

    m_logTextEdit->append(logEntry);

    // 自动滚动到底部
    QTextCursor cursor = m_logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logTextEdit->setTextCursor(cursor);
}

void MainWindow::onCameraSelected(const QString &cameraId)
{
    logMessage("INFO", tr("选中相机: %1").arg(cameraId));
}

void MainWindow::onCameraDoubleClicked(const QString &cameraId)
{
    logMessage("INFO", tr("双击相机: %1，准备连接").arg(cameraId));
    // TODO: 实现相机连接逻辑
}

void MainWindow::onConnectClicked()
{
    logMessage("INFO", tr("开始连接服务器..."));

    if (!m_streamClient->start()) {
        logMessage("ERROR", tr("客户端启动失败"));
        return;
    }

    if (!m_streamClient->connectToServer()) {
        logMessage("ERROR", tr("连接服务器失败"));
        return;
    }

    m_connectionWidget->startConnection();
    m_clientStatsTimer->start(5000);

    logMessage("INFO", tr("客户端启动成功"));
}

void MainWindow::onDisconnectClicked()
{
    logMessage("INFO", tr("断开连接"));

    if (m_streamClient) {
        m_streamClient->stop();
    }

    m_connectionWidget->stopConnection();
    m_clientStatsTimer->stop();
}

void MainWindow::onAboutClicked()
{
    QMessageBox::about(this, tr("关于"),
        tr("<h3>地面站客户端</h3>"
           "<p>版本: 1.0.0</p>"
           "<p>基于Qt 5.12.8开发</p>"
           "<p>用于遥测数据接收和视频显示</p>"));
}

void MainWindow::updateStatistics()
{
    // 更新FPS显示
    m_fpsLabel->setText(tr("FPS: %1/%2").arg(m_frameCount1).arg(m_frameCount2));

    // 更新码率显示
    double bitrateKB = m_totalBytes / 1024.0;
    m_bitrateLabel->setText(tr("码率: %1 KB/s").arg(bitrateKB, 0, 'f', 1));

    // 重置计数器
    m_frameCount1 = 0;
    m_frameCount2 = 0;
    m_totalBytes = 0;
}

void MainWindow::onConnectionStatusChanged(bool connected)
{
    if (connected) {
        m_statusLabel->setText(tr("已连接"));
        m_statusLabel->setStyleSheet("color: green;");
        logMessage("INFO", tr("连接成功"));
    } else {
        m_statusLabel->setText(tr("未连接"));
        m_statusLabel->setStyleSheet("color: red;");
        logMessage("WARN", tr("连接断开"));
    }
}

void MainWindow::onVideoFrameReceived(int displayIndex, const QImage &frame)
{
    if (displayIndex == 0) {
        m_videoDisplay1->updateFrame(frame);
        m_frameCount1++;
    } else if (displayIndex == 1) {
        m_videoDisplay2->updateFrame(frame);
        m_frameCount2++;
    }
    m_totalBytes += frame.sizeInBytes();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("退出确认"),
                                  tr("确定要退出地面站客户端吗？"),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        logMessage("INFO", tr("地面站客户端关闭"));
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    // 响应式布局由QSplitter自动处理
}

/**
 * @brief 切换最大化/还原窗口
 */
void MainWindow::toggleMaximize()
{
    // 重置拖拽位置，防止拖拽后点击最大化按钮时误触发拖拽
    m_dragPos = QPoint();
    
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
}

/**
 * @brief 鼠标按下事件处理（用于拖拽窗口）
 * @param event 鼠标事件
 */
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    // 只允许在标题栏区域拖拽窗口
    if (event->button() == Qt::LeftButton && m_titleBar && m_titleBar->rect().contains(event->pos())) {
        m_dragPos = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

/**
 * @brief 鼠标移动事件处理（用于拖拽窗口）
 * @param event 鼠标事件
 */
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    // 只有在有效的拖拽位置时才移动窗口
    if (event->buttons() & Qt::LeftButton && !m_dragPos.isNull()) {
        move(event->globalPos() - m_dragPos);
        event->accept();
    }
}

/**
 * @brief 鼠标释放事件处理（重置拖拽状态）
 * @param event 鼠标事件
 */
void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPos = QPoint(); // 重置拖拽位置
        event->accept();
    }
}
} // namespace ground_station
