/**
 * @file loginwidget.h
 * @brief 登录对话框头文件
 *
 * 提供用户登录界面，支持：
 * - 用户名/密码输入
 * - 记住登录状态
 * - 登录验证
 */

#ifndef GROUND_STATION_LOGINWIDGET_H
#define GROUND_STATION_LOGINWIDGET_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>

namespace ground_station {

/**
 * @class LoginWidget
 * @brief 登录对话框类
 *
 * 提供用户登录界面，包含用户名、密码输入和登录按钮
 */
class LoginWidget : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit LoginWidget(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~LoginWidget();

    /**
     * @brief 获取登录的用户名
     * @return 用户名
     */
    QString getUsername() const { return m_username; }
    
    /**
     * @brief 获取登录密码
     * @return 密码
     */
    QString getPassword() const { return m_password; }
    
    /**
     * @brief 是否记住登录状态
     * @return true=记住，false=不记住
     */
    bool isRemembered() const { return m_rememberCheckBox->isChecked(); }
    
    /**
     * @brief 设置用户名
     * @param username 用户名
     */
    void setUsername(const QString &username) { m_usernameEdit->setText(username); }
    
    /**
     * @brief 设置记住登录状态
     * @param remember true=记住，false=不记住
     */
    void setRemembered(bool remember) { m_rememberCheckBox->setChecked(remember); }

signals:
    /**
     * @brief 登录成功信号
     * @param username 用户名
     * @param password 密码
     */
    void loginSuccess(const QString &username, const QString &password);

private slots:
    /**
     * @brief 登录按钮点击处理
     */
    void onLoginClicked();
    
    /**
     * @brief 取消按钮点击处理
     */
    void onCancelClicked();

private:
    /**
     * @brief 设置UI布局
     */
    void setupUi();
    
    /**
     * @brief 设置信号槽连接
     */
    void setupConnections();
    
    /**
     * @brief 验证登录凭证
     * @param username 用户名
     * @param password 密码
     * @return true=验证通过，false=验证失败
     */
    bool validateCredentials(const QString &username, const QString &password);

private:
    QVBoxLayout *m_mainLayout;      ///< 主布局
    QLabel *m_logoLabel;            ///< Logo标签
    QLabel *m_titleLabel;           ///< 标题标签
    QLabel *m_usernameLabel;        ///< 用户名标签
    QLineEdit *m_usernameEdit;      ///< 用户名输入框
    QLabel *m_passwordLabel;        ///< 密码标签
    QLineEdit *m_passwordEdit;      ///< 密码输入框
    QCheckBox *m_rememberCheckBox;  ///< 记住登录复选框
    QPushButton *m_loginBtn;       ///< 登录按钮
    QPushButton *m_cancelBtn;      ///< 取消按钮
    QHBoxLayout *m_buttonLayout;   ///< 按钮布局
    
    QString m_username;             ///< 登录用户名
    QString m_password;             ///< 登录密码
};

} // namespace ground_station

#endif // GROUND_STATION_LOGINWIDGET_H
