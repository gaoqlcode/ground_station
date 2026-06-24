/**
 * @file videodecoder.cpp
 * @brief 视频解码器实现
 */

#include "videodecoder.h"

#include <QDebug>
#include <QMetaObject>
#include <QMutexLocker>
#include <opencv2/imgcodecs.hpp>

namespace ground_station {

DecodeWorker::DecodeWorker(QObject *parent)
    : QObject(parent)
    , m_videoStats(nullptr)
    , m_running(true)
{
    m_timer.start();
}

DecodeWorker::~DecodeWorker() = default;

void DecodeWorker::setFrameBuffer(const QSharedPointer<FrameBuffer> &buffer)
{
    m_frameBuffer = buffer;
}

void DecodeWorker::setVideoStats(VideoStats *stats)
{
    m_videoStats = stats;
}

void DecodeWorker::decodeFrame(const EncodedFrame &frame)
{
    if (!m_running) {
        return;
    }

    if (frame.data.isEmpty()) {
        emit decodeError(frame.frameId, frame.streamId, "空的编码帧数据");
        return;
    }

    qint64 startTime = m_timer.elapsed();

    cv::Mat decodedImage;
    try {
        cv::_InputArray inputArray(reinterpret_cast<const uchar *>(frame.data.constData()),
                                   static_cast<int>(frame.data.size()));
        decodedImage = cv::imdecode(inputArray, cv::IMREAD_COLOR);

        if (decodedImage.empty()) {
            emit decodeError(frame.frameId, frame.streamId, "解码结果为空");
            return;
        }
    } catch (const cv::Exception &e) {
        emit decodeError(frame.frameId, frame.streamId, QString("OpenCV错误: %1").arg(e.what()));
        return;
    } catch (const std::exception &e) {
        emit decodeError(frame.frameId, frame.streamId, QString("解码错误: %1").arg(e.what()));
        return;
    }

    qint64 endTime = m_timer.elapsed();
    double decodeTimeMs = static_cast<double>(endTime - startTime);

    auto videoFrame = VideoFramePtr::create();
    videoFrame->frameId = frame.frameId;
    videoFrame->streamId = frame.streamId;
    videoFrame->width = decodedImage.cols;
    videoFrame->height = decodedImage.rows;
    videoFrame->channels = decodedImage.channels();
    videoFrame->timestamp = frame.timestamp;
    videoFrame->captureTime = frame.captureTime;
    videoFrame->decodeTime = decodeTimeMs;
    videoFrame->data = decodedImage;
    videoFrame->isValid = true;

    if (m_frameBuffer) {
        if (!m_frameBuffer->tryPushFrame(videoFrame)) {
            qWarning() << "Frame buffer full, frame dropped:" << frame.frameId;
        }
    }

    if (m_videoStats) {
        m_videoStats->recordFrameDecoded(frame.streamId, frame.frameId, decodeTimeMs);
    }

    emit frameDecoded(frame.frameId, frame.streamId, decodeTimeMs);
}

void DecodeWorker::stop()
{
    m_running = false;
}

VideoDecoder::VideoDecoder(QObject *parent)
    : QObject(parent)
    , m_videoStats(nullptr)
    , m_maxPendingSize(100)
    , m_currentThreadIndex(0)
    , m_initialized(false)
{
}

VideoDecoder::~VideoDecoder()
{
    shutdown();
}

bool VideoDecoder::initialize(int threadCount)
{
    QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        qWarning() << "解码器已初始化";
        return true;
    }

    if (threadCount < 1) {
        threadCount = 1;
    }

    for (int i = 0; i < threadCount; ++i) {
        DecodeThread dt;
        dt.thread = new QThread(this);
        dt.worker = new DecodeWorker(dt.thread);
        dt.worker->moveToThread(dt.thread);

        connect(dt.worker, &DecodeWorker::frameDecoded,
                this, &VideoDecoder::frameDecoded);
        connect(dt.worker, &DecodeWorker::decodeError,
                this, &VideoDecoder::decodeError);
        connect(dt.thread, &QThread::finished,
                dt.worker, &QObject::deleteLater);

        dt.thread->start();
        m_decodeThreads.append(dt);
    }

    m_initialized = true;
    qDebug() << "VideoDecoder initialized with" << threadCount << "threads";
    return true;
}

void VideoDecoder::shutdown()
{
    QMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        return;
    }

    for (auto &dt : m_decodeThreads) {
        if (dt.worker) {
            dt.worker->stop();
        }
        if (dt.thread) {
            dt.thread->quit();
            dt.thread->wait();
            dt.thread->deleteLater();
        }
    }

    m_decodeThreads.clear();
    m_pendingQueue.clear();
    m_initialized = false;
    qDebug() << "VideoDecoder shutdown";
}

void VideoDecoder::setFrameBuffer(int streamId, const QSharedPointer<FrameBuffer> &buffer)
{
    QMutexLocker locker(&m_mutex);
    m_frameBuffers[streamId] = buffer;

    for (auto &dt : m_decodeThreads) {
        if (dt.worker) {
            dt.worker->setFrameBuffer(buffer);
        }
    }
}

void VideoDecoder::setVideoStats(VideoStats *stats)
{
    QMutexLocker locker(&m_mutex);
    m_videoStats = stats;

    for (auto &dt : m_decodeThreads) {
        if (dt.worker) {
            dt.worker->setVideoStats(stats);
        }
    }
}

bool VideoDecoder::decode(const EncodedFrame &frame)
{
    QMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        qWarning() << "Decoder not initialized";
        return false;
    }

    if (m_pendingQueue.size() >= m_maxPendingSize) {
        qWarning() << "Pending queue full, dropping frame:" << frame.frameId;
        return false;
    }

    m_pendingQueue.enqueue(frame);
    dispatchFrame(frame);
    return true;
}

int VideoDecoder::decodeBatch(const QList<EncodedFrame> &frames)
{
    QMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        qWarning() << "解码器未初始化";
        return 0;
    }

    int successCount = 0;
    for (const auto &frame : frames) {
        if (m_pendingQueue.size() >= m_maxPendingSize) {
            qWarning() << "待解码队列已满，停止批量解码";
            break;
        }

        m_pendingQueue.enqueue(frame);
        dispatchFrame(frame);
        successCount++;
    }

    return successCount;
}

int VideoDecoder::pendingQueueSize() const
{
    QMutexLocker locker(&m_mutex);
    return m_pendingQueue.size();
}

void VideoDecoder::clearPendingQueue()
{
    QMutexLocker locker(&m_mutex);
    m_pendingQueue.clear();
}

void VideoDecoder::setMaxPendingSize(int size)
{
    QMutexLocker locker(&m_mutex);
    m_maxPendingSize = size;
}

bool VideoDecoder::isInitialized() const
{
    QMutexLocker locker(&m_mutex);
    return m_initialized;
}

int VideoDecoder::threadCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_decodeThreads.size();
}

void VideoDecoder::dispatchFrame(const EncodedFrame &frame)
{
    if (m_decodeThreads.isEmpty()) {
        return;
    }

    DecodeThread &dt = m_decodeThreads[m_currentThreadIndex];
    m_currentThreadIndex = (m_currentThreadIndex + 1) % m_decodeThreads.size();

    QMetaObject::invokeMethod(dt.worker, "decodeFrame",
                              Qt::QueuedConnection,
                              Q_ARG(EncodedFrame, frame));
}

} // namespace ground_station

