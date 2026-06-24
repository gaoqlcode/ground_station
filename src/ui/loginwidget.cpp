/**
 * @file loginwidget.cpp
 * @brief 登录对话框实现
 */

#include "loginwidget.h"

#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QCryptographicHash>

namespace ground_station {

/**
 * @brief 构造函数
 * @param parent 父对象指针
 */
LoginWidget::LoginWidget(QWidget *parent)
    : QDialog(parent)
{
    // 设置对话框属性 - 移除系统标题栏，使用自定义标题栏
    setWindowTitle("地面站客户端 - 登录");
    setFixedSize(400, 500);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    
    // 设置背景色
    setStyleSheet("background-color: #1E1E1E;");
    
    setupUi();
    setupConnections();
    
    // 设置密码输入框为密码模式
    m_passwordEdit->setEchoMode(QLineEdit::Password);
}

/**
 * @brief 析构函数
 */
LoginWidget::~LoginWidget()
{
}

/**
 * @brief 设置UI布局
 */
void LoginWidget::setupUi()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    // 自定义标题栏
    QWidget *titleBar = new QWidget(this);
    titleBar->setFixedHeight(32);
    titleBar->setStyleSheet("background-color: #2D2D30;");
    
    QHBoxLayout *titleBarLayout = new QHBoxLayout(titleBar);
    titleBarLayout->setContentsMargins(10, 0, 10, 0);
    titleBarLayout->setSpacing(0);
    
    // 标题栏图标
    QLabel *titleIcon = new QLabel(titleBar);
    titleIcon->setText("GQL");
    titleIcon->setStyleSheet("color: #0078D4; font-size: 12pt; font-weight: bold;");
    titleBarLayout->addWidget(titleIcon);
    
    // 标题栏文字
    QLabel *titleText = new QLabel("地面站客户端 - 登录", titleBar);
    titleText->setStyleSheet("color: #E0E0E0; font-size: 11pt; padding-left: 8px;");
    titleBarLayout->addWidget(titleText);
    
    // 弹性空间
    titleBarLayout->addStretch();
    
    // 关闭按钮
    QPushButton *closeBtn = new QPushButton(titleBar);
    closeBtn->setFixedSize(36, 28);
    closeBtn->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            border: none;
            color: #B0B0B0;
            font-family: "Segoe UI", "Arial", sans-serif;
            font-size: 18px;
            font-weight: 600;
            text-align: center;
            padding: 0;
            margin: 0;
            line-height: 28px;
        }
        QPushButton:hover {
            background-color: #E81123;
            color: white;
            border-radius: 4px;
        }
    )");
    closeBtn->setText(QChar(0x2715));
    connect(closeBtn, &QPushButton::clicked, this, &LoginWidget::onCancelClicked);
    titleBarLayout->addWidget(closeBtn);
    
    m_mainLayout->addWidget(titleBar);
    
    // 主内容布局
    QWidget *contentWidget = new QWidget(this);
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(40, 20, 40, 40);
    contentLayout->setSpacing(15);
    
    // Logo区域（使用文字代替图标）
    m_logoLabel = new QLabel(this);
    m_logoLabel->setText("GQL");
    m_logoLabel->setAlignment(Qt::AlignCenter);
    m_logoLabel->setStyleSheet(R"(
        QLabel {
            font-size: 48pt;
            font-weight: bold;
            color: #0078D4;
            padding: 20px;
        }
    )");
    contentLayout->addWidget(m_logoLabel);
    
    // 标题
    m_titleLabel = new QLabel("地面站客户端", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(R"(
        QLabel {
            font-size: 18pt;
            font-weight: bold;
            color: #E0E0E0;
            padding-bottom: 20px;
        }
    )");
    contentLayout->addWidget(m_titleLabel);
    
    // 用户名标签
    m_usernameLabel = new QLabel("用户名", this);
    m_usernameLabel->setStyleSheet(R"(
        QLabel {
            font-size: 14pt;
            color: #A0A0A0;
        }
    )");
    contentLayout->addWidget(m_usernameLabel);
    
    // 用户名输入框
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("请输入用户名");
    m_usernameEdit->setMinimumHeight(40);
    m_usernameEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #252526;
            border: 1px solid #3C3C3C;
            border-radius: 4px;
            padding: 8px 12px;
            color: #E0E0E0;
            font-size: 14pt;
        }
        QLineEdit:focus {
            border-color: #0078D4;
        }
        QLineEdit:hover {
            border-color: #505050;
        }
    )");
    contentLayout->addWidget(m_usernameEdit);
    
    // 密码标签
    m_passwordLabel = new QLabel("密码", this);
    m_passwordLabel->setStyleSheet(R"(
        QLabel {
            font-size: 14pt;
            color: #A0A0A0;
            margin-top: 10px;
        }
    )");
    contentLayout->addWidget(m_passwordLabel);
    
    // 密码输入框
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText("请输入密码");
    m_passwordEdit->setMinimumHeight(40);
    m_passwordEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #252526;
            border: 1px solid #3C3C3C;
            border-radius: 4px;
            padding: 8px 12px;
            color: #E0E0E0;
            font-size: 14pt;
        }
        QLineEdit:focus {
            border-color: #0078D4;
        }
        QLineEdit:hover {
            border-color: #505050;
        }
    )");
    contentLayout->addWidget(m_passwordEdit);
    
    // 记住登录复选框
    m_rememberCheckBox = new QCheckBox("记住登录状态", this);
    m_rememberCheckBox->setStyleSheet(R"(
        QCheckBox {
            spacing: 8px;
            color: #A0A0A0;
            font-size: 13pt;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border-radius: 3px;
            border: 2px solid #505050;
        }
        QCheckBox::indicator:checked {
            background-color: #0078D4;
            border-color: #0078D4;
        }
    )");
    contentLayout->addWidget(m_rememberCheckBox);
    
    // 添加弹性空间
    contentLayout->addStretch();
    
    // 按钮布局
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->setSpacing(15);
    
    // 登录按钮
    m_loginBtn = new QPushButton("登录", this);
    m_loginBtn->setMinimumHeight(45);
    m_loginBtn->setDefault(true);
    m_loginBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #0078D4;
            border: none;
            border-radius: 6px;
            color: white;
            font-size: 15pt;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #106EBE;
        }
        QPushButton:pressed {
            background-color: #005A9E;
        }
    )");
    m_buttonLayout->addWidget(m_loginBtn);
    
    // 取消按钮
    m_cancelBtn = new QPushButton("取消", this);
    m_cancelBtn->setMinimumHeight(45);
    m_cancelBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3C3C3C;
            border: none;
            border-radius: 6px;
            color: #E0E0E0;
            font-size: 15pt;
        }
        QPushButton:hover {
            background-color: #4C4C4C;
        }
        QPushButton:pressed {
            background-color: #2C2C2C;
        }
    )");
    m_buttonLayout->addWidget(m_cancelBtn);
    
    contentLayout->addLayout(m_buttonLayout);
    
    // 将内容布局添加到主布局
    m_mainLayout->addWidget(contentWidget);
}

/**
 * @brief 设置信号槽连接
 */
void LoginWidget::setupConnections()
{
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginWidget::onLoginClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &LoginWidget::onCancelClicked);
    
    // 回车键登录
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginWidget::onLoginClicked);
}

/**
 * @brief 登录按钮点击处理
 */
void LoginWidget::onLoginClicked()
{
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();
    
    // 验证凭证
    if (validateCredentials(username, password)) {
        m_username = username;
        m_password = password;
        accept();  // 关闭对话框并返回 Accepted
    }
}

/**
 * @brief 取消按钮点击处理
 */
void LoginWidget::onCancelClicked()
{
    reject();  // 关闭对话框并返回 Rejected
}

/**
 * @brief 验证登录凭证
 * @param username 用户名
 * @param password 密码
 * @return true=验证通过，false=验证失败
 */
bool LoginWidget::validateCredentials(const QString &username, const QString &password)
{
    // 检查是否为空
    if (username.isEmpty()) {
        QMessageBox::warning(this, "登录失败", "用户名不能为空");
        m_usernameEdit->setFocus();
        return false;
    }
    
    if (password.isEmpty()) {
        QMessageBox::warning(this, "登录失败", "密码不能为空");
        m_passwordEdit->setFocus();
        return false;
    }
    
    // TODO: 在这里添加实际的验证逻辑
    // 可以是：
    // 1. 本地配置文件验证
    // 2. 服务器远程验证
    // 3. 数据库验证
    
    // 示例：简单的本地验证（用户名和密码相同）
    // 实际应用中应该连接服务器或读取本地用户数据库
    if (username == password) {
        return true;  // 测试用：用户名和密码相同时通过
    }
    
    // 如果验证失败，显示错误信息
    QMessageBox::warning(this, "登录失败", "用户名或密码错误");
    m_passwordEdit->clear();
    m_passwordEdit->setFocus();
    return false;
}

} // namespace ground_station
