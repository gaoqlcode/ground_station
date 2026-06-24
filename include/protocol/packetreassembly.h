/**
 * @file packetreassembly.h
 * @brief 数据包重组模块头文件
 *
 * 提供UDP分片数据包重组功能，支持：
 * - 基于帧ID的数据包重组
 * - CRC32校验
 * - 超时处理机制
 * - 多帧并发处理
 *
 * 协议格式：
 * UDP_HEADER(24字节) + 负载数据
 *
 * @author GroundStation Team
 * @date 2024
 */

#ifndef GROUND_STATION_PACKETREASSEMBLY_H
#define GROUND_STATION_PACKETREASSEMBLY_H

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QVector>
#include <QTimer>
#include <QMutex>
#include <QHostAddress>
#include <cstdint>

namespace ground_station {

// ============================================================================
// 常量定义
// ============================================================================

/// UDP协议头大小（字节）
constexpr std::size_t UDP_HEADER_SIZE = 24;

/// 最大分片大小（字节），考虑MTU限制
constexpr std::size_t MAX_FRAGMENT_SIZE = 1400;

/// 最大负载大小（字节）= 最大分片大小 - 协议头大小
constexpr std::size_t MAX_PAYLOAD_SIZE = MAX_FRAGMENT_SIZE - UDP_HEADER_SIZE;

/// UDP协议魔数，用于标识数据包格式
constexpr std::uint32_t UDP_MAGIC_NUMBER = 0x47514C55;  // "GQLU"

/// 协议版本号
constexpr std::uint8_t UDP_PROTOCOL_VERSION = 1;

/// 默认帧超时时间（毫秒）
constexpr std::uint32_t DEFAULT_FRAME_TIMEOUT_MS = 5000;

/// 最大并发处理帧数
constexpr std::size_t MAX_CONCURRENT_FRAMES = 100;

// ============================================================================
// 枚举类型
// ============================================================================

/**
 * @enum UdpPacketType
 * @brief UDP数据包类型枚举
 */
enum class UdpPacketType : std::uint8_t {
    FrameData = 0x01,       ///< 帧数据
    Ack = 0x02,             ///< 确认包
    Nack = 0x03,            ///< 否定确认包
    Heartbeat = 0x04,       ///< 心跳包
    FlowControl = 0x05      ///< 流控制包
};

/**
 * @enum ReassemblyError
 * @brief 重组错误类型枚举
 */
enum class ReassemblyError {
    None = 0,               ///< 无错误
    InvalidHeader,          ///< 无效协议头
    InvalidMagic,           ///< 无效魔数
    InvalidVersion,         ///< 无效版本号
    InvalidCrc,             ///< CRC校验失败
    InvalidFragment,        ///< 无效分片
    Timeout,                ///< 超时
    BufferFull              ///< 缓冲区已满
};

// ============================================================================
// 结构体定义
// ============================================================================

/**
 * @struct UdpHeader
 * @brief UDP协议头结构
 *
 * 采用紧凑打包（1字节对齐），总大小24字节
 */
#pragma pack(push, 1)
struct UdpHeader {
    std::uint32_t magic;            ///< 魔数，固定为0x47514C55
    std::uint8_t version;           ///< 协议版本号
    std::uint8_t type;              ///< 数据包类型（UdpPacketType）
    std::uint8_t flags;             ///< 标志位
    std::uint8_t reserved;          ///< 保留位
    std::uint32_t frame_id;         ///< 帧ID，用于分片重组
    std::uint16_t fragment_index;   ///< 当前分片索引（从0开始）
    std::uint16_t fragment_count;   ///< 总分片数
    std::uint16_t payload_size;     ///< 负载大小
    std::uint16_t payload_offset;   ///< 负载在帧中的偏移
    std::uint32_t crc32;            ///< CRC32校验值
};
#pragma pack(pop)

/**
 * @struct ReceivedFrame
 * @brief 重组完成的帧结构
 */
struct ReceivedFrame {
    std::uint32_t frame_id;         ///< 帧ID
    std::uint64_t timestamp;        ///< 时间戳
    QByteArray data;                ///< 完整的帧数据
    std::uint16_t fragment_count;   ///< 分片数量
    std::uint64_t recv_duration;    ///< 接收耗时（毫秒）
    QHostAddress source_address;    ///< 源地址
    quint16 source_port;            ///< 源端口
};

/**
 * @class ReassemblyFrameBuffer
 * @brief 单帧重组缓冲区
 *
 * 用于存储正在重组中的帧数据，管理分片接收状态
 */
class ReassemblyFrameBuffer {
public:
    /**
     * @brief 构造函数
     * @param fid 帧ID
     * @param total 总分片数
     */
    ReassemblyFrameBuffer(std::uint32_t fid, std::uint16_t total);
    
    /**
     * @brief 添加分片数据
     * @param frag_index 分片索引
     * @param data 分片数据
     * @return true=添加成功，false=索引无效
     */
    bool addFragment(std::uint16_t frag_index, const QByteArray& data);
    
    /**
     * @brief 检查帧是否完整
     * @return true=所有分片已接收，false=还缺少分片
     */
    bool isComplete() const;
    
    /**
     * @brief 重组帧数据
     * @param out_data 输出的完整帧数据
     * @return true=重组成功，false=帧不完整
     */
    bool reassemble(QByteArray& out_data) const;
    
    std::uint32_t frame_id;             ///< 帧ID
    std::uint16_t total_fragments;      ///< 总分片数
    std::uint16_t received_count;       ///< 已接收分片数
    std::uint64_t first_recv_time;      ///< 首个分片接收时间
    std::uint64_t last_recv_time;       ///< 最后分片接收时间
    std::uint64_t timestamp;            ///< 帧时间戳
    QVector<bool> fragment_bitmap;      ///< 分片接收位图
    QVector<QByteArray> fragments;      ///< 分片数据存储
};

/**
 * @class PacketReassembly
 * @brief UDP数据包重组类
 *
 * 负责接收UDP分片数据包，进行CRC校验、超时管理，并重组为完整帧。
 * 支持多帧并发处理，具有自动清理超时帧的机制。
 */
class PacketReassembly : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit PacketReassembly(QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~PacketReassembly();

    /**
     * @brief 设置帧超时时间
     * @param timeout_ms 超时时间（毫秒）
     */
    void setFrameTimeout(std::uint32_t timeout_ms);
    
    /**
     * @brief 设置最大并发处理帧数
     * @param max_frames 最大并发帧数
     */
    void setMaxConcurrentFrames(std::size_t max_frames);
    
    /**
     * @brief 设置是否启用CRC校验
     * @param enabled true=启用，false=禁用
     */
    void setCrcCheckEnabled(bool enabled);
    
    /**
     * @brief 启动重组器
     */
    void start();
    
    /**
     * @brief 停止重组器
     */
    void stop();

    /**
     * @brief 处理接收到的UDP数据包
     * @param packet 原始数据包（包含协议头）
     * @return 重组错误类型，None表示成功
     */
    ReassemblyError processPacket(const QByteArray &packet);
    
    /**
     * @brief 清空所有帧缓冲区
     */
    void clearBuffers();
    
    /**
     * @brief 获取统计信息
     * @param total_packets 输出：处理的总包数
     * @param total_frames 输出：处理的总帧数
     * @param complete_frames 输出：成功重组的帧数
     * @param timeout_frames 输出：超时丢弃的帧数
     * @param crc_errors 输出：CRC错误数
     */
    void getStatistics(std::uint64_t &total_packets, std::uint64_t &total_frames,
                      std::uint64_t &complete_frames, std::uint64_t &timeout_frames,
                      std::uint64_t &crc_errors);

signals:
    /**
     * @brief 帧重组完成信号
     * @param frame 重组完成的帧数据
     */
    void frameReassembled(const ReceivedFrame &frame);
    
    /**
     * @brief 帧超时信号
     * @param frame_id 超时的帧ID
     */
    void frameTimeout(std::uint32_t frame_id);
    
    /**
     * @brief 错误发生信号
     * @param error 错误类型
     * @param message 错误描述信息
     */
    void errorOccurred(ReassemblyError error, const QString &message);

private slots:
    /**
     * @brief 帧超时定时器槽函数
     */
    void onFrameTimeout();

private:
    /**
     * @brief 验证UDP协议头
     * @param header 协议头指针
     * @return true=验证通过，false=验证失败
     */
    bool validateHeader(const UdpHeader *header);
    
    /**
     * @brief 验证CRC校验值
     * @param packet 完整数据包
     * @param header 协议头指针
     * @return true=校验通过，false=校验失败
     */
    bool validateCrc(const QByteArray &packet, const UdpHeader *header);
    
    /**
     * @brief 处理分片数据
     * @param frame_id 帧ID
     * @param fragment_index 分片索引
     * @param fragment_count 总分片数
     * @param payload 分片负载数据
     */
    void processFragment(std::uint32_t frame_id, std::uint16_t fragment_index,
                        std::uint16_t fragment_count, const QByteArray &payload);
    
    /**
     * @brief 检查帧是否完整并触发信号
     * @param frame_id 帧ID
     */
    void checkFrameComplete(std::uint32_t frame_id);
    
    /**
     * @brief 清理超时的帧缓冲区
     */
    void cleanupOldFrames();
    
    /**
     * @brief 清理指定帧的缓冲区
     * @param frame_id 帧ID
     */
    void cleanupFrame(std::uint32_t frame_id);
    
    /**
     * @brief 处理帧数据类型的数据包
     * @param header 协议头
     * @param payload 负载数据
     * @return 错误类型
     */
    ReassemblyError processFrameData(const UdpHeader *header, const QByteArray &payload);

    QHash<std::uint32_t, ReassemblyFrameBuffer*> frame_buffers_;
    QHash<std::uint32_t, QTimer*> frame_timers_;
    QTimer* timeout_timer_;
    std::uint32_t frame_timeout_ms_;
    std::size_t max_concurrent_frames_;
    bool crc_check_enabled_;
    bool running_;
    std::uint64_t total_packets_;
    std::uint64_t total_frames_;
    std::uint64_t complete_frames_;
    std::uint64_t timeout_frames_;
    std::uint64_t crc_errors_;
    QMutex mutex_;
};

} // namespace ground_station

#endif // GROUND_STATION_PACKETREASSEMBLY_H
