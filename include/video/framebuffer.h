/**
 * @file framebuffer.h
 * @brief 帧缓冲管理模块
 *
 * 提供线程安全的帧缓冲管理功能，支持单流和多流场景。
 * 实现了双缓冲机制和预缓冲策略，确保视频流畅显示。
 *
 * @author GroundStation Team
 * @date 2024
 */

#ifndef GROUND_STATION_FRAMEBUFFER_H
#define GROUND_STATION_FRAMEBUFFER_H

#include <QObject>
#include <QDebug>
#include <QImage>
#include <QMap>
#include <QMutex>
#include <QQueue>
#include <QSharedPointer>
#include <QWaitCondition>
#include <opencv2/opencv.hpp>

namespace ground_station {

/**
 * @struct VideoFrame
 * @brief 视频帧数据结构（解码后用于渲染）
 */
struct VideoFrame {
    quint64 frameId;        ///< 帧ID
    int streamId;           ///< 流ID
    int width;              ///< 帧宽度
    int height;             ///< 帧高度
    int channels;           ///< 通道数（1/3/4）
    qint64 timestamp;       ///< 时间戳
    qint64 captureTime;     ///< 采集时间
    double decodeTime;      ///< 解码耗时（毫秒）
    cv::Mat data;           ///< 帧数据
    bool isValid;           ///< 是否有效

    /**
     * @brief 构造函数
     */
    VideoFrame()
        : frameId(0)
        , streamId(-1)
        , width(0)
        , height(0)
        , channels(0)
        , timestamp(0)
        , captureTime(0)
        , decodeTime(0.0)
        , isValid(false)
    {}

    /**
     * @brief 转换为QImage
     * @return QImage对象
     */
    QImage toQImage() const {
        if (!isValid || data.empty()) {
            return QImage();
        }

        cv::Mat rgb;
        if (channels == 3) {
            cv::cvtColor(data, rgb, cv::COLOR_BGR2RGB);
        } else if (channels == 4) {
            cv::cvtColor(data, rgb, cv::COLOR_BGRA2RGB);
        } else if (channels == 1) {
            cv::cvtColor(data, rgb, cv::COLOR_GRAY2RGB);
        } else {
            rgb = data;
        }

        return QImage(rgb.data, rgb.cols, rgb.rows, 
                      static_cast<int>(rgb.step), QImage::Format_RGB888).copy();
    }
};

/// 视频帧智能指针类型
using VideoFramePtr = QSharedPointer<VideoFrame>;

/**
 * @class FrameBuffer
 * @brief 帧缓冲管理类
 * 
 * 实现线程安全的帧缓冲管理，支持多生产者多消费者模式。
 * 提供阻塞/非阻塞两种操作方式，支持帧丢弃策略配置。
 */
class FrameBuffer : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数（默认缓冲区大小）
     * @param parent 父对象
     */
    explicit FrameBuffer(QObject *parent = nullptr);

    /**
     * @brief 构造函数（指定缓冲区大小）
     * @param maxBufferSize 最大缓冲帧数
     * @param parent 父对象
     */
    explicit FrameBuffer(int maxBufferSize, QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~FrameBuffer();

    /**
     * @brief 设置最大缓冲区大小
     * @param size 最大帧数
     */
    void setMaxBufferSize(int size);

    /**
     * @brief 获取最大缓冲区大小
     * @return 最大帧数
     */
    int maxBufferSize() const;

    /**
     * @brief 获取当前缓冲区大小
     * @return 当前帧数
     */
    int currentBufferSize() const;

    /**
     * @brief 推入帧（阻塞方式）
     * @param frame 帧指针
     * @return 是否成功
     */
    bool pushFrame(const VideoFramePtr &frame);

    /**
     * @brief 尝试推入帧（非阻塞方式）
     * @param frame 帧指针
     * @return 是否成功
     */
    bool tryPushFrame(const VideoFramePtr &frame);

    /**
     * @brief 弹出帧（阻塞方式）
     * @param timeoutMs 超时时间（毫秒），-1表示无限等待
     * @return 帧指针，超时返回nullptr
     */
    VideoFramePtr popFrame(int timeoutMs = -1);

    /**
     * @brief 尝试弹出帧（非阻塞方式）
     * @return 帧指针，无帧返回nullptr
     */
    VideoFramePtr tryPopFrame();

    /**
     * @brief 查看最新帧（不弹出）
     * @return 最新帧指针
     */
    VideoFramePtr peekLatestFrame() const;

    /**
     * @brief 清空缓冲区
     */
    void clear();

    /**
     * @brief 检查缓冲区是否为空
     * @return 是否为空
     */
    bool isEmpty() const;

    /**
     * @brief 检查缓冲区是否已满
     * @return 是否已满
     */
    bool isFull() const;

    /**
     * @brief 获取丢弃的帧数
     * @return 丢弃帧数
     */
    quint64 droppedFrames() const;

    /**
     * @brief 重置丢弃帧数统计
     */
    void resetDroppedFrames();

    /**
     * @brief 设置帧丢弃策略
     * @param dropOld true=丢弃旧帧，false=丢弃新帧
     */
    void setDropPolicy(bool dropOld);

signals:
    /**
     * @brief 帧就绪信号
     * @param streamId 流ID
     */
    void frameReady(int streamId);

    /**
     * @brief 缓冲区状态改变信号
     * @param currentSize 当前大小
     * @param maxSize 最大大小
     */
    void bufferStatusChanged(int currentSize, int maxSize);

    /**
     * @brief 缓冲区溢出信号
     * @param droppedCount 丢弃帧数
     */
    void bufferOverflow(quint64 droppedCount);

private:
    mutable QMutex m_mutex;                ///< 互斥锁
    QWaitCondition m_notEmpty;             ///< 非空条件变量
    QWaitCondition m_notFull;              ///< 非满条件变量
    QQueue<VideoFramePtr> m_buffer;        ///< 帧队列
    VideoFramePtr m_latestFrame;           ///< 最新帧缓存
    int m_maxBufferSize;                   ///< 最大缓冲大小
    quint64 m_droppedFrames;               ///< 丢弃帧数统计
    bool m_dropOld;                        ///< 丢弃策略：true=丢弃旧帧
};

/**
 * @class MultiStreamFrameBuffer
 * @brief 多路视频帧缓冲管理器
 * 
 * 管理多个视频流的帧缓冲，支持按流ID访问和动态添加/移除流。
 */
class MultiStreamFrameBuffer : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数（默认缓冲区大小）
     * @param parent 父对象
     */
    explicit MultiStreamFrameBuffer(QObject *parent = nullptr);

    /**
     * @brief 构造函数（指定默认缓冲区大小）
     * @param defaultBufferSize 默认缓冲帧数
     * @param parent 父对象
     */
    explicit MultiStreamFrameBuffer(int defaultBufferSize, QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~MultiStreamFrameBuffer();

    /**
     * @brief 添加流
     * @param streamId 流ID
     * @param bufferSize 缓冲帧数（默认30）
     */
    void addStream(int streamId, int bufferSize = 30);

    /**
     * @brief 移除流
     * @param streamId 流ID
     */
    void removeStream(int streamId);

    /**
     * @brief 检查流是否存在
     * @param streamId 流ID
     * @return 是否存在
     */
    bool hasStream(int streamId) const;

    /**
     * @brief 获取所有流ID
     * @return 流ID列表
     */
    QList<int> streamIds() const;

    /**
     * @brief 推入帧到指定流
     * @param streamId 流ID
     * @param frame 帧指针
     * @return 是否成功
     */
    bool pushFrame(int streamId, const VideoFramePtr &frame);

    /**
     * @brief 从指定流弹出帧
     * @param streamId 流ID
     * @param timeoutMs 超时时间
     * @return 帧指针
     */
    VideoFramePtr popFrame(int streamId, int timeoutMs = -1);

    /**
     * @brief 查看指定流的最新帧
     * @param streamId 流ID
     * @return 帧指针
     */
    VideoFramePtr peekLatestFrame(int streamId) const;

    /**
     * @brief 清空指定流的缓冲区
     * @param streamId 流ID
     */
    void clearStream(int streamId);

    /**
     * @brief 清空所有流的缓冲区
     */
    void clearAll();

    /**
     * @brief 设置指定流的缓冲区大小
     * @param streamId 流ID
     * @param size 缓冲帧数
     */
    void setStreamBufferSize(int streamId, int size);

    /**
     * @brief 设置默认缓冲区大小
     * @param size 缓冲帧数
     */
    void setDefaultBufferSize(int size);

signals:
    /**
     * @brief 帧就绪信号
     * @param streamId 流ID
     */
    void frameReady(int streamId);

    /**
     * @brief 流添加信号
     * @param streamId 流ID
     */
    void streamAdded(int streamId);

    /**
     * @brief 流移除信号
     * @param streamId 流ID
     */
    void streamRemoved(int streamId);

private:
    /**
     * @brief 连接缓冲区信号
     * @param streamId 流ID
     * @param buffer 缓冲区指针
     */
    void connectBufferSignals(int streamId, const QSharedPointer<FrameBuffer> &buffer);

    mutable QMutex m_mutex;                                   ///< 互斥锁
    QMap<int, QSharedPointer<FrameBuffer>> m_streamBuffers;  ///< 流缓冲区映射
    int m_defaultBufferSize;                                  ///< 默认缓冲区大小
};

} // namespace ground_station

#endif // GROUND_STATION_FRAMEBUFFER_H
