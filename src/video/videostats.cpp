#include "videostats.h"
#include <QMutexLocker>

namespace ground_station {

VideoStats::VideoStats(QObject *parent)
    : QObject(parent)
    , m_defaultFpsWindowSize(30)
    , m_defaultLatencyWindowSize(30)
    , m_highLatencyThreshold(500.0)
    , m_highDropRateThreshold(0.1)
{
}

VideoStats::~VideoStats()
{
}

void VideoStats::recordFrameReceived(int streamId, quint64 frameId, const FrameTimestamp &timestamp)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_streamStats.contains(streamId)) {
        initializeStreamStats(streamId);
    }
    
    VideoStats::StreamStats &stats = m_streamStats[streamId];
    stats.pendingFrames[frameId] = timestamp;
    stats.pendingFrames[frameId].receiveTime = QDateTime::currentMSecsSinceEpoch() * 1000;
}

void VideoStats::recordFrameDecoded(int streamId, quint64 frameId, double decodeTimeMs)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_streamStats.contains(streamId)) {
        initializeStreamStats(streamId);
    }
    
    VideoStats::StreamStats &stats = m_streamStats[streamId];
    
    stats.decodeTimeHistory.enqueue(decodeTimeMs);
    if (stats.decodeTimeHistory.size() > m_defaultLatencyWindowSize) {
        stats.decodeTimeHistory.dequeue();
    }
    
    if (stats.pendingFrames.contains(frameId)) {
        stats.pendingFrames[frameId].decodeTime = QDateTime::currentMSecsSinceEpoch() * 1000;
    }
}

void VideoStats::recordFrameRendered(int streamId, quint64 frameId, double renderTimeMs)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_streamStats.contains(streamId)) {
        initializeStreamStats(streamId);
    }
    
    VideoStats::StreamStats &stats = m_streamStats[streamId];
    
    stats.totalFrames++;
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    stats.frameTimestamps.enqueue(currentTime);
    if (stats.frameTimestamps.size() > stats.fpsWindowSize) {
        stats.frameTimestamps.dequeue();
    }
    
    stats.renderTimeHistory.enqueue(renderTimeMs);
    if (stats.renderTimeHistory.size() > m_defaultLatencyWindowSize) {
        stats.renderTimeHistory.dequeue();
    }
    
    if (stats.pendingFrames.contains(frameId)) {
        FrameTimestamp &ts = stats.pendingFrames[frameId];
        ts.renderTime = currentTime * 1000;
        
        double endToEndLatency = (ts.renderTime - ts.captureTime) / 1000.0;
        stats.latencyHistory.enqueue(endToEndLatency);
        if (stats.latencyHistory.size() > stats.latencyWindowSize) {
            stats.latencyHistory.dequeue();
        }
        
        stats.pendingFrames.remove(frameId);
    }
    
    stats.lastFrameId = frameId;
    
    VideoStatsInfo info = getStats(streamId);
    emit statsUpdated(streamId, info);
}

void VideoStats::recordDroppedFrames(int streamId, quint64 count)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_streamStats.contains(streamId)) {
        initializeStreamStats(streamId);
    }
    
    VideoStats::StreamStats &stats = m_streamStats[streamId];
    stats.droppedFrames += count;
    
    double dropRate = static_cast<double>(stats.droppedFrames) / 
                      static_cast<double>(stats.totalFrames + stats.droppedFrames);
    
    if (dropRate > m_highDropRateThreshold) {
        emit highDropRateWarning(streamId, dropRate);
    }
}

VideoStatsInfo VideoStats::getStats(int streamId) const
{
    QMutexLocker locker(&const_cast<QMutex&>(m_mutex));
    
    VideoStatsInfo info;
    info.fps = 0.0;
    info.decodeTime = 0.0;
    info.renderTime = 0.0;
    info.endToEndLatency = 0.0;
    info.totalFrames = 0;
    info.droppedFrames = 0;
    info.dropRate = 0.0;
    info.currentBitrate = 0;
    
    if (!m_streamStats.contains(streamId)) {
        return info;
    }
    
    const VideoStats::StreamStats &stats = m_streamStats[streamId];
    
    info.fps = calculateFps(const_cast<VideoStats::StreamStats&>(stats));
    info.endToEndLatency = calculateAverageLatency(const_cast<VideoStats::StreamStats&>(stats));
    info.decodeTime = calculateAverageDecodeTime(const_cast<VideoStats::StreamStats&>(stats));
    info.renderTime = calculateAverageRenderTime(const_cast<VideoStats::StreamStats&>(stats));
    
    info.totalFrames = stats.totalFrames;
    info.droppedFrames = stats.droppedFrames;
    
    quint64 total = stats.totalFrames + stats.droppedFrames;
    if (total > 0) {
        info.dropRate = static_cast<double>(stats.droppedFrames) / static_cast<double>(total);
    }
    
    info.currentBitrate = stats.currentBitrate;
    
    return info;
}

QMap<int, VideoStatsInfo> VideoStats::getAllStats() const
{
    QMutexLocker locker(&const_cast<QMutex&>(m_mutex));
    
    QMap<int, VideoStatsInfo> allStats;
    for (int streamId : m_streamStats.keys()) {
        allStats[streamId] = getStats(streamId);
    }
    
    return allStats;
}

void VideoStats::resetStats(int streamId)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_streamStats.contains(streamId)) {
        m_streamStats.remove(streamId);
    }
}

void VideoStats::resetAllStats()
{
    QMutexLocker locker(&m_mutex);
    m_streamStats.clear();
}

void VideoStats::setFpsWindowSize(int windowSize)
{
    QMutexLocker locker(&m_mutex);
    m_defaultFpsWindowSize = windowSize;
}

void VideoStats::setLatencyWindowSize(int windowSize)
{
    QMutexLocker locker(&m_mutex);
    m_defaultLatencyWindowSize = windowSize;
}

void VideoStats::initializeStreamStats(int streamId)
{
    VideoStats::StreamStats stats;
    stats.fpsWindowSize = m_defaultFpsWindowSize;
    stats.latencyWindowSize = m_defaultLatencyWindowSize;
    stats.totalFrames = 0;
    stats.droppedFrames = 0;
    stats.lastFrameId = 0;
    stats.currentBitrate = 0;
    
    m_streamStats[streamId] = stats;
}

double VideoStats::calculateFps(VideoStats::StreamStats &stats) const
{
    if (stats.frameTimestamps.size() < 2) {
        return 0.0;
    }
    
    qint64 firstTime = stats.frameTimestamps.first();
    qint64 lastTime = stats.frameTimestamps.last();
    qint64 duration = lastTime - firstTime;
    
    if (duration <= 0) {
        return 0.0;
    }
    
    double seconds = duration / 1000.0;
    return static_cast<double>(stats.frameTimestamps.size() - 1) / seconds;
}

double VideoStats::calculateAverageLatency(VideoStats::StreamStats &stats) const
{
    if (stats.latencyHistory.isEmpty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (double latency : stats.latencyHistory) {
        sum += latency;
    }
    
    return sum / static_cast<double>(stats.latencyHistory.size());
}

double VideoStats::calculateAverageDecodeTime(VideoStats::StreamStats &stats) const
{
    if (stats.decodeTimeHistory.isEmpty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (double time : stats.decodeTimeHistory) {
        sum += time;
    }
    
    return sum / static_cast<double>(stats.decodeTimeHistory.size());
}

double VideoStats::calculateAverageRenderTime(VideoStats::StreamStats &stats) const
{
    if (stats.renderTimeHistory.isEmpty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (double time : stats.renderTimeHistory) {
        sum += time;
    }
    
    return sum / static_cast<double>(stats.renderTimeHistory.size());
}

} // namespace ground_station