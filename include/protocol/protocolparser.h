/**
 * @file protocolparser.h
 * @brief 协议解析器头文件
 *
 * 提供地面站通信协议的解析功能，包括：
 * - 命令解析
 * - 响应解析
 * - 相机列表解析
 * - 注册响应解析
 * - 状态报告解析
 */

#ifndef GROUND_STATION_PROTOCOLPARSER_H
#define GROUND_STATION_PROTOCOLPARSER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <cstdint>

namespace ground_station {

// ============================================================================
// 常量定义
// ============================================================================

/// 命令分隔符
constexpr char CMD_SEPARATOR = ' ';

/// 命令结束符
constexpr char CMD_TERMINATOR = '\n';

/// 响应分隔符
constexpr char RESPONSE_SEPARATOR = '|';

/// 最大命令长度
constexpr std::size_t MAX_CMD_LENGTH = 1024;

// ============================================================================
// 命令类型定义
// ============================================================================

/**
 * @enum CommandType
 * @brief 命令类型枚举
 */
enum class CommandType {
    Unknown = 0,
    
    // 流控制命令
    StreamRegister,     ///< stream_register - 注册流
    StreamSubscribe,    ///< stream_subscribe - 订阅流
    StreamUnsubscribe,  ///< stream_unsubscribe - 取消订阅
    StreamSwitch,       ///< stream_switch - 切换流
    StreamList,         ///< stream_list - 获取流列表
    StreamInfo,         ///< stream_info - 获取流信息
    
    // 心跳命令
    Ping,               ///< ping - 心跳检测
    
    // 持久化控制命令
    PersistStart,       ///< persist_start - 开始持久化
    PersistStop,        ///< persist_stop - 停止持久化
    PersistPause,       ///< persist_pause - 暂停持久化
    PersistResume,      ///< persist_resume - 恢复持久化
    PersistStatus,      ///< persist_status - 持久化状态查询
    PersistStats,       ///< persist_stats - 持久化统计
    
    // 配置命令
    ConfigGet,          ///< config_get - 获取配置
    ConfigSet,          ///< config_set - 设置配置
    Interval,           ///< interval - 设置同步间隔
    Threshold,          ///< threshold - 设置同步阈值
    
    // 诊断命令
    DiagStatus,         ///< diag_status - 系统诊断状态
    LogLevel,           ///< log_level - 设置日志级别
    Version,            ///< version - 版本查询
    Help,               ///< help - 帮助信息
    Shutdown            ///< shutdown - 系统关闭
};

/**
 * @enum ResponseStatus
 * @brief 响应状态枚举
 */
enum class ResponseStatus {
    Success = 0,    ///< 成功 (OK)
    Error = 1,      ///< 错误 (ERR)
    Unknown = 2     ///< 未知
};

// ============================================================================
// 结构体定义
// ============================================================================

/**
 * @struct Command
 * @brief 命令结构
 */
struct Command {
    CommandType type = CommandType::Unknown;    ///< 命令类型
    QString name;                               ///< 命令名称
    QStringList args;                           ///< 命令参数列表
    QString raw_data;                           ///< 原始数据
    
    /**
     * @brief 检查命令是否有效
     * @return true 有效；false 无效
     */
    bool isValid() const {
        return type != CommandType::Unknown && !name.isEmpty();
    }
};

/**
 * @struct Response
 * @brief 响应结构
 */
struct Response {
    QString command_name;                       ///< 命令名称
    ResponseStatus status = ResponseStatus::Unknown; ///< 响应状态
    QString message;                            ///< 响应消息
    QJsonObject data;                           ///< JSON数据
    QByteArray binary_data;                     ///< 二进制数据
    QString raw_data;                           ///< 原始数据
    
    /**
     * @brief 检查是否成功
     * @return true 成功；false 失败
     */
    bool isSuccess() const {
        return status == ResponseStatus::Success;
    }
    
    bool isValid() const {
        return status != ResponseStatus::Unknown;
    }
};

/**
 * @struct CameraInfo
 * @brief 相机信息结构
 */
struct CameraInfo {
    QString camera_id;          ///< 相机ID
    QString device_path;        ///< 设备路径
    QString card_name;          ///< 设备名称
    QString driver;             ///< 驱动名称
    QString bus_info;           ///< 总线信息
    bool is_available = false;  ///< 是否可用
    bool is_capture = false;    ///< 是否为采集设备
    int index = 0;              ///< 设备索引
    
    /**
     * @brief 从JSON对象解析
     * @param json JSON对象
     * @return true 成功；false 失败
     */
    bool fromJson(const QJsonObject& json);
    
    /**
     * @brief 转换为JSON对象
     * @return JSON对象
     */
    QJsonObject toJson() const;
};

/**
 * @struct StreamInfo
 * @brief 流信息结构
 */
struct StreamInfo {
    QString stream_id;          ///< 流ID
    QString camera_id;          ///< 相机ID
    QString state;              ///< 流状态
    int fps = 0;                ///< 帧率
    int width = 0;              ///< 图像宽度
    int height = 0;             ///< 图像高度
    QString encode_format;      ///< 编码格式
    int encode_quality = 0;     ///< 编码质量
    int client_count = 0;       ///< 客户端数量
    std::uint64_t frames_sent = 0; ///< 已发送帧数
    std::uint64_t bytes_sent = 0;  ///< 已发送字节数
    
    /**
     * @brief 从JSON对象解析
     * @param json JSON对象
     * @return true 成功；false 失败
     */
    bool fromJson(const QJsonObject& json);
    
    /**
     * @brief 转换为JSON对象
     * @return JSON对象
     */
    QJsonObject toJson() const;
};

/**
 * @struct RegisterResponse
 * @brief 注册响应结构
 */
struct RegisterResponse {
    bool success = false;       ///< 是否成功
    QString client_id;          ///< 客户端ID
    QString message;            ///< 响应消息
    QStringList available_streams; ///< 可用流列表
    
    /**
     * @brief 从响应解析
     * @param response 响应数据
     * @return true 成功；false 失败
     */
    bool fromResponse(const Response& response);
};

/**
 * @struct SystemStatus
 * @brief 系统状态结构
 */
struct SystemStatus {
    QString state;              ///< 系统状态
    int total_streams = 0;      ///< 总流数
    int active_streams = 0;     ///< 活动流数
    int total_clients = 0;      ///< 总客户端数
    std::uint64_t total_frames = 0; ///< 总帧数
    std::uint64_t total_bytes = 0;  ///< 总字节数
    double avg_latency_ms = 0.0;    ///< 平均延迟
    double max_latency_ms = 0.0;    ///< 最大延迟
    int uptime_seconds = 0;         ///< 运行时间（秒）
    
    /**
     * @brief 从JSON对象解析
     * @param json JSON对象
     * @return true 成功；false 失败
     */
    bool fromJson(const QJsonObject& json);
    
    /**
     * @brief 转换为JSON对象
     * @return JSON对象
     */
    QJsonObject toJson() const;
};

/**
 * @struct PersistStatus
 * @brief 持久化状态结构
 */
struct PersistStatus {
    bool is_running = false;    ///< 是否运行中
    bool is_paused = false;     ///< 是否暂停
    QString output_path;        ///< 输出路径
    std::uint64_t saved_frames = 0; ///< 已保存帧数
    std::uint64_t saved_bytes = 0;  ///< 已保存字节数
    double write_rate = 0.0;        ///< 写入速率（MB/s）
    QString error_message;          ///< 错误消息
    
    /**
     * @brief 从JSON对象解析
     * @param json JSON对象
     * @return true 成功；false 失败
     */
    bool fromJson(const QJsonObject& json);
};

// ============================================================================
// ProtocolParser 类
// ============================================================================

/**
 * @class ProtocolParser
 * @brief 协议解析器类
 *
 * 提供地面站通信协议的解析功能，支持：
 * - 命令解析：CMD_NAME [arg1] [arg2] ...\n
 * - 响应解析：CMD_NAME|RESULT|MESSAGE\n
 * - JSON数据解析
 *
 * 使用方式：
 * 1. 调用 parseCommand() 解析命令
 * 2. 调用 parseResponse() 解析响应
 * 3. 调用 parseCameraList() 解析相机列表
 * 4. 调用 parseStreamList() 解析流列表
 */
class ProtocolParser : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit ProtocolParser(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ProtocolParser() override = default;

    // ========================================================================
    // 命令解析
    // ========================================================================

    /**
     * @brief 解析命令
     * @param data 原始数据
     * @return 解析后的命令结构
     */
    static Command parseCommand(const QByteArray& data);

    /**
     * @brief 解析命令
     * @param data 原始数据字符串
     * @return 解析后的命令结构
     */
    static Command parseCommand(const QString& data);

    /**
     * @brief 将命令类型转换为字符串
     * @param type 命令类型
     * @return 命令字符串
     */
    static QString commandTypeToString(CommandType type);

    /**
     * @brief 将字符串转换为命令类型
     * @param name 命令字符串
     * @return 命令类型
     */
    static CommandType stringToCommandType(const QString& name);

    /**
     * @brief 构建命令字符串
     * @param type 命令类型
     * @param args 参数列表
     * @return 命令字符串
     */
    static QString buildCommand(CommandType type, const QStringList& args = QStringList());

    // ========================================================================
    // 响应解析
    // ========================================================================

    /**
     * @brief 解析响应
     * @param data 原始数据
     * @return 解析后的响应结构
     */
    static Response parseResponse(const QByteArray& data);

    /**
     * @brief 解析响应
     * @param data 原始数据字符串
     * @return 解析后的响应结构
     */
    static Response parseResponse(const QString& data);

    /**
     * @brief 解析JSON响应
     * @param data JSON数据
     * @return 解析后的响应结构
     */
    static Response parseJsonResponse(const QByteArray& data);

    /**
     * @brief 构建响应字符串
     * @param command_name 命令名称
     * @param success 是否成功
     * @param message 响应消息
     * @return 响应字符串
     */
    static QString buildResponse(const QString& command_name, bool success, const QString& message);

    /**
     * @brief 构建JSON响应
     * @param command_name 命令名称
     * @param success 是否成功
     * @param message 响应消息
     * @param data JSON数据
     * @return JSON响应字节数组
     */
    static QByteArray buildJsonResponse(const QString& command_name, bool success,
                                         const QString& message, const QJsonObject& data = QJsonObject());

    // ========================================================================
    // 特定数据解析
    // ========================================================================

    /**
     * @brief 解析相机列表
     * @param data JSON数据
     * @return 相机信息列表
     */
    static QList<CameraInfo> parseCameraList(const QByteArray& data);

    /**
     * @brief 解析相机列表
     * @param json JSON对象
     * @return 相机信息列表
     */
    static QList<CameraInfo> parseCameraList(const QJsonObject& json);

    /**
     * @brief 解析流列表
     * @param data JSON数据
     * @return 流信息列表
     */
    static QList<StreamInfo> parseStreamList(const QByteArray& data);

    /**
     * @brief 解析流列表
     * @param json JSON对象
     * @return 流信息列表
     */
    static QList<StreamInfo> parseStreamList(const QJsonObject& json);

    /**
     * @brief 解析注册响应
     * @param response 响应数据
     * @return 注册响应结构
     */
    static RegisterResponse parseRegisterResponse(const Response& response);

    /**
     * @brief 解析系统状态
     * @param data JSON数据
     * @return 系统状态结构
     */
    static SystemStatus parseSystemStatus(const QByteArray& data);

    /**
     * @brief 解析持久化状态
     * @param data JSON数据
     * @return 持久化状态结构
     */
    static PersistStatus parsePersistStatus(const QByteArray& data);

    // ========================================================================
    // 数据完整性检查
    // ========================================================================

    /**
     * @brief 查找消息结束位置
     * @param data 数据
     * @return 结束位置，-1表示不完整
     */
    static int findMessageEnd(const QByteArray& data);

    /**
     * @brief 检查数据是否包含完整消息
     * @param data 数据
     * @return true 完整；false 不完整
     */
    static bool hasCompleteMessage(const QByteArray& data);

    /**
     * @brief 提取完整消息
     * @param data 数据缓冲区
     * @param message 输出消息
     * @return 提取的字节数，0表示无完整消息
     */
    static int extractMessage(QByteArray& data, QByteArray& message);

    // ========================================================================
    // 辅助方法
    // ========================================================================

    /**
     * @brief 分割命令行
     * @param line 命令行
     * @return 分割后的字符串列表
     */
    static QStringList splitCommandLine(const QString& line);

    /**
     * @brief 去除命令行空白
     * @param line 命令行
     * @return 处理后的命令行
     */
    static QString trimCommandLine(const QString& line);

    /**
     * @brief 解析JSON文档
     * @param data JSON数据
     * @return JSON文档，解析失败返回空文档
     */
    static QJsonDocument parseJsonDocument(const QByteArray& data);

private:
    /**
     * @brief 解析响应状态字符串
     * @param status_str 状态字符串
     * @return 响应状态
     */
    static ResponseStatus parseResponseStatus(const QString& status_str);
};

} // namespace ground_station

#endif // GROUND_STATION_PROTOCOLPARSER_H