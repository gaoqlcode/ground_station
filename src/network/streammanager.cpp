/**
 * @file streammanager.cpp
 * @brief 流管理器实现
 *
 * 本文件实现了StreamManager类，用于管理多个视频流。
 * 主要功能包括：
 * - 视频流的注册、注销和激活
 * - 流切换管理
 * - 流超时检测
 * - 流状态和统计信息管理
 *
 * @author GroundStation Team
 * @version 1.0
 * @date 2024
 */

#include "streammanager.h"
#include "logger.h"
#include <QDateTime>

namespace ground_station {

/**
 * @brief 构造函数
 *
 * 初始化流管理器，设置最大流数量和超时检查定时器
 *
 * @param maxStreams 最大流数量
 * @param parent 父对象指针
 */
StreamManager::StreamManager(int maxStreams, QObject* parent)
    : QObject(parent)
    , m_maxStreams(maxStreams)
    , m_currentStreamId(-1)
    , m_streamTimeout(10000)  // 默认10秒
    , m_timeoutTimer(new QTimer(this)) {

    // 设置超时检查定时器
    connect(m_timeoutTimer, &QTimer::timeout, this, &StreamManager::checkStreamTimeout);
    m_timeoutTimer->start(1000);  // 每秒检查一次

    LOG_INFO("StreamManager", QString("StreamManager 已初始化. 最大流数量: %1").arg(m_maxStreams));
}

/**
 * @brief 析构函数
 *
 * 停止定时器并清除所有流
 */
StreamManager::~StreamManager() {
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
    }
    clearAllStreams();
    LOG_INFO("StreamManager", "StreamManager 已销毁");
}

int StreamManager::registerStream(const QString& streamName) {
    QMutexLocker locker(&m_mutex);

    // 检查是否已达到最大流数量
    if (m_streams.size() >= m_maxStreams) {
        LOG_WARN("StreamManager", QString("无法注册流: 已达到最大流数量 (%1)").arg(m_maxStreams));
        return -1;
    }

    // 分配流ID
    int streamId = allocateStreamId();
    if (streamId < 0) {
        LOG_ERROR("StreamManager", "流ID分配失败");
        return -1;
    }

    // 创建流信息
    StreamInfo info;
    info.streamId = streamId;
    info.streamName = streamName;
    info.status = StreamStatus::Registering;
    info.lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
    info.frameCount = 0;
    info.totalBytes = 0;

    m_streams[streamId] = info;

    LOG_INFO("StreamManager", QString("流已注册: ID=%1, 名称=%2")
                                .arg(streamId)
                                .arg(streamName));

    emit streamRegistered(streamId, streamName);

    return streamId;
}

/**
 * @brief 注销流
 *
 * 注销指定的视频流
 *
 * @param streamId 流ID
 * @return 成功返回true，失败返回false
 */
bool StreamManager::unregisterStream(int streamId) {
    QMutexLocker locker(&m_mutex);

    if (!isValidStreamId(streamId) || !m_streams.contains(streamId)) {
        LOG_WARN("StreamManager", QString("无法注销流: 无效的ID %1").arg(streamId));
        return false;
    }

    // 如果是当前激活的流，需要切换
    if (m_currentStreamId == streamId) {
        m_currentStreamId = -1;
    }

    m_streams.remove(streamId);

    LOG_INFO("StreamManager", QString("流已注销: ID=%1").arg(streamId));
    emit streamUnregistered(streamId);

    return true;
}

bool StreamManager::activateStream(int streamId) {
    QMutexLocker locker(&m_mutex);

    if (!isValidStreamId(streamId) || !m_streams.contains(streamId)) {
        LOG_WARN("StreamManager", QString("无法激活流: 无效的ID %1").arg(streamId));
        return false;
    }

    StreamInfo& info = m_streams[streamId];
    info.status = StreamStatus::Active;
    info.lastUpdateTime = QDateTime::currentMSecsSinceEpoch();

    LOG_INFO("StreamManager", QString("流已激活: ID=%1").arg(streamId));
    emit streamActivated(streamId);

    return true;
}

bool StreamManager::deactivateStream(int streamId) {
    QMutexLocker locker(&m_mutex);

    if (!isValidStreamId(streamId) || !m_streams.contains(streamId)) {
        LOG_WARN("StreamManager", QString("无法停用流: 无效的ID %1").arg(streamId));
        return false;
    }

    StreamInfo& info = m_streams[streamId];
    info.status = StreamStatus::Inactive;

    // 如果是当前激活的流，需要切换
    if (m_currentStreamId == streamId) {
        m_currentStreamId = -1;
    }

    LOG_INFO("StreamManager", QString("流已停用: ID=%1").arg(streamId));
    emit streamDeactivated(streamId);

    return true;
}

bool StreamManager::switchToStream(int streamId) {
    QMutexLocker locker(&m_mutex);

    if (!isValidStreamId(streamId) || !m_streams.contains(streamId)) {
        LOG_WARN("StreamManager", QString("无法切换到流: 无效的ID %1").arg(streamId));
        return false;
    }

    if (m_streams[streamId].status != StreamStatus::Active) {
        LOG_WARN("StreamManager", QString("无法切换到未激活的流: ID=%1").arg(streamId));
        return false;
    }

    int oldStreamId = m_currentStreamId;
    m_currentStreamId = streamId;

    LOG_INFO("StreamManager", QString("流已切换: 从 %1 到 %2")
                                .arg(oldStreamId)
                                .arg(streamId));

    emit streamSwitched(oldStreamId, streamId);

    return true;
}

int StreamManager::getCurrentStreamId() const {
    QMutexLocker locker(&m_mutex);
    return m_currentStreamId;
}

/**
 * @brief 获取流信息
 *
 * 获取指定流的详细信息
 *
 * @param streamId 流ID
 * @return 流信息结构
 */
StreamInfo StreamManager::getStreamInfo(int streamId) const {
    QMutexLocker locker(&m_mutex);

    if (m_streams.contains(streamId)) {
        return m_streams[streamId];
    }

    return StreamInfo();
}

QList<StreamInfo> StreamManager::getAllStreamInfo() const {
    QMutexLocker locker(&m_mutex);
    return m_streams.values();
}

void StreamManager::updateStreamData(int streamId, int dataSize) {
    QMutexLocker locker(&m_mutex);

    if (!m_streams.contains(streamId)) {
        return;
    }

    StreamInfo& info = m_streams[streamId];
    info.lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
    info.frameCount++;
    info.totalBytes += dataSize;
}

bool StreamManager::isStreamValid(int streamId) const {
    QMutexLocker locker(&m_mutex);
    return isValidStreamId(streamId) && m_streams.contains(streamId);
}

int StreamManager::getStreamCount() const {
    QMutexLocker locker(&m_mutex);
    return m_streams.size();
}

/**
 * @brief 清除所有流
 *
 * 清除所有已注册的流
 */
void StreamManager::clearAllStreams() {
    QMutexLocker locker(&m_mutex);

    m_streams.clear();
    m_currentStreamId = -1;

    LOG_INFO("StreamManager", "所有流已清除");
}

void StreamManager::setStreamTimeout(int timeout) {
    QMutexLocker locker(&m_mutex);
    m_streamTimeout = timeout;
    LOG_INFO("StreamManager", QString("流超时时间已设置为 %1ms").arg(timeout));
}

void StreamManager::checkStreamTimeout() {
    QMutexLocker locker(&m_mutex);

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    QList<int> timeoutStreams;

    for (auto it = m_streams.begin(); it != m_streams.end(); ++it) {
        StreamInfo& info = it.value();

        // 只检查激活状态的流
        if (info.status == StreamStatus::Active) {
            if (currentTime - info.lastUpdateTime > m_streamTimeout) {
                timeoutStreams.append(it.key());
            }
        }
    }

    // 处理超时的流
    for (int streamId : timeoutStreams) {
        LOG_WARN("StreamManager", QString("流超时: ID=%1").arg(streamId));

        m_streams[streamId].status = StreamStatus::Error;
        emit streamTimeout(streamId);
        emit streamError(streamId, "流超时");
    }
}

bool StreamManager::isValidStreamId(int streamId) const {
    return streamId >= 0 && streamId < m_maxStreams;
}

/**
 * @brief 分配流ID
 *
 * 分配一个新的可用流ID
 *
 * @return 成功返回流ID，失败返回-1
 */
int StreamManager::allocateStreamId() {
    for (int i = 0; i < m_maxStreams; ++i) {
        if (!m_streams.contains(i)) {
            return i;
        }
    }
    return -1;
}

}  // namespace ground_station