/**
 * @file protocolparser.cpp
 * @brief 协议解析器实现
 *
 * 实现地面站与服务端之间的通信协议解析，包括：
 * - 命令解析（文本协议）
 * - 响应解析（文本协议和JSON协议）
 * - 数据结构序列化/反序列化（CameraInfo, StreamInfo等）
 * - 消息提取和缓冲区管理
 *
 * 协议格式：
 * 命令格式: CMD_NAME [arg1] [arg2] ...\n
 * 响应格式: CMD_NAME|STATUS|MESSAGE[|JSON_DATA]\n
 * JSON响应格式: {"command": "...", "success": true/false, "message": "...", "data": {...}}\n
 */

#include "protocolparser.h"

#include <QDebug>
#include <QRegularExpression>

namespace ground_station {

// ============================================================================
// CameraInfo 实现
// ============================================================================

/**
 * @brief 从JSON对象解析相机信息
 * @param json JSON对象
 * @return 是否解析成功
 */
bool CameraInfo::fromJson(const QJsonObject& json)
{
    camera_id = json["camera_id"].toString();
    device_path = json["device_path"].toString();
    card_name = json["card_name"].toString();
    driver = json["driver"].toString();
    bus_info = json["bus_info"].toString();
    is_available = json["is_available"].toBool();
    is_capture = json["is_capture"].toBool();
    index = json["index"].toInt();
    return !camera_id.isEmpty();
}

/**
 * @brief 将相机信息转换为JSON对象
 * @return JSON对象
 */
QJsonObject CameraInfo::toJson() const
{
    QJsonObject json;
    json["camera_id"] = camera_id;
    json["device_path"] = device_path;
    json["card_name"] = card_name;
    json["driver"] = driver;
    json["bus_info"] = bus_info;
    json["is_available"] = is_available;
    json["is_capture"] = is_capture;
    json["index"] = index;
    return json;
}

// ============================================================================
// StreamInfo 实现
// ============================================================================

/**
 * @brief 从JSON对象解析流信息
 * @param json JSON对象
 * @return 是否解析成功
 */
bool StreamInfo::fromJson(const QJsonObject& json)
{
    stream_id = json["stream_id"].toString();
    camera_id = json["camera_id"].toString();
    state = json["state"].toString();
    fps = json["fps"].toInt();
    width = json["width"].toInt();
    height = json["height"].toInt();
    encode_format = json["encode_format"].toString();
    encode_quality = json["encode_quality"].toInt();
    client_count = json["client_count"].toInt();
    frames_sent = json["frames_sent"].toVariant().toULongLong();
    bytes_sent = json["bytes_sent"].toVariant().toULongLong();
    return !stream_id.isEmpty();
}

/**
 * @brief 将流信息转换为JSON对象
 * @return JSON对象
 */
QJsonObject StreamInfo::toJson() const
{
    QJsonObject json;
    json["stream_id"] = stream_id;
    json["camera_id"] = camera_id;
    json["state"] = state;
    json["fps"] = fps;
    json["width"] = width;
    json["height"] = height;
    json["encode_format"] = encode_format;
    json["encode_quality"] = encode_quality;
    json["client_count"] = client_count;
    json["frames_sent"] = static_cast<qint64>(frames_sent);
    json["bytes_sent"] = static_cast<qint64>(bytes_sent);
    return json;
}

// ============================================================================
// RegisterResponse 实现
// ============================================================================

/**
 * @brief 从响应对象解析注册响应
 * @param response 响应对象
 * @return 是否解析成功
 */
bool RegisterResponse::fromResponse(const Response& response)
{
    success = response.isSuccess();
    message = response.message;
    
    if (response.data.contains("client_id")) {
        client_id = response.data["client_id"].toString();
    }
    
    if (response.data.contains("available_streams")) {
        QJsonArray streams = response.data["available_streams"].toArray();
        for (const auto& stream : streams) {
            available_streams.append(stream.toString());
        }
    }
    
    return success;
}

// ============================================================================
// SystemStatus 实现
// ============================================================================

bool SystemStatus::fromJson(const QJsonObject& json)
{
    state = json["state"].toString();
    total_streams = json["total_streams"].toInt();
    active_streams = json["active_streams"].toInt();
    total_clients = json["total_clients"].toInt();
    total_frames = json["total_frames"].toVariant().toULongLong();
    total_bytes = json["total_bytes"].toVariant().toULongLong();
    avg_latency_ms = json["avg_latency_ms"].toDouble();
    max_latency_ms = json["max_latency_ms"].toDouble();
    uptime_seconds = json["uptime_seconds"].toInt();
    return !state.isEmpty();
}

QJsonObject SystemStatus::toJson() const
{
    QJsonObject json;
    json["state"] = state;
    json["total_streams"] = total_streams;
    json["active_streams"] = active_streams;
    json["total_clients"] = total_clients;
    json["total_frames"] = static_cast<qint64>(total_frames);
    json["total_bytes"] = static_cast<qint64>(total_bytes);
    json["avg_latency_ms"] = avg_latency_ms;
    json["max_latency_ms"] = max_latency_ms;
    json["uptime_seconds"] = uptime_seconds;
    return json;
}

// ============================================================================
// PersistStatus 实现
// ============================================================================

bool PersistStatus::fromJson(const QJsonObject& json)
{
    is_running = json["is_running"].toBool();
    is_paused = json["is_paused"].toBool();
    output_path = json["output_path"].toString();
    saved_frames = json["saved_frames"].toVariant().toULongLong();
    saved_bytes = json["saved_bytes"].toVariant().toULongLong();
    write_rate = json["write_rate"].toDouble();
    error_message = json["error_message"].toString();
    return true;
}

// ============================================================================
// ProtocolParser 实现
// ============================================================================

ProtocolParser::ProtocolParser(QObject* parent)
    : QObject(parent)
{
}

Command ProtocolParser::parseCommand(const QByteArray& data)
{
    return parseCommand(QString::fromUtf8(data));
}

Command ProtocolParser::parseCommand(const QString& data)
{
    Command cmd;
    cmd.raw_data = data;

    // 去除空白
    QString line = trimCommandLine(data);
    
    if (line.isEmpty()) {
        return cmd;
    }

    // 分割命令行
    QStringList parts = splitCommandLine(line);
    
    if (parts.isEmpty()) {
        return cmd;
    }

    // 第一个部分是命令名称
    cmd.name = parts[0].toLower();
    cmd.type = stringToCommandType(cmd.name);

    // 剩余部分是参数
    for (int i = 1; i < parts.size(); ++i) {
        cmd.args.append(parts[i]);
    }

    return cmd;
}

QString ProtocolParser::commandTypeToString(CommandType type)
{
    switch (type) {
        case CommandType::StreamRegister:    return "stream_register";
        case CommandType::StreamSubscribe:   return "stream_subscribe";
        case CommandType::StreamUnsubscribe: return "stream_unsubscribe";
        case CommandType::StreamSwitch:      return "stream_switch";
        case CommandType::StreamList:        return "stream_list";
        case CommandType::StreamInfo:        return "stream_info";
        case CommandType::Ping:              return "ping";
        case CommandType::PersistStart:      return "persist_start";
        case CommandType::PersistStop:       return "persist_stop";
        case CommandType::PersistPause:      return "persist_pause";
        case CommandType::PersistResume:     return "persist_resume";
        case CommandType::PersistStatus:     return "persist_status";
        case CommandType::PersistStats:      return "persist_stats";
        case CommandType::ConfigGet:         return "config_get";
        case CommandType::ConfigSet:         return "config_set";
        case CommandType::Interval:          return "interval";
        case CommandType::Threshold:         return "threshold";
        case CommandType::DiagStatus:        return "diag_status";
        case CommandType::LogLevel:          return "log_level";
        case CommandType::Version:           return "version";
        case CommandType::Help:              return "help";
        case CommandType::Shutdown:          return "shutdown";
        default:                             return "unknown";
    }
}

CommandType ProtocolParser::stringToCommandType(const QString& name)
{
    QString lower = name.toLower();
    
    if (lower == "stream_register")    return CommandType::StreamRegister;
    if (lower == "stream_subscribe")   return CommandType::StreamSubscribe;
    if (lower == "stream_unsubscribe") return CommandType::StreamUnsubscribe;
    if (lower == "stream_switch")      return CommandType::StreamSwitch;
    if (lower == "stream_list")        return CommandType::StreamList;
    if (lower == "stream_info")        return CommandType::StreamInfo;
    if (lower == "ping")               return CommandType::Ping;
    if (lower == "persist_start")      return CommandType::PersistStart;
    if (lower == "persist_stop")       return CommandType::PersistStop;
    if (lower == "persist_pause")      return CommandType::PersistPause;
    if (lower == "persist_resume")     return CommandType::PersistResume;
    if (lower == "persist_status")     return CommandType::PersistStatus;
    if (lower == "persist_stats")      return CommandType::PersistStats;
    if (lower == "config_get")         return CommandType::ConfigGet;
    if (lower == "config_set")         return CommandType::ConfigSet;
    if (lower == "interval")           return CommandType::Interval;
    if (lower == "threshold")          return CommandType::Threshold;
    if (lower == "diag_status")        return CommandType::DiagStatus;
    if (lower == "log_level")          return CommandType::LogLevel;
    if (lower == "version")            return CommandType::Version;
    if (lower == "help")               return CommandType::Help;
    if (lower == "shutdown")           return CommandType::Shutdown;
    
    return CommandType::Unknown;
}

QString ProtocolParser::buildCommand(CommandType type, const QStringList& args)
{
    QString cmd = commandTypeToString(type);
    
    if (!args.isEmpty()) {
        for (const auto& arg : args) {
            cmd += " " + arg;
        }
    }
    
    cmd += "\n";
    return cmd;
}

Response ProtocolParser::parseResponse(const QByteArray& data)
{
    return parseResponse(QString::fromUtf8(data));
}

Response ProtocolParser::parseResponse(const QString& data)
{
    Response resp;
    resp.raw_data = data;

    // 去除空白
    QString line = trimCommandLine(data);
    
    if (line.isEmpty()) {
        return resp;
    }

    // 分割响应：CMD_NAME|RESULT|MESSAGE
    QStringList parts = line.split(RESPONSE_SEPARATOR);
    
    if (parts.size() < 3) {
        return resp;
    }

    resp.command_name = parts[0];
    resp.status = parseResponseStatus(parts[1]);
    resp.message = parts[2];

    // 如果有更多部分，可能是JSON数据
    if (parts.size() > 3) {
        QString json_str = parts[3];
        QJsonDocument doc = QJsonDocument::fromJson(json_str.toUtf8());
        if (!doc.isNull() && doc.isObject()) {
            resp.data = doc.object();
        }
    }

    return resp;
}

Response ProtocolParser::parseJsonResponse(const QByteArray& data)
{
    Response resp;
    resp.raw_data = QString::fromUtf8(data);

    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull() || !doc.isObject()) {
        return resp;
    }

    QJsonObject json = doc.object();
    
    resp.command_name = json["command"].toString();
    resp.status = json["success"].toBool() ? ResponseStatus::Success : ResponseStatus::Error;
    resp.message = json["message"].toString();
    
    if (json.contains("data")) {
        resp.data = json["data"].toObject();
    }

    return resp;
}

QString ProtocolParser::buildResponse(const QString& command_name, bool success, const QString& message)
{
    QString status = success ? "OK" : "ERR";
    return QString("%1|%2|%3\n").arg(command_name, status, message);
}

QByteArray ProtocolParser::buildJsonResponse(const QString& command_name, bool success,
                                               const QString& message, const QJsonObject& data)
{
    QJsonObject json;
    json["command"] = command_name;
    json["success"] = success;
    json["message"] = message;
    
    if (!data.isEmpty()) {
        json["data"] = data;
    }
    
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

QList<CameraInfo> ProtocolParser::parseCameraList(const QByteArray& data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull()) {
        return QList<CameraInfo>();
    }
    
    if (doc.isObject()) {
        return parseCameraList(doc.object());
    }
    
    // 如果是数组
    if (doc.isArray()) {
        QList<CameraInfo> cameras;
        QJsonArray array = doc.array();
        for (const auto& item : array) {
            CameraInfo info;
            if (info.fromJson(item.toObject())) {
                cameras.append(info);
            }
        }
        return cameras;
    }
    
    return QList<CameraInfo>();
}

QList<CameraInfo> ProtocolParser::parseCameraList(const QJsonObject& json)
{
    QList<CameraInfo> cameras;
    
    if (json.contains("cameras")) {
        QJsonArray array = json["cameras"].toArray();
        for (const auto& item : array) {
            CameraInfo info;
            if (info.fromJson(item.toObject())) {
                cameras.append(info);
            }
        }
    }
    
    return cameras;
}

QList<StreamInfo> ProtocolParser::parseStreamList(const QByteArray& data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull()) {
        return QList<StreamInfo>();
    }
    
    if (doc.isObject()) {
        return parseStreamList(doc.object());
    }
    
    // 如果是数组
    if (doc.isArray()) {
        QList<StreamInfo> streams;
        QJsonArray array = doc.array();
        for (const auto& item : array) {
            StreamInfo info;
            if (info.fromJson(item.toObject())) {
                streams.append(info);
            }
        }
        return streams;
    }
    
    return QList<StreamInfo>();
}

QList<StreamInfo> ProtocolParser::parseStreamList(const QJsonObject& json)
{
    QList<StreamInfo> streams;
    
    if (json.contains("streams")) {
        QJsonArray array = json["streams"].toArray();
        for (const auto& item : array) {
            StreamInfo info;
            if (info.fromJson(item.toObject())) {
                streams.append(info);
            }
        }
    }
    
    return streams;
}

RegisterResponse ProtocolParser::parseRegisterResponse(const Response& response)
{
    RegisterResponse reg_resp;
    reg_resp.fromResponse(response);
    return reg_resp;
}

SystemStatus ProtocolParser::parseSystemStatus(const QByteArray& data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull() || !doc.isObject()) {
        return SystemStatus();
    }
    
    SystemStatus status;
    status.fromJson(doc.object());
    return status;
}

PersistStatus ProtocolParser::parsePersistStatus(const QByteArray& data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull() || !doc.isObject()) {
        return PersistStatus();
    }
    
    PersistStatus status;
    status.fromJson(doc.object());
    return status;
}

int ProtocolParser::findMessageEnd(const QByteArray& data)
{
    int pos = data.indexOf(CMD_TERMINATOR);
    return pos;
}

bool ProtocolParser::hasCompleteMessage(const QByteArray& data)
{
    return data.contains(CMD_TERMINATOR);
}

int ProtocolParser::extractMessage(QByteArray& data, QByteArray& message)
{
    int pos = findMessageEnd(data);
    
    if (pos < 0) {
        return 0;
    }
    
    // 提取消息（包含终止符）
    message = data.left(pos + 1);
    
    // 从缓冲区移除已提取的消息
    data.remove(0, pos + 1);
    
    return message.size();
}

QStringList ProtocolParser::splitCommandLine(const QString& line)
{
    // 使用正则表达式分割，支持引号内的参数
    QStringList parts;
    
    QRegularExpression regex("\"([^\"]*)\"|'([^']*)'|([^\\s]+)");
    QRegularExpressionMatchIterator it = regex.globalMatch(line);
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        
        // 优先使用引号内的内容
        QString cap1 = match.captured(1);
        QString cap2 = match.captured(2);
        QString cap3 = match.captured(3);
        if (!cap1.isEmpty()) {
            parts.append(cap1);
        } else if (!cap2.isEmpty()) {
            parts.append(cap2);
        } else if (!cap3.isEmpty()) {
            parts.append(cap3);
        }
    }
    
    return parts;
}

QString ProtocolParser::trimCommandLine(const QString& line)
{
    // 去除首尾空白和终止符
    QString trimmed = line.trimmed();
    
    if (trimmed.endsWith(CMD_TERMINATOR)) {
        trimmed.chop(1);
    }
    
    return trimmed;
}

QJsonDocument ProtocolParser::parseJsonDocument(const QByteArray& data)
{
    return QJsonDocument::fromJson(data);
}

ResponseStatus ProtocolParser::parseResponseStatus(const QString& status_str)
{
    QString upper = status_str.toUpper().trimmed();
    
    if (upper == "OK" || upper == "SUCCESS") {
        return ResponseStatus::Success;
    }
    
    if (upper == "ERR" || upper == "ERROR") {
        return ResponseStatus::Error;
    }
    
    return ResponseStatus::Unknown;
}

} // namespace ground_station