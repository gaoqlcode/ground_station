/**
 * @file framebuffer.cpp
 * @brief 视频帧缓冲区实现
 */

#include "framebuffer.h"
#include <QMutexLocker>

namespace ground_station {

FrameBuffer::FrameBuffer(QObject *parent)
    : QObject(parent)
    , m_maxBufferSize(30)
    , m_droppedFrames(0)
    , m_dropOld(true)
{
}

FrameBuffer::FrameBuffer(int maxBufferSize, QObject *parent)
    : QObject(parent)
    , m_maxBufferSize(maxBufferSize)
    , m_droppedFrames(0)
    , m_dropOld(true)
{
}

FrameBuffer::~FrameBuffer()
{
    clear();
}

void FrameBuffer::setMaxBufferSize(int size)
{
    QMutexLocker locker(&m_mutex);
    m_maxBufferSize = size;
    while (m_buffer.size() > m_maxBufferSize) {
        if (m_dropOld) {
            m_buffer.dequeue();
        } else {
            QQueue<VideoFramePtr> temp;
            while (m_buffer.size() > 1) {
                temp.enqueue(m_buffer.dequeue());
            }
            m_buffer.dequeue();
            m_buffer = temp;
        }
        m_droppedFrames++;
    }
    m_notFull.wakeAll();
    emit bufferStatusChanged(m_buffer.size(), m_maxBufferSize);
}

int FrameBuffer::maxBufferSize() const
{
    QMutexLocker locker(&m_mutex);
    return m_maxBufferSize;
}

int FrameBuffer::currentBufferSize() const
{
    QMutexLocker locker(&m_mutex);
    return m_buffer.size();
}

bool FrameBuffer::pushFrame(const VideoFramePtr &frame)
{
    if (!frame || !frame->isValid) {
        return false;
    }
    QMutexLocker locker(&m_mutex);
    while (m_buffer.size() >= m_maxBufferSize) {
        if (m_dropOld) {
            m_buffer.dequeue();
            m_droppedFrames++;
            break;
        } else {
            m_notFull.wait(&m_mutex, 100);
            if (m_buffer.size() >= m_maxBufferSize) {
                m_droppedFrames++;
                emit bufferOverflow(m_droppedFrames);
                return false;
            }
        }
    }
    m_buffer.enqueue(frame);
    m_latestFrame = frame;
    m_notEmpty.wakeOne();
    emit bufferStatusChanged(m_buffer.size(), m_maxBufferSize);
    emit frameReady(frame->streamId);
    return true;
}

bool FrameBuffer::tryPushFrame(const VideoFramePtr &frame)
{
    if (!frame || !frame->isValid) {
        return false;
    }
    QMutexLocker locker(&m_mutex);
    if (m_buffer.size() >= m_maxBufferSize) {
        if (m_dropOld) {
            m_buffer.dequeue();
            m_droppedFrames++;
        } else {
            m_droppedFrames++;
            emit bufferOverflow(m_droppedFrames);
            return false;
        }
    }
    m_buffer.enqueue(frame);
    m_latestFrame = frame;
    m_notEmpty.wakeOne();
    emit bufferStatusChanged(m_buffer.size(), m_maxBufferSize);
    emit frameReady(frame->streamId);
    return true;
}

VideoFramePtr FrameBuffer::popFrame(int timeoutMs)
{
    QMutexLocker locker(&m_mutex);
    if (m_buffer.isEmpty()) {
        if (timeoutMs == 0) {
            return nullptr;
        }
        if (timeoutMs < 0) {
            m_notEmpty.wait(&m_mutex);
        } else {
            if (!m_notEmpty.wait(&m_mutex, timeoutMs)) {
                return nullptr;
            }
        }
    }
    if (m_buffer.isEmpty()) {
        return nullptr;
    }
    VideoFramePtr frame = m_buffer.dequeue();
    m_notFull.wakeOne();
    emit bufferStatusChanged(m_buffer.size(), m_maxBufferSize);
    return frame;
}

VideoFramePtr FrameBuffer::tryPopFrame()
{
    return popFrame(0);
}

VideoFramePtr FrameBuffer::peekLatestFrame() const
{
    QMutexLocker locker(&m_mutex);
    return m_latestFrame;
}

void FrameBuffer::clear()
{
    QMutexLocker locker(&m_mutex);
    m_buffer.clear();
    m_latestFrame.reset();
    m_notFull.wakeAll();
    emit bufferStatusChanged(0, m_maxBufferSize);
}

bool FrameBuffer::isEmpty() const
{
    QMutexLocker locker(&m_mutex);
    return m_buffer.isEmpty();
}

bool FrameBuffer::isFull() const
{
    QMutexLocker locker(&m_mutex);
    return m_buffer.size() >= m_maxBufferSize;
}

quint64 FrameBuffer::droppedFrames() const
{
    QMutexLocker locker(&m_mutex);
    return m_droppedFrames;
}

void FrameBuffer::resetDroppedFrames()
{
    QMutexLocker locker(&m_mutex);
    m_droppedFrames = 0;
}

void FrameBuffer::setDropPolicy(bool dropOld)
{
    QMutexLocker locker(&m_mutex);
    m_dropOld = dropOld;
}

MultiStreamFrameBuffer::MultiStreamFrameBuffer(QObject *parent)
    : QObject(parent)
    , m_defaultBufferSize(30)
{
}

MultiStreamFrameBuffer::MultiStreamFrameBuffer(int defaultBufferSize, QObject *parent)
    : QObject(parent)
    , m_defaultBufferSize(defaultBufferSize)
{
}

MultiStreamFrameBuffer::~MultiStreamFrameBuffer()
{
    clearAll();
}

void MultiStreamFrameBuffer::addStream(int streamId, int bufferSize)
{
    QMutexLocker locker(&m_mutex);
    if (m_streamBuffers.contains(streamId)) {
        qWarning() << "Stream" << streamId << "already exists";
        return;
    }
    int size = (bufferSize > 0) ? bufferSize : m_defaultBufferSize;
    auto buffer = QSharedPointer<FrameBuffer>::create(size);
    connectBufferSignals(streamId, buffer);
    m_streamBuffers[streamId] = buffer;
    emit streamAdded(streamId);
}

void MultiStreamFrameBuffer::removeStream(int streamId)
{
    QMutexLocker locker(&m_mutex);
    if (!m_streamBuffers.contains(streamId)) {
        return;
    }
    m_streamBuffers.remove(streamId);
    emit streamRemoved(streamId);
}

bool MultiStreamFrameBuffer::hasStream(int streamId) const
{
    QMutexLocker locker(&m_mutex);
    return m_streamBuffers.contains(streamId);
}

QList<int> MultiStreamFrameBuffer::streamIds() const
{
    QMutexLocker locker(&m_mutex);
    return m_streamBuffers.keys();
}

bool MultiStreamFrameBuffer::pushFrame(int streamId, const VideoFramePtr &frame)
{
    QMutexLocker locker(&m_mutex);
    if (!m_streamBuffers.contains(streamId)) {
        qWarning() << "Video stream" << streamId << "not found";
        return false;
    }
    auto buffer = m_streamBuffers[streamId];
    locker.unlock();
    return buffer->tryPushFrame(frame);
}

VideoFramePtr MultiStreamFrameBuffer::popFrame(int streamId, int timeoutMs)
{
    QMutexLocker locker(&m_mutex);
    if (!m_streamBuffers.contains(streamId)) {
        return nullptr;
    }
    auto buffer = m_streamBuffers[streamId];
    locker.unlock();
    return buffer->popFrame(timeoutMs);
}

VideoFramePtr MultiStreamFrameBuffer::peekLatestFrame(int streamId) const
{
    QMutexLocker locker(&m_mutex);
    if (!m_streamBuffers.contains(streamId)) {
        return nullptr;
    }
    return m_streamBuffers[streamId]->peekLatestFrame();
}

void MultiStreamFrameBuffer::clearStream(int streamId)
{
    QMutexLocker locker(&m_mutex);
    if (m_streamBuffers.contains(streamId)) {
        m_streamBuffers[streamId]->clear();
    }
}

void MultiStreamFrameBuffer::clearAll()
{
    QMutexLocker locker(&m_mutex);
    for (auto &buffer : m_streamBuffers) {
        buffer->clear();
    }
}

void MultiStreamFrameBuffer::setStreamBufferSize(int streamId, int size)
{
    QMutexLocker locker(&m_mutex);
    if (m_streamBuffers.contains(streamId)) {
        m_streamBuffers[streamId]->setMaxBufferSize(size);
    }
}

void MultiStreamFrameBuffer::setDefaultBufferSize(int size)
{
    QMutexLocker locker(&m_mutex);
    m_defaultBufferSize = size;
}

void MultiStreamFrameBuffer::connectBufferSignals(int streamId, const QSharedPointer<FrameBuffer> &buffer)
{
    connect(buffer.data(), &FrameBuffer::frameReady, this, [this, streamId](int) {
        emit frameReady(streamId);
    });
}

} // namespace ground_station
