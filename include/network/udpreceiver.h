/**
 * @file udpreceiver.h
 * @brief UDP视频接收器头文件
 *
 * 提供UDP视频流接收功能，包括：
 * - UDP socket管理
 * - 数据包接收
 * - 帧重组集成
 * - 统计信息
 */

#ifndef GROUND_STATION_UDPRECEIVER_H
#define GROUND_STATION_UDPRECEIVER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QTimer>
#include <QMutex>
#include <cstdint>

#include "packetreassembly.h"

namespace ground_station {

// ============================================================================
// 常量定义
// ============================================================================

/// 默认UDP接收端口
constexpr int DEFAULT_UDP_PORT = 8889;

/// 默认接收缓冲区大小
constexpr int DEFAULT_BUFFER_SIZE = 1024 * 1024;  // 1MB

/// 默认心跳间隔（毫秒）
constexpr std::uint32_t DEFAULT_HEARTBEAT_INTERVAL_MS = 5000;

/// 默认统计更新间隔（毫秒）
constexpr std::uint32_t DEFAULT_STATS_INTERVAL_MS = 1000;

// ============================================================================
// 枚举类型
// ============================================================================

/**
 * @enum ReceiverState
 * @brief 接收器状态枚举
 */
enum class ReceiverState {
    Idle,           ///< 空闲
    Binding,        ///< 绑定端口中
    Listening,      ///< 监听中
    Receiving,      ///< 接收中
    Error           ///< 错误
};

/**
 * @enum ReceiverError
 * @brief 接收器错误类型
 */
enum class ReceiverError {
    None = 0,               ///< 无错误
    SocketError,            ///< Socket错误
    BindError,              ///< 端口绑定错误
    BufferOverflow,         ///< 缓冲区溢出
    ReassemblyError,        ///< 重组错误
    TimeoutError,           ///< 超时错误
    NetworkError            ///< 网络错误
};

// ============================================================================
// 结构体定义
// ============================================================================

/**
 * @struct ReceiverStats
 * @brief 接收器统计信息
 */
struct ReceiverStats {
    std::uint64_t total_packets = 0;        ///< 总接收包数
    std::uint64_t total_bytes = 0;          ///< 总接收字节数
    std::uint64_t total_frames = 0;         ///< 总接收帧数
    std::uint64_t complete_frames = 0;      ///< 完整帧数
    std::uint64_t incomplete_frames = 0;    ///< 不完整帧数
    std::uint64_t timeout_frames = 0;       ///< 超时帧数
    std::uint64_t crc_errors = 0;           ///< CRC错误数
    std::uint64_t invalid_packets = 0;      ///< 无效包数
    
    double packets_per_second = 0.0;        ///< 每秒包数
    double bytes_per_second = 0.0;          ///< 每秒字节数
    double frames_per_second = 0.0;         ///< 每秒帧数
    double avg_frame_size = 0.0;            ///< 平均帧大小
    
    double packet_loss_rate = 0.0;          ///< 丢包率
    double avg_reassembly_time_ms = 0.0;    ///< 平均重组时间
    
    QString last_error;                     ///< 最后错误信息
};

// ============================================================================
// UdpReceiver 类
// ============================================================================

/**
 * @class UdpReceiver
 * @brief UDP视频接收器类
 *
 * 实现UDP视频流接收功能，包括：
 * - UDP socket管理
 * - 数据包接收
 * - 帧重组集成
 * - 统计信息收集
 *
 * 使用方式：
 * 1. 创建 UdpReceiver 实例
 * 2. 连接 frameReceived 信号
 * 3. 调用 start() 启动接收
 * 4. 调用 stop() 停止接收
 */
class UdpReceiver : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit UdpReceiver(QObject* parent = nullptr);

    /**
     * @brief 构造函数
     * @param port 本地端口
     * @param parent 父对象
     */
    explicit UdpReceiver(int port, QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~UdpReceiver() override;

    // 禁用拷贝
    UdpReceiver(const UdpReceiver&) = delete;
    UdpReceiver& operator=(const UdpReceiver&) = delete;

    // ========================================================================
    // 配置接口
    // ========================================================================

    /**
     * @brief 设置本地端口
     * @param port 端口号
     */
    void setLocalPort(int port);

    /**
     * @brief 获取本地端口
     * @return 端口号
     */
    int getLocalPort() const;

    /**
     * @brief 设置接收缓冲区大小
     * @param size 缓冲区大小（字节）
     */
    void setBufferSize(int size);

    /**
     * @brief 设置帧超时时间
     * @param timeout_ms 超时时间（毫秒）
     */
    void setFrameTimeout(std::uint32_t timeout_ms);

    /**
     * @brief 设置最大并发帧数
     * @param max_frames 最大帧数
     */
    void setMaxConcurrentFrames(std::size_t max_frames);

    /**
     * @brief 启用/禁用CRC校验
     * @param enabled 是否启用
     */
    void setCrcCheckEnabled(bool enabled);

    /**
     * @brief 设置统计更新间隔
     * @param interval_ms 间隔（毫秒）
     */
    void setStatsInterval(std::uint32_t interval_ms);

    // ========================================================================
    // 控制接口
    // ========================================================================

    /**
     * @brief 启动接收器
     * @return true 成功；false 失败
     */
    bool start();

    /**
     * @brief 停止接收器
     */
    void stop();

    /**
     * @brief 检查是否正在运行
     * @return true 运行中；false 已停止
     */
    bool isRunning() const;

    /**
     * @brief 获取当前状态
     * @return 接收器状态
     */
    ReceiverState getState() const;

    /**
     * @brief 清空缓冲区
     */
    void clearBuffers();

    /**
     * @brief 重置统计信息
     */
    void resetStats();

    /**
     * @brief 获取统计信息
     * @return 统计信息结构
     */
    ReceiverStats getStats() const;

    /**
     * @brief 获取重组模块
     * @return 重组模块指针
     */
    PacketReassembly* getReassembly();

    /**
     * @brief 获取UDP Socket
     * @return Socket指针
     */
    QUdpSocket* getSocket();

signals:
    /**
     * @brief 帧接收完成信号
     * @param frame 接收到的原始视频帧
     */
    void frameReceived(const ReceivedFrame& frame);

    /**
     * @brief 数据包接收信号
     * @param size 数据大小
     */
    void packetReceived(std::size_t size);

    /**
     * @brief 状态变更信号
     * @param state 新状态
     */
    void stateChanged(ReceiverState state);

    /**
     * @brief 错误发生信号
     * @param error 错误类型
     * @param message 错误消息
     */
    void errorOccurred(ReceiverError error, const QString& message);

    /**
     * @brief 统计更新信号
     * @param stats 统计信息
     */
    void statsUpdated(const ReceiverStats& stats);

    /**
     * @brief 帧超时信号
     * @param frame_id 超时的帧ID
     */
    void frameTimeout(std::uint32_t frame_id);

private slots:
    /**
     * @brief UDP数据就绪槽函数
     */
    void onReadyRead();

    /**
     * @brief Socket错误槽函数
     */
    void onSocketError(QAbstractSocket::SocketError error);

    /**
     * @brief 帧重组完成槽函数
     * @param frame 重组完成的帧
     */
    void onFrameReassembled(const ReceivedFrame& frame);

    /**
     * @brief 帧超时槽函数
     * @param frame_id 超时的帧ID
     */
    void onFrameTimeout(std::uint32_t frame_id);

    /**
     * @brief 重组错误槽函数
     * @param error 错误类型
     * @param message 错误消息
     */
    void onReassemblyError(ReassemblyError error, const QString& message);

    /**
     * @brief 统计更新定时器槽函数
     */
    void onStatsTimer();

private:
    /**
     * @brief 初始化
     */
    void initialize();

    /**
     * @brief 处理接收到的数据包
     */
    void processPendingDatagrams();

    /**
     * @brief 更新状态
     * @param state 新状态
     */
    void updateState(ReceiverState state);

    /**
     * @brief 更新统计信息
     */
    void updateStats();

    // 成员变量
    QUdpSocket* socket_;                    ///< UDP Socket
    PacketReassembly* reassembly_;          ///< 数据包重组模块
    QTimer* stats_timer_;                   ///< 统计更新定时器

    int local_port_;                        ///< 本地端口
    int buffer_size_;                       ///< 接收缓冲区大小
    ReceiverState state_;                   ///< 当前状态

    mutable QMutex stats_mutex_;            ///< 统计信息互斥锁
    ReceiverStats stats_;                   ///< 统计信息

    // 统计计算辅助变量
    std::uint64_t last_packets_;
    std::uint64_t last_bytes_;
    std::uint64_t last_frames_;
    qint64 last_stats_time_;

    // 最近接收的来源信息
    QHostAddress last_source_address_;
    std::uint16_t last_source_port_;
};

} // namespace ground_station

#endif // GROUND_STATION_UDPRECEIVER_H