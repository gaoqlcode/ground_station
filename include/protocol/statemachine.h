/**
 * @file statemachine.h
 * @brief 地面站客户端状态机
 *
 * 定义客户端状态转换逻辑：Disconnected → Connecting → Connected → Registering → Streaming
 */

#ifndef GROUND_STATION_STATEMACHINE_H
#define GROUND_STATION_STATEMACHINE_H

#include <QObject>
#include <QStateMachine>
#include <QState>
#include <QFinalState>
#include <QTimer>
#include <QMap>
#include <memory>

namespace ground_station {

/**
 * @enum ClientState
 * @brief 客户端状态枚举
 */
enum class ClientState {
    Disconnected,   // 断开连接
    Connecting,      // 连接中
    Connected,       // 已连接
    Registering,     // 注册中
    Streaming,       // 流传输中
    Error            // 错误状态
};

/**
 * @class StateMachine
 * @brief 客户端状态机类
 *
 * 管理客户端的状态转换，确保状态变更的合法性
 * 提供状态变更通知和超时处理
 */
class StateMachine : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit StateMachine(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~StateMachine() override;

    /**
     * @brief 获取当前状态
     * @return 当前状态
     */
    ClientState getCurrentState() const;

    /**
     * @brief 获取状态名称
     * @param state 状态枚举
     * @return 状态名称字符串
     */
    QString getStateName(ClientState state) const;

    /**
     * @brief 启动状态机
     */
    void start();

    /**
     * @brief 停止状态机
     */
    void stop();

    /**
     * @brief 重置状态机到初始状态
     */
    void reset();

    /**
     * @brief 请求状态转换
     * @param newState 目标状态
     * @return 是否允许转换
     */
    bool requestStateTransition(ClientState newState);

    /**
     * @brief 设置状态超时时间
     * @param state 状态
     * @param timeout 超时时间（毫秒）
     */
    void setStateTimeout(ClientState state, int timeout);

signals:
    /**
     * @brief 状态变更信号
     * @param oldState 旧状态
     * @param newState 新状态
     */
    void stateChanged(ClientState oldState, ClientState newState);

    /**
     * @brief 进入状态信号
     * @param state 进入的状态
     */
    void stateEntered(ClientState state);

    /**
     * @brief 离开状态信号
     * @param state 离开的状态
     */
    void stateExited(ClientState state);

    /**
     * @brief 状态超时信号
     * @param state 超时的状态
     */
    void stateTimeout(ClientState state);

    /**
     * @brief 错误信号
     * @param errorMessage 错误消息
     */
    void errorOccurred(const QString& errorMessage);

private slots:
    /**
     * @brief 处理状态超时
     */
    void handleTimeout();

private:
    /**
     * @brief 初始化状态转换规则
     */
    void initializeTransitions();

    /**
     * @brief 检查状态转换是否合法
     * @param from 源状态
     * @param to 目标状态
     * @return 是否合法
     */
    bool isValidTransition(ClientState from, ClientState to) const;

    /**
     * @brief 设置当前状态
     * @param newState 新状态
     */
    void setCurrentState(ClientState newState);

    /**
     * @brief 启动状态超时计时器
     */
    void startStateTimer();

    /**
     * @brief 停止状态超时计时器
     */
    void stopStateTimer();

    ClientState m_currentState;                    // 当前状态
    QMap<ClientState, int> m_stateTimeouts;        // 状态超时配置
    QMap<ClientState, QList<ClientState>> m_validTransitions;  // 合法状态转换映射
    QTimer* m_stateTimer;                          // 状态超时计时器
    bool m_running;                                // 状态机是否运行中
};

}  // namespace ground_station

#endif  // GROUND_STATION_STATEMACHINE_H