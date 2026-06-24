/**
 * @file udpreceiver.cpp
 * @brief UDP接收器实现
 */

#include "udpreceiver.h"
#include <QDebug>
#include <QDateTime>
#include <QMutexLocker>

namespace ground_station {

UdpReceiver::UdpReceiver(QObject *parent)
    : QObject(parent)
    , socket_(nullptr)
    , reassembly_(nullptr)
    , stats_timer_(nullptr)
    , local_port_(DEFAULT_UDP_PORT)
    , buffer_size_(DEFAULT_BUFFER_SIZE)
    , state_(ReceiverState::Idle)
    , last_packets_(0)
    , last_bytes_(0)
    , last_frames_(0)
    , last_source_port_(0)
{
    initialize();
}

UdpReceiver::UdpReceiver(int port, QObject *parent)
    : QObject(parent)
    , socket_(nullptr)
    , reassembly_(nullptr)
    , stats_timer_(nullptr)
    , local_port_(port)
    , buffer_size_(DEFAULT_BUFFER_SIZE)
    , state_(ReceiverState::Idle)
    , last_packets_(0)
    , last_bytes_(0)
    , last_frames_(0)
    , last_source_port_(0)
{
    initialize();
}

UdpReceiver::~UdpReceiver()
{
    stop();
    
    if (reassembly_) {
        delete reassembly_;
        reassembly_ = nullptr;
    }
    
    if (stats_timer_) {
        delete stats_timer_;
        stats_timer_ = nullptr;
    }
    
    if (socket_) {
        delete socket_;
        socket_ = nullptr;
    }
}

void UdpReceiver::initialize()
{
    socket_ = new QUdpSocket(this);
    reassembly_ = new PacketReassembly(this);
    stats_timer_ = new QTimer(this);
    
    connect(socket_, &QUdpSocket::readyRead,
            this, &UdpReceiver::onReadyRead);
    connect(socket_, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(
            &QAbstractSocket::error),
            this, &UdpReceiver::onSocketError);
    connect(reassembly_, &PacketReassembly::frameReassembled,
            this, &UdpReceiver::onFrameReassembled);
    connect(reassembly_, &PacketReassembly::frameTimeout,
            this, &UdpReceiver::onFrameTimeout);
    connect(reassembly_, &PacketReassembly::errorOccurred,
            this, &UdpReceiver::onReassemblyError);
    connect(stats_timer_, &QTimer::timeout,
            this, &UdpReceiver::onStatsTimer);
    
    last_stats_time_ = QDateTime::currentMSecsSinceEpoch();
}

void UdpReceiver::setLocalPort(int port)
{
    local_port_ = port;
}

int UdpReceiver::getLocalPort() const
{
    return local_port_;
}

void UdpReceiver::setBufferSize(int size)
{
    buffer_size_ = size;
}

void UdpReceiver::setFrameTimeout(std::uint32_t timeout_ms)
{
    if (reassembly_) {
        reassembly_->setFrameTimeout(timeout_ms);
    }
}

void UdpReceiver::setMaxConcurrentFrames(std::size_t max_frames)
{
    if (reassembly_) {
        reassembly_->setMaxConcurrentFrames(max_frames);
    }
}

void UdpReceiver::setCrcCheckEnabled(bool enabled)
{
    if (reassembly_) {
        reassembly_->setCrcCheckEnabled(enabled);
    }
}

void UdpReceiver::setStatsInterval(std::uint32_t interval_ms)
{
    stats_timer_->setInterval(static_cast<int>(interval_ms));
}

bool UdpReceiver::start()
{
    if (state_ == ReceiverState::Listening || state_ == ReceiverState::Receiving) {
        return true;
    }
    
    updateState(ReceiverState::Binding);
    
    socket_->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, buffer_size_);
    
    if (!socket_->bind(QHostAddress::Any, local_port_,
                       QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        QString error_msg = QString("Failed to bind UDP socket to port %1: %2")
                           .arg(local_port_)
                           .arg(socket_->errorString());
        emit errorOccurred(ReceiverError::BindError, error_msg);
        updateState(ReceiverState::Error);
        return false;
    }
    
    reassembly_->start();
    stats_timer_->start(1000);
    
    updateState(ReceiverState::Listening);
    
    qDebug() << "UDP receiver started, bound to port" << local_port_;
    
    return true;
}

void UdpReceiver::stop()
{
    if (state_ == ReceiverState::Idle) {
        return;
    }
    
    stats_timer_->stop();
    reassembly_->stop();
    socket_->close();
    
    updateState(ReceiverState::Idle);
    
    qDebug() << "UDP receiver stopped";
}

bool UdpReceiver::isRunning() const
{
    return state_ == ReceiverState::Listening || state_ == ReceiverState::Receiving;
}

ReceiverState UdpReceiver::getState() const
{
    return state_;
}

void UdpReceiver::clearBuffers()
{
    if (reassembly_) {
        reassembly_->clearBuffers();
    }
}

void UdpReceiver::resetStats()
{
    QMutexLocker locker(&stats_mutex_);
    stats_ = ReceiverStats();
    last_packets_ = 0;
    last_bytes_ = 0;
    last_frames_ = 0;
    last_stats_time_ = QDateTime::currentMSecsSinceEpoch();
}

ReceiverStats UdpReceiver::getStats() const
{
    QMutexLocker locker(&stats_mutex_);
    return stats_;
}

PacketReassembly* UdpReceiver::getReassembly()
{
    return reassembly_;
}

QUdpSocket* UdpReceiver::getSocket()
{
    return socket_;
}

void UdpReceiver::onReadyRead()
{
    processPendingDatagrams();
}

void UdpReceiver::onSocketError(QAbstractSocket::SocketError error)
{
    QString error_msg = QString("UDP socket error: %1 (%2)")
                       .arg(static_cast<int>(error))
                       .arg(socket_->errorString());
    
    emit errorOccurred(ReceiverError::SocketError, error_msg);
    
    {
        QMutexLocker locker(&stats_mutex_);
        stats_.last_error = error_msg;
    }
}

void UdpReceiver::onFrameReassembled(const ReceivedFrame &frame)
{
    ReceivedFrame video_frame;
    video_frame.frame_id = frame.frame_id;
    video_frame.timestamp = frame.timestamp;
    video_frame.data = frame.data;
    video_frame.fragment_count = frame.fragment_count;
    video_frame.recv_duration = frame.recv_duration;
    video_frame.source_address = last_source_address_;
    video_frame.source_port = last_source_port_;
    
    {
        QMutexLocker locker(&stats_mutex_);
        stats_.total_frames++;
        stats_.complete_frames++;
    }
    
    emit frameReceived(video_frame);
    
    if (state_ == ReceiverState::Listening) {
        updateState(ReceiverState::Receiving);
    }
}

void UdpReceiver::onFrameTimeout(std::uint32_t frame_id)
{
    QMutexLocker locker(&stats_mutex_);
    stats_.timeout_frames++;
    emit frameTimeout(frame_id);
}

void UdpReceiver::onReassemblyError(ReassemblyError error, const QString &message)
{
    ReceiverError recv_error = ReceiverError::ReassemblyError;
    
    switch (error) {
    case ReassemblyError::InvalidCrc:
        recv_error = ReceiverError::ReassemblyError;
        {
            QMutexLocker locker(&stats_mutex_);
            stats_.crc_errors++;
        }
        break;
    case ReassemblyError::Timeout:
        recv_error = ReceiverError::TimeoutError;
        break;
    case ReassemblyError::BufferFull:
        recv_error = ReceiverError::BufferOverflow;
        break;
    default:
        break;
    }
    
    {
        QMutexLocker locker(&stats_mutex_);
        stats_.invalid_packets++;
        stats_.last_error = message;
    }
    
    emit errorOccurred(recv_error, message);
}

void UdpReceiver::onStatsTimer()
{
    updateStats();
}

void UdpReceiver::processPendingDatagrams()
{
    while (socket_->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(socket_->pendingDatagramSize());
        
        QHostAddress sender;
        quint16 sender_port;
        
        socket_->readDatagram(datagram.data(), datagram.size(), &sender, &sender_port);
        
        last_source_address_ = sender;
        last_source_port_ = sender_port;
        
        std::size_t size = static_cast<std::size_t>(datagram.size());
        
        {
            QMutexLocker locker(&stats_mutex_);
            stats_.total_packets++;
            stats_.total_bytes += size;
        }
        
        emit packetReceived(size);
        
        ReassemblyError error = reassembly_->processPacket(datagram);
        
        if (error != ReassemblyError::None && error != ReassemblyError::InvalidCrc) {
            QString error_msg = QString("Packet processing error");
            
            {
                QMutexLocker locker(&stats_mutex_);
                stats_.invalid_packets++;
                stats_.last_error = error_msg;
            }
        }
    }
}

void UdpReceiver::updateState(ReceiverState state)
{
    if (state_ != state) {
        state_ = state;
        emit stateChanged(state);
    }
}

void UdpReceiver::updateStats()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    double seconds = (now - last_stats_time_) / 1000.0;
    
    if (seconds <= 0) {
        return;
    }
    
    QMutexLocker locker(&stats_mutex_);
    
    std::uint64_t current_packets = stats_.total_packets;
    std::uint64_t current_bytes = stats_.total_bytes;
    std::uint64_t current_frames = stats_.total_frames;
    
    stats_.packets_per_second = (current_packets - last_packets_) / seconds;
    stats_.bytes_per_second = (current_bytes - last_bytes_) / seconds;
    stats_.frames_per_second = (current_frames - last_frames_) / seconds;
    
    last_packets_ = current_packets;
    last_bytes_ = current_bytes;
    last_frames_ = current_frames;
    last_stats_time_ = now;
    
    emit statsUpdated(stats_);
}

} // namespace ground_station
