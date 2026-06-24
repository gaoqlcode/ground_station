/**
 * @file videodecoder.h
 * @brief 视频解码器模块
 *
 * 提供多线程视频解码功能，支持批量解码和帧队列管理。
 * 采用生产者-消费者模式，实现高效的视频帧解码。
 *
 * @author GroundStation Team
 * @date 2024
 */

#ifndef GROUND_STATION_VIDEODECODER_H
#define GROUND_STATION_VIDEODECODER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QMap>
#include <QElapsedTimer>
#include "framebuffer.h"
#include "videostats.h"

namespace ground_station {

/**
 * @struct EncodedFrame
 * @brief 编码帧数据结构
 */
struct EncodedFrame {
    quint64 frameId;      ///< 帧ID
    int streamId;         ///< 流ID
    QByteArray data;      ///< 编码数据
    qint64 timestamp;     ///< 时间戳
    qint64 captureTime;   ///< 采集时间
    int width;            ///< 宽度
    int height;           ///< 高度
};

/**
 * @class DecodeWorker
 * @brief 解码工作者类
 *
 * 在独立线程中执行视频帧解码任务。
 */
class DecodeWorker : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit DecodeWorker(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~DecodeWorker();

    /**
     * @brief 设置帧缓冲区
     * @param buffer 帧缓冲区指针
     */
    void setFrameBuffer(const QSharedPointer<FrameBuffer> &buffer);

    /**
     * @brief 设置视频统计对象
     * @param stats 视频统计对象
     */
    void setVideoStats(VideoStats *stats);

public slots:
    /**
     * @brief 解码帧
     * @param frame 编码帧
     */
    void decodeFrame(const EncodedFrame &frame);

    /**
     * @brief 停止解码
     */
    void stop();

signals:
    /**
     * @brief 帧解码完成信号
     * @param frameId 帧ID
     * @param streamId 流ID
     * @param decodeTimeMs 解码耗时（毫秒）
     */
    void frameDecoded(quint64 frameId, int streamId, double decodeTimeMs);

    /**
     * @brief 解码错误信号
     * @param frameId 帧ID
     * @param streamId 流ID
     * @param error 错误消息
     */
    void decodeError(quint64 frameId, int streamId, const QString &error);

private:
    QSharedPointer<FrameBuffer> m_frameBuffer;  ///< 帧缓冲区
    VideoStats *m_videoStats;                   ///< 视频统计对象
    bool m_running;                             ///< 运行标志
    QElapsedTimer m_timer;                      ///< 计时器
};

/**
 * @class VideoDecoder
 * @brief 视频解码器类
 *
 * 管理多个解码线程，实现多线程并行解码。
 * 支持帧队列管理和批量解码。
 */
class VideoDecoder : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit VideoDecoder(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~VideoDecoder();

    /**
     * @brief 初始化解码器
     * @param threadCount 线程数（默认1）
     * @return 是否成功
     */
    bool initialize(int threadCount = 1);

    /**
     * @brief 关闭解码器
     */
    void shutdown();

    /**
     * @brief 设置指定流的帧缓冲区
     * @param streamId 流ID
     * @param buffer 帧缓冲区指针
     */
    void setFrameBuffer(int streamId, const QSharedPointer<FrameBuffer> &buffer);

    /**
     * @brief 设置视频统计对象
     * @param stats 视频统计对象
     */
    void setVideoStats(VideoStats *stats);

    /**
     * @brief 解码单帧
     * @param frame 编码帧
     * @return 是否成功
     */
    bool decode(const EncodedFrame &frame);

    /**
     * @brief 批量解码
     * @param frames 编码帧列表
     * @return 成功解码的帧数
     */
    int decodeBatch(const QList<EncodedFrame> &frames);

    /**
     * @brief 获取待解码队列大小
     * @return 队列大小
     */
    int pendingQueueSize() const;

    /**
     * @brief 清空待解码队列
     */
    void clearPendingQueue();

    /**
     * @brief 设置最大待解码队列大小
     * @param size 队列大小
     */
    void setMaxPendingSize(int size);

    /**
     * @brief 检查是否已初始化
     * @return 是否已初始化
     */
    bool isInitialized() const;

    /**
     * @brief 获取线程数
     * @return 线程数
     */
    int threadCount() const;

signals:
    /**
     * @brief 帧解码完成信号
     * @param frameId 帧ID
     * @param streamId 流ID
     * @param decodeTimeMs 解码耗时（毫秒）
     */
    void frameDecoded(quint64 frameId, int streamId, double decodeTimeMs);

    /**
     * @brief 解码错误信号
     * @param frameId 帧ID
     * @param streamId 流ID
     * @param error 错误消息
     */
    void decodeError(quint64 frameId, int streamId, const QString &error);

private:
    /**
     * @struct DecodeThread
     * @brief 解码线程结构
     */
    struct DecodeThread {
        QThread *thread;          ///< 线程指针
        DecodeWorker *worker;     ///< 工作者对象
    };

    /**
     * @brief 分发帧到解码线程
     * @param frame 编码帧
     */
    void dispatchFrame(const EncodedFrame &frame);

    mutable QMutex m_mutex;                             ///< 互斥锁
    QList<DecodeThread> m_decodeThreads;                ///< 解码线程列表
    QMap<int, QSharedPointer<FrameBuffer>> m_frameBuffers; ///< 帧缓冲区映射
    VideoStats *m_videoStats;                           ///< 视频统计对象
    
    QQueue<EncodedFrame> m_pendingQueue;                ///< 待解码队列
    int m_maxPendingSize;                               ///< 最大队列大小
    int m_currentThreadIndex;                           ///< 当前线程索引
    bool m_initialized;                                 ///< 初始化标志
};

} // namespace ground_station

#endif // GROUND_STATION_VIDEODECODER_H