/**
 * @file videostats.h
 * @brief 视频统计模块头文件
 *
 * 提供视频流统计功能，包括帧率计算、延迟测量、丢帧统计等。
 * 支持多路视频流的独立统计，提供实时统计更新和告警功能。
 *
 * @author GroundStation Team
 * @date 2024
 */

#ifndef GROUND_STATION_VIDEOSTATS_H
#define GROUND_STATION_VIDEOSTATS_H

#include <QObject>
#include <QMutex>
#include <QDateTime>
#include <QMap>
#include <QQueue>
#include <QPair>

namespace ground_station {

/**
 * @struct VideoStatsInfo
 * @brief 视频统计信息结构
 * 
 * 包含视频流的关键统计数据，用于实时监控和显示
 */
struct VideoStatsInfo {
    double fps;              ///< 实际帧率（帧/秒）
    double decodeTime;       ///< 解码耗时（毫秒）
    double renderTime;       ///< 渲染耗时（毫秒）
    double endToEndLatency;  ///< 端到端延迟（毫秒）
    quint64 totalFrames;     ///< 总帧数
    quint64 droppedFrames;   ///< 丢帧数
    double dropRate;         ///< 丢帧率（0.0-1.0）
    quint64 currentBitrate;  ///< 当前码率（bps）
};

/**
 * @struct FrameTimestamp
 * @brief 帧时间戳记录结构
 * 
 * 记录帧在各个处理阶段的时间戳，用于计算延迟
 */
struct FrameTimestamp {
    quint64 frameId;        ///< 帧ID
    qint64 captureTime;     ///< 采集时间戳（纳秒）
    qint64 encodeTime;      ///< 编码时间戳（纳秒）
    qint64 sendTime;        ///< 发送时间戳（纳秒）
    qint64 receiveTime;     ///< 接收时间戳（纳秒）
    qint64 decodeTime;      ///< 解码时间戳（纳秒）
    qint64 renderTime;      ///< 渲染时间戳（纳秒）
};

/**
 * @class VideoStats
 * @brief 视频统计器类
 * 
 * 提供帧率统计、延迟测量、丢帧统计等功能，支持多路视频流的独立统计。
 * 使用滑动窗口算法计算平均值，确保统计数据的实时性和稳定性。
 */
class VideoStats : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit VideoStats(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~VideoStats();

    /**
     * @brief 记录帧接收事件
     * @param streamId 视频流ID
     * @param frameId 帧ID
     * @param timestamp 帧时间戳信息
     */
    void recordFrameReceived(int streamId, quint64 frameId, const FrameTimestamp &timestamp);

    /**
     * @brief 记录解码完成事件
     * @param streamId 视频流ID
     * @param frameId 帧ID
     * @param decodeTimeMs 解码耗时（毫秒）
     */
    void recordFrameDecoded(int streamId, quint64 frameId, double decodeTimeMs);

    /**
     * @brief 记录渲染完成事件
     * @param streamId 视频流ID
     * @param frameId 帧ID
     * @param renderTimeMs 渲染耗时（毫秒）
     */
    void recordFrameRendered(int streamId, quint64 frameId, double renderTimeMs);

    /**
     * @brief 记录丢帧事件
     * @param streamId 视频流ID
     * @param count 丢帧数量（默认1）
     */
    void recordDroppedFrames(int streamId, quint64 count = 1);

    /**
     * @brief 获取指定视频流的统计信息
     * @param streamId 视频流ID
     * @return 统计信息结构
     */
    VideoStatsInfo getStats(int streamId) const;

    /**
     * @brief 获取所有视频流的统计信息
     * @return 映射表：流ID -> 统计信息
     */
    QMap<int, VideoStatsInfo> getAllStats() const;

    /**
     * @brief 重置指定视频流的统计信息
     * @param streamId 视频流ID
     */
    void resetStats(int streamId);

    /**
     * @brief 重置所有视频流的统计信息
     */
    void resetAllStats();

    /**
     * @brief 设置帧率计算窗口大小
     * @param windowSize 窗口大小（帧数）
     */
    void setFpsWindowSize(int windowSize);

    /**
     * @brief 设置延迟计算窗口大小
     * @param windowSize 窗口大小（帧数）
     */
    void setLatencyWindowSize(int windowSize);

signals:
    /**
     * @brief 统计信息更新信号
     * @param streamId 视频流ID
     * @param stats 更新后的统计信息
     */
    void statsUpdated(int streamId, const VideoStatsInfo &stats);

    /**
     * @brief 高延迟告警信号
     * @param streamId 视频流ID
     * @param latencyMs 延迟（毫秒）
     */
    void highLatencyWarning(int streamId, double latencyMs);

    /**
     * @brief 高丢帧率告警信号
     * @param streamId 视频流ID
     * @param dropRate 丢帧率
     */
    void highDropRateWarning(int streamId, double dropRate);

private:
    /**
     * @struct StreamStats
     * @brief 单流统计数据结构
     * 
     * 包含单个视频流的所有统计数据和历史记录
     */
    struct StreamStats {
        QQueue<qint64> frameTimestamps;      ///< 帧时间戳队列（用于计算FPS）
        int fpsWindowSize;                    ///< FPS计算窗口大小
        
        QQueue<double> latencyHistory;        ///< 延迟历史队列
        int latencyWindowSize;                ///< 延迟计算窗口大小
        
        QQueue<double> decodeTimeHistory;     ///< 解码时间历史队列
        QQueue<double> renderTimeHistory;     ///< 渲染时间历史队列
        
        quint64 totalFrames;                  ///< 总帧数
        quint64 droppedFrames;                ///< 丢帧数
        quint64 lastFrameId;                  ///< 最后帧ID
        
        QQueue<QPair<qint64, quint64>> bitrateHistory; ///< 码率历史队列
        quint64 currentBitrate;               ///< 当前码率
        
        QMap<quint64, FrameTimestamp> pendingFrames; ///< 待处理帧的时间戳映射
    };

    /**
     * @brief 初始化流统计数据
     * @param streamId 流ID
     */
    void initializeStreamStats(int streamId);

    /**
     * @brief 计算帧率
     * @param stats 流统计数据
     * @return 帧率值
     */
    double calculateFps(StreamStats &stats) const;

    /**
     * @brief 计算平均延迟
     * @param stats 流统计数据
     * @return 平均延迟（毫秒）
     */
    double calculateAverageLatency(StreamStats &stats) const;

    /**
     * @brief 计算平均解码时间
     * @param stats 流统计数据
     * @return 平均解码时间（毫秒）
     */
    double calculateAverageDecodeTime(StreamStats &stats) const;

    /**
     * @brief 计算平均渲染时间
     * @param stats 流统计数据
     * @return 平均渲染时间（毫秒）
     */
    double calculateAverageRenderTime(StreamStats &stats) const;

    mutable QMutex m_mutex;                       ///< 互斥锁，保证线程安全
    QMap<int, StreamStats> m_streamStats;         ///< 流统计数据映射
    int m_defaultFpsWindowSize;                   ///< 默认FPS窗口大小
    int m_defaultLatencyWindowSize;               ///< 默认延迟窗口大小
    double m_highLatencyThreshold;                ///< 高延迟阈值（毫秒）
    double m_highDropRateThreshold;               ///< 高丢帧率阈值
};

} // namespace ground_station

#endif // GROUND_STATION_VIDEOSTATS_H