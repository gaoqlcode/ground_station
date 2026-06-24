/**
 * @file main.cpp
 * @brief 地面站客户端程序入口
 *
 * 本文件是地面站客户端的主程序入口，提供现代化 GUI 界面。
 * 主要功能包括：
 * - 应用程序初始化
 * - 登录验证（可选）
 * - 主窗口显示
 * - 事件循环运行
 *
 * @author GroundStation Team
 * @version 1.0
 * @date 2024
 */

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QStyleFactory>
#include <QMessageBox>
#include <QDialog>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "mainwindow.h"
#include "logger.h"
#include "loginwidget.h"

// ============================================================================
// 登录功能配置
// ============================================================================

/// 是否启用登录界面（使用宏定义，支持预处理器判断）
/// 设置为 1 = 启动时显示登录对话框
/// 设置为 0 = 跳过登录，直接进入主界面
#define ENABLE_LOGIN 1

#if ENABLE_LOGIN
#include <QInputDialog>
#include <QCryptographicHash>
#include <QFile>
#include <QSettings>

/// 保存登录凭证到本地文件
/// @param username 用户名
/// @param password 密码（未加密，仅用于测试）
static void saveCredentials(const QString &username, const QString &password)
{
    QString configPath = QCoreApplication::applicationDirPath() + "/login.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    
    // 简单加密（仅防止明文显示，实际使用应使用更安全的方式）
    QString encodedPassword = QString(QCryptographicHash::hash(
        (password + "ground_station_salt").toUtf8(), 
        QCryptographicHash::Md5).toHex());
    
    settings.beginGroup("Credentials");
    settings.setValue("username", username);
    settings.setValue("password", encodedPassword);
    settings.setValue("remember", true);
    settings.endGroup();
}

/// 加载保存的登录凭证
/// @param username 输出用户名
/// @param password 输出密码
/// @return true=有保存的凭证，false=没有保存的凭证
static bool loadCredentials(QString &username, QString &password)
{
    QString configPath = QCoreApplication::applicationDirPath() + "/login.ini";
    QFile file(configPath);
    if (!file.exists()) {
        return false;
    }
    
    QSettings settings(configPath, QSettings::IniFormat);
    if (!settings.contains("Credentials/remember")) {
        return false;
    }
    
    username = settings.value("Credentials/username").toString();
    password = settings.value("Credentials/password").toString();
    return true;
}

/// 清除保存的登录凭证
static void clearCredentials()
{
    QString configPath = QCoreApplication::applicationDirPath() + "/login.ini";
    QFile file(configPath);
    if (file.exists()) {
        file.remove();
    }
}

/// 验证登录凭证
/// @param username 用户名
/// @param password 密码
/// @return true=验证通过，false=验证失败
static bool verifyCredentials(const QString &username, const QString &password)
{
    // TODO: 在这里实现实际的验证逻辑
    // 可以是：
    // 1. 连接远程服务器验证
    // 2. 读取本地用户数据库验证
    // 3. LDAP/Active Directory 验证
    
    // 示例：简单验证 - 用户名和密码相同则通过
    // 实际应用中请替换为真正的验证逻辑
    if (username == password) {
        return true;
    }
    
    return false;
}

#endif // ENABLE_LOGIN

/*
@brief 应用现代化 Fluent Design 样式
*/
void applyModernStyle(QApplication& app)
{
    // 设置 Fusion 风格为基础
    app.setStyle(QStyleFactory::create("Fusion"));

    // 定义现代化配色方案 (深色主题)
    QColor primaryColor("#0078D4");      // Windows 蓝色
    QColor accentColor("#106EBE");       // 强调色
    QColor backgroundColor("#1E1E1E");   // 深色背景
    QColor surfaceColor("#252526");      // 卡片/表面
    QColor borderColor("#3C3C3C");       // 边框
    QColor textColor("#E0E0E0");         // 主文本
    QColor textSecondary("#A0A0A0");     // 次要文本

    // 设置全局调色板
    QPalette palette;
    palette.setColor(QPalette::Window, backgroundColor);
    palette.setColor(QPalette::WindowText, textColor);
    palette.setColor(QPalette::Base, surfaceColor);
    palette.setColor(QPalette::AlternateBase, backgroundColor);
    palette.setColor(QPalette::ToolTipBase, surfaceColor);
    palette.setColor(QPalette::ToolTipText, textColor);
    palette.setColor(QPalette::Text, textColor);
    palette.setColor(QPalette::Button, surfaceColor);
    palette.setColor(QPalette::ButtonText, textColor);
    palette.setColor(QPalette::BrightText, Qt::white);
    palette.setColor(QPalette::Highlight, primaryColor);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Link, accentColor);
    app.setPalette(palette);

    // 设置全局字体（pt 单位随 DPI 缩放，比样式表 px 更可靠）
    QFont appFont(QStringLiteral("Microsoft YaHei UI"), 11);
    appFont.setStyleHint(QFont::SansSerif);
    appFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(appFont);

    // 应用现代化样式表
    app.setStyleSheet(R"(
        /* 全局字体设置 - 使用适中字体，适配室内环境 */
        QWidget {
            font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI", sans-serif;
            font-size: 11pt;
        }

        /* 主窗口 */
        QMainWindow {
            background-color: #1E1E1E;
        }

        /* 菜单栏 */
        QMenuBar {
            background-color: #252526;
            border-bottom: 1px solid #3C3C3C;
            padding: 8px;
            font-size: 14pt;
        }
        QMenuBar::item {
            background: transparent;
            padding: 10px 18px;
            border-radius: 4px;
            color: #E0E0E0;
        }
        QMenuBar::item:selected {
            background-color: #3C3C3C;
        }
        QMenuBar::item:pressed {
            background-color: #0078D4;
        }

        /* 菜单 */
        QMenu {
            background-color: #252526;
            border: 1px solid #3C3C3C;
            border-radius: 4px;
            padding: 8px;
            font-size: 14pt;
        }
        QMenu::item {
            padding: 12px 32px;
            border-radius: 4px;
            color: #E0E0E0;
        }
        QMenu::item:selected {
            background-color: #0078D4;
        }
        QMenu::separator {
            height: 1px;
            background-color: #3C3C3C;
            margin: 4px 8px;
        }

        /* 工具栏 */
        QToolBar {
            background-color: #252526;
            border-bottom: 1px solid #3C3C3C;
            padding: 8px;
            spacing: 8px;
            font-size: 14pt;
        }
        QToolButton {
            background-color: transparent;
            border: none;
            border-radius: 4px;
            padding: 12px 16px;
            color: #E0E0E0;
            font-size: 14pt;
        }
        QToolButton:hover {
            background-color: #3C3C3C;
        }
        QToolButton:pressed {
            background-color: #0078D4;
        }

        /* 按钮 */
        QPushButton {
            background-color: #0078D4;
            border: none;
            border-radius: 6px;
            padding: 8px 16px;
            color: white;
            font-weight: 600;
            font-size: 11pt;
            min-height: 28px;
        }
        QPushButton:hover {
            background-color: #106EBE;
        }
        QPushButton:pressed {
            background-color: #005A9E;
        }
        QPushButton:disabled {
            background-color: #3C3C3C;
            color: #666666;
        }
        QPushButton[flat="true"] {
            background-color: transparent;
            color: #E0E0E0;
        }
        QPushButton[flat="true"]:hover {
            background-color: #3C3C3C;
        }

        /* 输入框 */
        QLineEdit {
            background-color: #1E1E1E;
            border: 1px solid #3C3C3C;
            border-radius: 4px;
            padding: 6px 10px;
            color: #E0E0E0;
            selection-background-color: #0078D4;
            font-size: 11pt;
            min-height: 28px;
        }
        QLineEdit:focus {
            border-color: #0078D4;
        }
        QLineEdit:hover {
            border-color: #505050;
        }

        /* 文本编辑 */
        QTextEdit {
            background-color: #1E1E1E;
            border: 1px solid #3C3C3C;
            border-radius: 4px;
            padding: 8px;
            color: #D4D4D4;
            font-family: "Consolas", "Monaco", "Microsoft YaHei", monospace;
            font-size: 10pt;
            selection-background-color: #264F78;
        }

        /* 列表/树 */
        QListView, QTreeView, QTreeWidget {
            background-color: #252526;
            border: 1px solid #3C3C3C;
            border-radius: 4px;
            outline: none;
            padding: 6px;
            font-size: 11pt;
        }
        QListView::item, QTreeView::item, QTreeWidget::item {
            padding: 8px 12px;
            border-radius: 4px;
            color: #E0E0E0;
            min-height: 26px;
        }
        QHeaderView::section {
            background-color: #252526;
            color: #E0E0E0;
            padding: 6px 10px;
            border: none;
            border-bottom: 1px solid #3C3C3C;
            font-size: 11pt;
            font-weight: bold;
        }
        QListView::item:selected, QTreeView::item:selected, QTreeWidget::item:selected {
            background-color: #37373D;
        }
        QListView::item:hover, QTreeView::item:hover, QTreeWidget::item:hover {
            background-color: #2A2D2E;
        }
        QListView::item:selected:hover {
            background-color: #37373D;
        }

        /* 分割器 */
        QSplitter::handle {
            background-color: #3C3C3C;
        }
        QSplitter::handle:horizontal {
            width: 2px;
        }
        QSplitter::handle:vertical {
            height: 2px;
        }
        QSplitter::handle:hover {
            background-color: #0078D4;
        }

        /* 状态栏 */
        QStatusBar {
            background-color: #0078D4;
            color: white;
            border-top: none;
            font-size: 11pt;
        }
        QStatusBar::item {
            border: none;
        }
        QStatusBar QLabel {
            color: white;
            padding: 6px 12px;
            font-size: 11pt;
        }

        /* 分组框 */
        QGroupBox {
            background-color: #252526;
            border: 1px solid #3C3C3C;
            border-radius: 6px;
            margin-top: 20px;
            padding-top: 20px;
            font-weight: bold;
            color: #E0E0E0;
            font-size: 16pt;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 16px;
            padding: 4px 12px;
            color: #E0E0E0;
            font-size: 16pt;
            font-weight: bold;
            background-color: #0078D4;
            border-radius: 4px;
        }

        /* 标签 */
        QLabel {
            color: #E0E0E0;
            font-size: 11pt;
        }
        QLabel[heading="true"] {
            font-size: 14pt;
            font-weight: bold;
            color: #E0E0E0;
        }
        QLabel[caption="true"] {
            font-size: 10pt;
            color: #A0A0A0;
        }

        /* 滚动条 */
        QScrollBar:vertical {
            background-color: #1E1E1E;
            width: 12px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical {
            background-color: #424242;
            border-radius: 6px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #4F4F4F;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            background-color: #1E1E1E;
            height: 12px;
            border-radius: 6px;
        }
        QScrollBar::handle:horizontal {
            background-color: #424242;
            border-radius: 6px;
            min-width: 30px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: #4F4F4F;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }

        /* 复选框 */
        QCheckBox {
            spacing: 8px;
            color: #E0E0E0;
            font-size: 11pt;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border-radius: 3px;
            border: 2px solid #606060;
            background-color: transparent;
        }
        QCheckBox::indicator:checked {
            background-color: #0078D4;
            border-color: #0078D4;
        }
        QCheckBox::indicator:hover {
            border-color: #808080;
        }

        /* 单选按钮 */
        QRadioButton {
            spacing: 8px;
            color: #E0E0E0;
            font-size: 11pt;
        }
        QRadioButton::indicator {
            width: 16px;
            height: 16px;
            border-radius: 8px;
            border: 2px solid #606060;
            background-color: transparent;
        }
        QRadioButton::indicator:checked {
            background-color: #0078D4;
            border-color: #0078D4;
        }

        /* 下拉框 */
        QComboBox {
            background-color: #1E1E1E;
            border: 1px solid #3C3C3C;
            border-radius: 4px;
            padding: 6px 10px;
            color: #E0E0E0;
            min-width: 100px;
            font-size: 11pt;
            min-height: 28px;
        }
        QComboBox:hover {
            border-color: #505050;
        }
        QComboBox:focus {
            border-color: #0078D4;
        }
        QComboBox::drop-down {
            border: none;
            width: 24px;
        }
        QComboBox::down-arrow {
            image: none;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 6px solid #E0E0E0;
        }
        QComboBox QAbstractItemView {
            background-color: #252526;
            border: 1px solid #3C3C3C;
            border-radius: 4px;
            selection-background-color: #0078D4;
        }

        /* Tab 控件 */
        QTabWidget::pane {
            border: 1px solid #3C3C3C;
            border-radius: 4px;
            background-color: #252526;
            top: -1px;
        }
        QTabBar::tab {
            background-color: #1E1E1E;
            border: 1px solid #3C3C3C;
            border-bottom: none;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            padding: 8px 16px;
            margin-right: 2px;
            color: #A0A0A0;
            font-size: 11pt;
        }
        QTabBar::tab:selected {
            background-color: #252526;
            color: #E0E0E0;
            border-bottom: 2px solid #0078D4;
        }
        QTabBar::tab:hover:!selected {
            background-color: #2D2D30;
            color: #E0E0E0;
        }

        /* 工具提示 */
        QToolTip {
            background-color: #252526;
            border: 1px solid #3C3C3C;
            border-radius: 4px;
            padding: 10px 16px;
            color: #E0E0E0;
            font-size: 13pt;
        }

        /* 对话框 */
        QDialog {
            background-color: #1E1E1E;
        }
        
        /* 自定义标题栏 */
        #customTitleBar {
            background-color: #2D2D30;
            min-height: 28px;
            max-height: 28px;
        }
        #customTitleBar QLabel {
            color: #E0E0E0;
            font-size: 10pt;
        }
        #customTitleBar #titleIcon {
            color: #0078D4;
            font-size: 11pt;
            font-weight: bold;
        }
        
        /* 窗口控制按钮 - 最小化 */
        #customTitleBar #windowMinBtn {
            background-color: transparent;
            border: none;
            color: #B0B0B0;
            padding: 0;
            width: 36px;
            height: 26px;
            font-size: 14pt;
            font-weight: bold;
            text-align: center;
        }
        #customTitleBar #windowMinBtn:hover {
            background-color: #3C3C3C;
            color: #FFFFFF;
        }
        
        /* 窗口控制按钮 - 最大化 */
        #customTitleBar #windowMaxBtn {
            background-color: transparent;
            border: none;
            color: #B0B0B0;
            padding: 0;
            width: 36px;
            height: 26px;
            font-size: 11pt;
            font-weight: bold;
            text-align: center;
        }
        #customTitleBar #windowMaxBtn:hover {
            background-color: #3C3C3C;
            color: #FFFFFF;
        }
        
        /* 窗口控制按钮 - 关闭 */
        #customTitleBar #windowCloseBtn {
            background-color: transparent;
            border: none;
            color: #B0B0B0;
            padding: 0;
            width: 36px;
            height: 26px;
            font-family: "Segoe UI", "Arial", sans-serif;
            font-size: 16px;
            font-weight: 600;
            text-align: center;
            line-height: 26px;
        }
        #customTitleBar #windowCloseBtn:hover {
            background-color: #E81123;
            color: #FFFFFF;
        }
        
        /* 自定义菜单栏 */
        #customMenuBar {
            background-color: #252526;
            min-height: 24px;
            max-height: 24px;
            border: none;
        }
        #customMenuBar QPushButton {
            background-color: transparent;
            border: none;
            color: #C0C0C0;
            font-size: 9pt;
            width: 60px;
            height: 24px;
            text-align: left;
            padding: 0;
            margin: 0;
        }
        #customMenuBar QPushButton:hover {
            background-color: #3C3C3C;
        }
        #customMenuBar QLabel {
            color: #404040;
            font-size: 9pt;
            padding: 0;
            margin: 0;
        }

        /* 进度条 */
        QProgressBar {
            background-color: #252526;
            border: 1px solid #3C3C3C;
            border-radius: 4px;
            text-align: center;
            color: #E0E0E0;
        }
        QProgressBar::chunk {
            background-color: #0078D4;
            border-radius: 3px;
        }

        /* 滑块 */
        QSlider::groove:horizontal {
            height: 4px;
            background-color: #3C3C3C;
            border-radius: 2px;
        }
        QSlider::sub-page:horizontal {
            background-color: #0078D4;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background-color: #E0E0E0;
            width: 16px;
            height: 16px;
            margin: -6px 0;
            border-radius: 8px;
        }
        QSlider::handle:horizontal:hover {
            background-color: white;
        }
    )");
}

/**
 * @brief 初始化控制台编码（Windows平台）
 * 
 * 在程序启动时设置控制台为UTF-8编码，确保中文输出正确
 */
void initializeConsoleEncoding() {
#ifdef Q_OS_WIN
    // 设置控制台输入输出代码页为UTF-8
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
}

int main(int argc, char* argv[])
{
    // 首先初始化控制台编码（必须在任何中文输出之前）
    initializeConsoleEncoding();

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);

    app.setApplicationName("GroundStationClient");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("GqlSystem");
    app.setOrganizationDomain("gql.system");

    // 应用现代化样式
    applyModernStyle(app);

    // 初始化日志系统
    QString logDir = app.applicationDirPath() + "/logs";
    QDir().mkpath(logDir);
    QString logPath = logDir + "/ground_station.log";
    ground_station::Logger::instance().initialize(logPath);

    LOG_INFO("Main", "地面站客户端 GUI 启动中...");

    // ========================================================================
    // 登录验证
    // ========================================================================
#if ENABLE_LOGIN
    bool loginSuccess = false;
    QString username;
    QString password;
    
    // 如果有保存的凭证，尝试自动登录
    if (loadCredentials(username, password)) {
        // 可以在这里实现自动登录逻辑
        // 为了安全起见，暂时不实现自动登录
        LOG_INFO("Main", "检测到保存的登录凭证");
    }
    
    // 显示登录对话框
    ground_station::LoginWidget loginDialog;
    
    // 如果有保存的用户名，设置到对话框
    if (!username.isEmpty()) {
        loginDialog.setUsername(username);
        loginDialog.setRemembered(true);
    }
    
    if (loginDialog.exec() == QDialog::Accepted) {
        username = loginDialog.getUsername();
        password = loginDialog.getPassword();
        
        // 验证登录凭证
        if (verifyCredentials(username, password)) {
            loginSuccess = true;
            LOG_INFO("Main", QString("用户登录成功: %1").arg(username).toStdString().c_str());
            
            // 如果选择记住登录状态，保存凭证
            if (loginDialog.isRemembered()) {
                saveCredentials(username, password);
                LOG_INFO("Main", "已保存登录凭证");
            }
        } else {
            LOG_WARN("Main", "登录凭证验证失败");
            QMessageBox::critical(nullptr, "登录失败", "用户名或密码错误，程序将退出。");
            ground_station::Logger::instance().shutdown();
            return 1;
        }
    } else {
        LOG_INFO("Main", "用户取消登录，程序退出");
        ground_station::Logger::instance().shutdown();
        return 0;
    }
#else
    LOG_INFO("Main", "登录功能已禁用（ENABLE_LOGIN=false）");
#endif // ENABLE_LOGIN

    // 创建并显示主窗口（默认最大化）
    ground_station::MainWindow window;
    window.showMaximized();

    LOG_INFO("Main", "主窗口已显示");

    int result = app.exec();

    LOG_INFO("Main", "地面站客户端已停止");
    ground_station::Logger::instance().shutdown();

    return result;
}
