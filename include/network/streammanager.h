/**
 * @file streammanager.h
 * @brief 流管理器
 *
 * 管理最多2路视频流的注册和切换
 */

#ifndef GROUND_STATION_STREAMMANAGER_H
#define GROUND_STATION_STREAMMANAGER_H

#include <QObject>
#include <QMap>
#include <QMutex>
#include <QTimer>
#include <QByteArray>
#include <memory>

namespace ground_station {

/**
 * @enum StreamStatus
 * @brief 流状态枚举
 */
enum class StreamStatus {
    Inactive,       // 未激活
    Registering,    // 注册中
    Active,         // 激活
    Error           // 错误
};

/**
 * @struct StreamInfo
 * @brief 流信息结构
 */
struct StreamInfo {
    int streamId;           // 流ID (0或1)
    QString streamName;     // 流名称
    StreamStatus status;    // 流状态
    qint64 lastUpdateTime;  // 最后更新时间戳
    int frameCount;         // 已接收帧数
    qint64 totalBytes;      // 总字节数

    StreamInfo()
        : streamId(-1)
        , status(StreamStatus::Inactive)
        , lastUpdateTime(0)
        , frameCount(0)
        , totalBytes(0) {
    }
};

/**
 * @class StreamManager
 * @brief 流管理器类
 *
 * 管理视频流的注册、切换和状态监控
 * 支持最多2路视频流
 */
class StreamManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param maxStreams 最大流数量
     * @param parent 父对象
     */
    explicit StreamManager(int maxStreams = 2, QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~StreamManager() override;

    /**
     * @brief 注册流
     * @param streamName 流名称
     * @return 分配的流ID，-1表示注册失败
     */
    int registerStream(const QString& streamName);

    /**
     * @brief 注销流
     * @param streamId 流ID
     * @return 是否成功
     */
    bool unregisterStream(int streamId);

    /**
     * @brief 激活流
     * @param streamId 流ID
     * @return 是否成功
     */
    bool activateStream(int streamId);

    /**
     * @brief 停用流
     * @param streamId 流ID
     * @return 是否成功
     */
    bool deactivateStream(int streamId);

    /**
     * @brief 切换到指定流
     * @param streamId 流ID
     * @return 是否成功
     */
    bool switchToStream(int streamId);

    /**
     * @brief 获取当前激活的流ID
     * @return 当前激活的流ID，-1表示无激活流
     */
    int getCurrentStreamId() const;

    /**
     * @brief 获取流信息
     * @param streamId 流ID
     * @return 流信息
     */
    StreamInfo getStreamInfo(int streamId) const;

    /**
     * @brief 获取所有流信息
     * @return 流信息列表
     */
    QList<StreamInfo> getAllStreamInfo() const;

    /**
     * @brief 更新流数据
     * @param streamId 流ID
     * @param dataSize 数据大小
     */
    void updateStreamData(int streamId, int dataSize);

    /**
     * @brief 检查流是否有效
     * @param streamId 流ID
     * @return 是否有效
     */
    bool isStreamValid(int streamId) const;

    /**
     * @brief 获取流数量
     * @return 当前注册的流数量
     */
    int getStreamCount() const;

    /**
     * @brief 清除所有流
     */
    void clearAllStreams();

    /**
     * @brief 设置流超时时间
     * @param timeout 超时时间（毫秒）
     */
    void setStreamTimeout(int timeout);

signals:
    /**
     * @brief 流注册信号
     * @param streamId 流ID
     * @param streamName 流名称
     */
    void streamRegistered(int streamId, const QString& streamName);

    /**
     * @brief 流注销信号
     * @param streamId 流ID
     */
    void streamUnregistered(int streamId);

    /**
     * @brief 流激活信号
     * @param streamId 流ID
     */
    void streamActivated(int streamId);

    /**
     * @brief 流停用信号
     * @param streamId 流ID
     */
    void streamDeactivated(int streamId);

    /**
     * @brief 流切换信号
     * @param oldStreamId 旧流ID
     * @param newStreamId 新流ID
     */
    void streamSwitched(int oldStreamId, int newStreamId);

    /**
     * @brief 流超时信号
     * @param streamId 流ID
     */
    void streamTimeout(int streamId);

    /**
     * @brief 流错误信号
     * @param streamId 流ID
     * @param errorMessage 错误消息
     */
    void streamError(int streamId, const QString& errorMessage);

private slots:
    /**
     * @brief 检查流超时
     */
    void checkStreamTimeout();

private:
    /**
     * @brief 检查流ID是否有效
     * @param streamId 流ID
     * @return 是否有效
     */
    bool isValidStreamId(int streamId) const;

    /**
     * @brief 分配流ID
     * @return 分配的流ID，-1表示无可用ID
     */
    int allocateStreamId();

    int m_maxStreams;                           // 最大流数量
    int m_currentStreamId;                      // 当前激活的流ID
    int m_streamTimeout;                        // 流超时时间（毫秒）
    QMap<int, StreamInfo> m_streams;            // 流映射表
    mutable QMutex m_mutex;                     // 互斥锁
    QTimer* m_timeoutTimer;                    // 超时检查定时器
};

}  // namespace ground_station

#endif  // GROUND_STATION_STREAMMANAGER_H