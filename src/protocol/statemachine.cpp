/**
 * @file statemachine.cpp
 * @brief 地面站客户端状态机实现
 *
 * 实现客户端状态管理，包括：
 * - 状态转换规则定义
 * - 状态超时处理
 * - 状态转换验证
 * - 状态变更通知（信号机制）
 *
 * 状态机：
 * Disconnected -> Connecting -> Connected -> Registering -> Streaming
 *      ^              |              |               |          |
 *      |              +--------------+---------------+----------+
 *      +------------------------------------------------------+ (断开/超时/错误)
 *                              |
 *                              v
 *                           Error -> Disconnected
 *
 * 状态超时时间：
 * - Connecting: 5秒
 * - Registering: 5秒
 * - Streaming: 10秒
 */

#include "statemachine.h"
#include "logger.h"

namespace ground_station {

/**
 * @brief 构造函数
 * @param parent 父对象指针
 */
StateMachine::StateMachine(QObject* parent)
    : QObject(parent)
    , m_currentState(ClientState::Disconnected)
    , m_stateTimer(new QTimer(this))
    , m_running(false) {

    // 初始化状态转换规则
    initializeTransitions();

    // 连接超时计时器信号
    connect(m_stateTimer, &QTimer::timeout, this, &StateMachine::handleTimeout);
}

/**
 * @brief 析构函数
 */
StateMachine::~StateMachine() {
    stop();
}

/**
 * @brief 初始化状态转换规则
 *
 * 定义所有合法的状态转换路径。
 */
void StateMachine::initializeTransitions() {
    // Disconnected 可以转换到 Connecting
    m_validTransitions[ClientState::Disconnected] = {
        ClientState::Connecting
    };

    // Connecting 可以转换到 Connected、Disconnected 或 Error
    m_validTransitions[ClientState::Connecting] = {
        ClientState::Connected,
        ClientState::Disconnected,
        ClientState::Error
    };

    // Connected 可以转换到 Registering、Disconnected 或 Error
    m_validTransitions[ClientState::Connected] = {
        ClientState::Registering,
        ClientState::Disconnected,
        ClientState::Error
    };

    // Registering 可以转换到 Streaming、Connected、Disconnected 或 Error
    m_validTransitions[ClientState::Registering] = {
        ClientState::Streaming,
        ClientState::Connected,
        ClientState::Disconnected,
        ClientState::Error
    };

    // Streaming 可以转换到 Connected、Disconnected 或 Error
    m_validTransitions[ClientState::Streaming] = {
        ClientState::Connected,
        ClientState::Disconnected,
        ClientState::Error
    };

    // Error 可以转换到 Disconnected（错误状态只能回到断开连接）
    m_validTransitions[ClientState::Error] = {
        ClientState::Disconnected
    };

    // 设置默认超时时间
    m_stateTimeouts[ClientState::Connecting] = 5000;   // 5秒
    m_stateTimeouts[ClientState::Registering] = 5000;  // 5秒
    m_stateTimeouts[ClientState::Streaming] = 10000;   // 10秒
}

/**
 * @brief 获取当前状态
 * @return 当前客户端状态
 */
ClientState StateMachine::getCurrentState() const {
    return m_currentState;
}

/**
 * @brief 将状态转换为可读字符串
 * @param state 客户端状态
 * @return 状态名称字符串
 */
QString StateMachine::getStateName(ClientState state) const {
    switch (state) {
        case ClientState::Disconnected:
            return "Disconnected";
        case ClientState::Connecting:
            return "Connecting";
        case ClientState::Connected:
            return "Connected";
        case ClientState::Registering:
            return "Registering";
        case ClientState::Streaming:
            return "Streaming";
        case ClientState::Error:
            return "Error";
        default:
            return "Unknown";
    }
}

/**
 * @brief 启动状态机
 */
void StateMachine::start() {
    if (m_running) {
        return;
    }

    m_running = true;
    m_currentState = ClientState::Disconnected;
    LOG_INFO("状态机", "状态机已启动");
    emit stateEntered(m_currentState);
}

/**
 * @brief 停止状态机
 */
void StateMachine::stop() {
    if (!m_running) {
        return;
    }

    stopStateTimer();
    m_running = false;
    m_currentState = ClientState::Disconnected;
    LOG_INFO("状态机", "状态机已停止");
}

/**
 * @brief 重置状态机到初始状态
 */
void StateMachine::reset() {
    stopStateTimer();
    ClientState oldState = m_currentState;
    m_currentState = ClientState::Disconnected;

    if (oldState != m_currentState) {
        emit stateExited(oldState);
        emit stateEntered(m_currentState);
        emit stateChanged(oldState, m_currentState);
    }

    LOG_INFO("状态机", "状态机已重置为断开连接");
}

/**
 * @brief 请求状态转换
 *
 * 验证状态转换的合法性，合法则执行转换。
 *
 * @param newState 目标状态
 * @return 是否转换成功
 */
bool StateMachine::requestStateTransition(ClientState newState) {
    if (!m_running) {
        LOG_WARN("状态机", "状态机未运行");
        return false;
    }

    // 验证状态转换合法性
    if (!isValidTransition(m_currentState, newState)) {
        QString errorMsg = QString("无效的状态转换: %1 -> %2")
                               .arg(getStateName(m_currentState))
                               .arg(getStateName(newState));
        LOG_ERROR("状态机", errorMsg);
        emit errorOccurred(errorMsg);
        return false;
    }

    // 执行状态转换
    ClientState oldState = m_currentState;
    setCurrentState(newState);

    return true;
}

/**
 * @brief 设置指定状态的超时时间
 * @param state 状态
 * @param timeout 超时时间（毫秒）
 */
void StateMachine::setStateTimeout(ClientState state, int timeout) {
    m_stateTimeouts[state] = timeout;
}

/**
 * @brief 设置当前状态
 *
 * 执行状态转换，发送状态变更信号。
 *
 * @param newState 新状态
 */
void StateMachine::setCurrentState(ClientState newState) {
    if (m_currentState == newState) {
        return;
    }

    ClientState oldState = m_currentState;

    LOG_INFO("状态机", QString("状态转换: %1 -> %2")
                                .arg(getStateName(oldState))
                                .arg(getStateName(newState)));

    // 发送状态退出信号
    emit stateExited(oldState);
    
    // 更新当前状态
    m_currentState = newState;
    
    // 发送状态进入和状态变更信号
    emit stateEntered(newState);
    emit stateChanged(oldState, newState);

    // 启动新状态的超时计时器
    startStateTimer();
}

/**
 * @brief 验证状态转换是否合法
 * @param from 源状态
 * @param to 目标状态
 * @return 是否合法
 */
bool StateMachine::isValidTransition(ClientState from, ClientState to) const {
    if (from == to) {
        return true;  // 允许停留在同一状态
    }

    if (m_validTransitions.contains(from)) {
        return m_validTransitions[from].contains(to);
    }

    return false;
}

/**
 * @brief 启动状态超时计时器
 */
void StateMachine::startStateTimer() {
    stopStateTimer();

    if (m_stateTimeouts.contains(m_currentState)) {
        int timeout = m_stateTimeouts[m_currentState];
        if (timeout > 0) {
            m_stateTimer->start(timeout);
            LOG_DEBUG("状态机", QString("状态计时器已启动，当前状态: %1，超时: %2ms")
                                           .arg(getStateName(m_currentState))
                                           .arg(timeout));
        }
    }
}

/**
 * @brief 停止状态超时计时器
 */
void StateMachine::stopStateTimer() {
    if (m_stateTimer->isActive()) {
        m_stateTimer->stop();
        LOG_DEBUG("状态机", "状态计时器已停止");
    }
}

/**
 * @brief 处理状态超时
 *
 * 状态超时后发送超时信号，并转换到错误状态（除非已经是断开连接状态）。
 */
void StateMachine::handleTimeout() {
    stopStateTimer();

    LOG_WARN("状态机", QString("状态超时: %1")
                                  .arg(getStateName(m_currentState)));

    emit stateTimeout(m_currentState);

    // 超时后转换到错误状态（断开连接状态不转换）
    if (m_currentState != ClientState::Disconnected) {
        requestStateTransition(ClientState::Error);
    }
}

}  // namespace ground_station