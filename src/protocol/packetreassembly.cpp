/**
 * @file packetreassembly.cpp
 * @brief 数据包重组模块实现
 *
 * 实现UDP分片数据包重组功能，包括：
 * - CRC32校验
 * - 基于帧ID的分片重组
 * - 超时管理机制
 * - 多帧并发处理
 */

#include "packetreassembly.h"

#include <QDateTime>
#include <QMutexLocker>
#include <QtGlobal>

namespace ground_station {

/**
 * @brief CRC32查找表
 * 
 * 使用标准CRC32算法（IEEE 802.3）预计算的查找表，用于快速计算CRC值
 */
static const std::uint32_t CRC32_TABLE[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCCB40,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3AB0072, 0xD4BB30E9,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C09066A, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

// ============================================================================
// ReassemblyFrameBuffer 类实现
// ============================================================================

/**
 * @brief 构造函数
 * @param fid 帧ID
 * @param total 总分片数
 */
ReassemblyFrameBuffer::ReassemblyFrameBuffer(std::uint32_t fid, std::uint16_t total)
    : frame_id(fid)
    , total_fragments(total)
    , received_count(0)
    , first_recv_time(0)
    , last_recv_time(0)
    , timestamp(0)
{
    // 初始化分片位图和分片存储数组
    fragment_bitmap.resize(total);
    fragment_bitmap.fill(false);
    fragments.resize(total);
}

/**
 * @brief 添加分片数据
 * @param frag_index 分片索引（从0开始）
 * @param data 分片数据
 * @return true=添加成功，false=索引无效
 */
bool ReassemblyFrameBuffer::addFragment(std::uint16_t frag_index, const QByteArray& data)
{
    // 验证分片索引有效性
    if (frag_index >= total_fragments) {
        return false;
    }
    // 如果该分片已接收，直接返回成功（幂等性）
    if (fragment_bitmap[frag_index]) {
        return true;
    }
    // 存储分片数据并标记接收状态
    fragments[frag_index] = data;
    fragment_bitmap[frag_index] = true;
    received_count++;
    return true;
}

/**
 * @brief 检查帧是否完整
 * @return true=所有分片已接收，false=还缺少分片
 */
bool ReassemblyFrameBuffer::isComplete() const
{
    return received_count == total_fragments;
}

/**
 * @brief 重组帧数据
 * @param out_data 输出的完整帧数据
 * @return true=重组成功，false=帧不完整
 */
bool ReassemblyFrameBuffer::reassemble(QByteArray& out_data) const
{
    // 帧不完整则无法重组
    if (!isComplete()) {
        return false;
    }
    
    // 计算总大小并预分配内存
    std::size_t total_size = 0;
    for (const auto& frag : fragments) {
        total_size += static_cast<std::size_t>(frag.size());
    }
    out_data.reserve(static_cast<int>(total_size));
    
    // 按顺序拼接所有分片
    for (const auto& frag : fragments) {
        out_data.append(frag);
    }
    return true;
}

// ============================================================================
// PacketReassembly 类实现
// ============================================================================

/**
 * @brief 构造函数
 * @param parent 父对象指针
 */
PacketReassembly::PacketReassembly(QObject* parent)
    : QObject(parent)
    , timeout_timer_(new QTimer(this))
    , frame_timeout_ms_(DEFAULT_FRAME_TIMEOUT_MS)
    , max_concurrent_frames_(MAX_CONCURRENT_FRAMES)
    , crc_check_enabled_(true)
    , running_(false)
    , total_packets_(0)
    , total_frames_(0)
    , complete_frames_(0)
    , timeout_frames_(0)
    , crc_errors_(0)
{
    // 连接超时检查定时器
    connect(timeout_timer_, &QTimer::timeout,
            this, &PacketReassembly::onFrameTimeout);
}

/**
 * @brief 析构函数
 */
PacketReassembly::~PacketReassembly()
{
    stop();
    clearBuffers();
}

/**
 * @brief 设置帧超时时间
 * @param timeout_ms 超时时间（毫秒）
 */
void PacketReassembly::setFrameTimeout(std::uint32_t timeout_ms)
{
    QMutexLocker locker(&mutex_);
    frame_timeout_ms_ = timeout_ms;
}

/**
 * @brief 设置最大并发处理帧数
 * @param max_frames 最大并发帧数
 */
void PacketReassembly::setMaxConcurrentFrames(std::size_t max_frames)
{
    QMutexLocker locker(&mutex_);
    max_concurrent_frames_ = max_frames;
}

/**
 * @brief 设置是否启用CRC校验
 * @param enabled true=启用，false=禁用
 */
void PacketReassembly::setCrcCheckEnabled(bool enabled)
{
    QMutexLocker locker(&mutex_);
    crc_check_enabled_ = enabled;
}

/**
 * @brief 启动重组器
 */
void PacketReassembly::start()
{
    if (running_) {
        return;
    }
    running_ = true;
    timeout_timer_->start(1000);  // 每秒检查一次超时帧
}

/**
 * @brief 停止重组器
 */
void PacketReassembly::stop()
{
    if (!running_) {
        return;
    }
    running_ = false;
    timeout_timer_->stop();
}

/**
 * @brief 处理接收到的UDP数据包
 * @param packet 原始数据包（包含协议头）
 * @return 重组错误类型，None表示成功
 */
ReassemblyError PacketReassembly::processPacket(const QByteArray& packet)
{
    QMutexLocker locker(&mutex_);
    
    // 如果重组器未运行，直接返回
    if (!running_) {
        return ReassemblyError::None;
    }
    
    // 检查最小包大小
    if (packet.size() < static_cast<int>(UDP_HEADER_SIZE)) {
        return ReassemblyError::InvalidHeader;
    }
    
    // 统计处理的包数
    total_packets_++;
    
    // 解析协议头
    const UdpHeader* header = reinterpret_cast<const UdpHeader*>(packet.constData());
    
    // 验证协议头
    if (!validateHeader(header)) {
        return ReassemblyError::InvalidHeader;
    }
    
    // 提取负载数据
    QByteArray payload(packet.constData() + UDP_HEADER_SIZE, 
                      packet.size() - static_cast<int>(UDP_HEADER_SIZE));
    
    // 如果启用CRC校验，验证CRC
    if (crc_check_enabled_ && !validateCrc(packet, header)) {
        crc_errors_++;
        return ReassemblyError::InvalidCrc;
    }
    
    // 根据数据包类型进行处理
    switch (static_cast<UdpPacketType>(header->type)) {
    case UdpPacketType::FrameData:
        return processFrameData(header, payload);
    case UdpPacketType::Ack:
    case UdpPacketType::Nack:
    case UdpPacketType::Heartbeat:
    case UdpPacketType::FlowControl:
        return ReassemblyError::None;
    default:
        return ReassemblyError::InvalidHeader;
    }
}

/**
 * @brief 清空所有帧缓冲区
 */
void PacketReassembly::clearBuffers()
{
    QMutexLocker locker(&mutex_);
    qDeleteAll(frame_buffers_);
    frame_buffers_.clear();
    qDeleteAll(frame_timers_);
    frame_timers_.clear();
}

/**
 * @brief 获取统计信息
 * @param total_packets 输出：处理的总包数
 * @param total_frames 输出：处理的总帧数
 * @param complete_frames 输出：成功重组的帧数
 * @param timeout_frames 输出：超时丢弃的帧数
 * @param crc_errors 输出：CRC错误数
 */
void PacketReassembly::getStatistics(std::uint64_t& total_packets, std::uint64_t& total_frames,
                                     std::uint64_t& complete_frames, std::uint64_t& timeout_frames,
                                     std::uint64_t& crc_errors)
{
    QMutexLocker locker(&mutex_);
    total_packets = total_packets_;
    total_frames = total_frames_;
    complete_frames = complete_frames_;
    timeout_frames = timeout_frames_;
    crc_errors = crc_errors_;
}

/**
 * @brief 帧超时定时器槽函数
 */
void PacketReassembly::onFrameTimeout()
{
    cleanupOldFrames();
}

/**
 * @brief 验证UDP协议头
 * @param header 协议头指针
 * @return true=验证通过，false=验证失败
 */
bool PacketReassembly::validateHeader(const UdpHeader* header)
{
    // 验证魔数
    if (header->magic != UDP_MAGIC_NUMBER) {
        return false;
    }
    // 验证版本号
    if (header->version != UDP_PROTOCOL_VERSION) {
        return false;
    }
    // 验证分片索引和总数的有效性
    if (header->fragment_count == 0 || header->fragment_index >= header->fragment_count) {
        return false;
    }
    return true;
}

/**
 * @brief 验证CRC校验值
 * @param packet 完整数据包
 * @param header 协议头指针
 * @return true=校验通过，false=校验失败
 */
bool PacketReassembly::validateCrc(const QByteArray& packet, const UdpHeader* header)
{
    // 仅计算负载数据的CRC
    const char* payload = packet.constData() + UDP_HEADER_SIZE;
    int payload_size = packet.size() - static_cast<int>(UDP_HEADER_SIZE);
    
    // 使用查找表计算CRC32
    std::uint32_t crc = 0xFFFFFFFF;
    for (int i = 0; i < payload_size; ++i) {
        std::uint8_t byte = static_cast<std::uint8_t>(payload[i]);
        crc = CRC32_TABLE[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    crc = crc ^ 0xFFFFFFFF;
    
    return crc == header->crc32;
}

/**
 * @brief 处理分片数据
 * @param frame_id 帧ID
 * @param fragment_index 分片索引
 * @param fragment_count 总分片数
 * @param payload 分片负载数据
 */
void PacketReassembly::processFragment(std::uint32_t frame_id, std::uint16_t fragment_index,
                                       std::uint16_t fragment_count, const QByteArray& payload)
{
    auto it = frame_buffers_.find(frame_id);
    
    // 如果是新帧，创建新的缓冲区
    if (it == frame_buffers_.end()) {
        // 检查是否超过最大并发帧数限制
        if (frame_buffers_.size() >= max_concurrent_frames_) {
            // 移除最旧的帧
            std::uint32_t oldest_id = frame_buffers_.keys().first();
            delete frame_buffers_.take(oldest_id);
            delete frame_timers_.take(oldest_id);
        }
        
        // 创建新的帧缓冲区
        ReassemblyFrameBuffer* buffer = new ReassemblyFrameBuffer(frame_id, fragment_count);
        buffer->first_recv_time = static_cast<std::uint64_t>(QDateTime::currentMSecsSinceEpoch());
        frame_buffers_[frame_id] = buffer;
        total_frames_++;
        
        // 为该帧创建超时定时器
        QTimer* timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, [this, frame_id]() {
            emit frameTimeout(frame_id);
            cleanupFrame(frame_id);
        });
        timer->start(static_cast<int>(frame_timeout_ms_));
        frame_timers_[frame_id] = timer;
        
        it = frame_buffers_.find(frame_id);
    }
    
    // 更新最后接收时间并添加分片
    ReassemblyFrameBuffer* buffer = it.value();
    buffer->last_recv_time = static_cast<std::uint64_t>(QDateTime::currentMSecsSinceEpoch());
    buffer->addFragment(fragment_index, payload);
    
    // 检查帧是否完整
    checkFrameComplete(frame_id);
}

/**
 * @brief 检查帧是否完整并触发信号
 * @param frame_id 帧ID
 */
void PacketReassembly::checkFrameComplete(std::uint32_t frame_id)
{
    auto it = frame_buffers_.find(frame_id);
    if (it == frame_buffers_.end()) {
        return;
    }
    
    ReassemblyFrameBuffer* buffer = it.value();
    if (!buffer->isComplete()) {
        return;
    }
    
    // 重组帧数据
    QByteArray data;
    if (buffer->reassemble(data)) {
        ReceivedFrame frame;
        frame.frame_id = buffer->frame_id;
        frame.timestamp = buffer->timestamp;
        frame.data = data;
        frame.fragment_count = buffer->total_fragments;
        frame.recv_duration = buffer->last_recv_time - buffer->first_recv_time;
        
        // 更新统计并发送帧重组完成信号
        complete_frames_++;
        emit frameReassembled(frame);
    }
    
    // 清理已完成的帧缓冲区
    cleanupFrame(frame_id);
}

/**
 * @brief 清理超时的帧缓冲区
 */
void PacketReassembly::cleanupOldFrames()
{
    QMutexLocker locker(&mutex_);
    
    std::uint64_t current_time = static_cast<std::uint64_t>(QDateTime::currentMSecsSinceEpoch());
    QList<std::uint32_t> timed_out;
    
    // 找出所有超时的帧
    for (auto it = frame_buffers_.begin(); it != frame_buffers_.end(); ++it) {
        if (current_time - it.value()->first_recv_time > frame_timeout_ms_) {
            timed_out.append(it.key());
        }
    }
    
    // 清理超时帧
    for (std::uint32_t frame_id : timed_out) {
        timeout_frames_++;
        emit frameTimeout(frame_id);
        cleanupFrame(frame_id);
    }
}

/**
 * @brief 清理指定帧的缓冲区
 * @param frame_id 帧ID
 */
void PacketReassembly::cleanupFrame(std::uint32_t frame_id)
{
    // 停止并删除定时器
    QTimer* timer = frame_timers_.take(frame_id);
    if (timer) {
        timer->stop();
        delete timer;
    }
    
    // 删除帧缓冲区
    ReassemblyFrameBuffer* buffer = frame_buffers_.take(frame_id);
    if (buffer) {
        delete buffer;
    }
}

/**
 * @brief 处理帧数据类型的数据包
 * @param header 协议头
 * @param payload 负载数据
 * @return 错误类型
 */
ReassemblyError PacketReassembly::processFrameData(const UdpHeader* header, const QByteArray& payload)
{
    processFragment(header->frame_id, header->fragment_index, 
                   header->fragment_count, payload);
    return ReassemblyError::None;
}

} // namespace ground_station
