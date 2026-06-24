# Ground Station Client (地面站客户端)

[![Qt](https://img.shields.io/badge/Qt-5.12%2B%20%7C%206.x-green.svg)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![OpenCV](https://img.shields.io/badge/OpenCV-4.x-orange.svg)](https://opencv.org/)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows-lightgrey.svg)](https://www.qt.io/)

地面站遥测数据显示与控制系统客户端应用程序

---

## 目录

- [项目概述](#项目概述)
- [技术架构分析](#技术架构分析)
- [工作流程分析](#工作流程分析)
- [环境配置与部署指南](#环境配置与部署指南)
- [项目扩展指南](#项目扩展指南)
- [代码结构说明](#代码结构说明)
- [常见问题与解决方案](#常见问题与解决方案)
- [未来功能规划建议](#未来功能规划建议)

---

## 项目概述

### 项目背景

地面站客户端是遥测数据系统的核心组件，用于接收、解码和显示来自无人机或其他飞行器的实时视频流和遥测数据。该客户端采用现代化的C++和Qt框架开发，提供稳定可靠的实时数据传输和显示能力。

### 核心功能

| 功能模块 | 描述 |
|---------|------|
| **视频显示** | 支持多路视频流实时显示，采用OpenGL硬件加速渲染 |
| **相机控制** | 远程相机列表管理、流订阅与切换 |
| **数据监控** | 实时统计信息显示，包括帧率、码率、延迟等 |
| **远程命令** | 通过TCP协议发送控制命令，支持心跳保活机制 |

### 核心价值

- **实时性**：采用UDP协议传输视频数据，TCP协议传输控制命令，确保低延迟通信
- **跨平台**：基于Qt框架，支持Linux和Windows桌面环境
- **易扩展**：模块化设计，清晰的接口定义，便于功能扩展和维护

---

## 技术架构分析

### 整体架构图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           Ground Station Client                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                          UI Layer (Qt Widgets)                       │    │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────┐  │    │
│  │  │ MainWindow   │  │CameraList    │  │ VideoDisplayWidget       │  │    │
│  │  │              │  │Widget        │  │ (OpenGL Video Renderer)  │  │    │
│  │  └──────────────┘  └──────────────┘  └──────────────────────────┘  │    │
│  │  ┌──────────────┐                                                    │    │
│  │  │Connection    │                                                    │    │
│  │  │Widget        │                                                    │    │
│  │  └──────────────┘                                                    │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                      │                                       │
│                                      ▼                                       │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                        Core Layer (Business Logic)                   │    │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────┐  │    │
│  │  │ StreamClient │  │StreamManager │  │ StateMachine             │  │    │
│  │  │   (Core)     │──│              │──│ (Client State Machine)   │  │    │
│  │  └──────────────┘  └──────────────┘  └──────────────────────────┘  │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                      │                                       │
│          ┌───────────────────────────┼───────────────────────────┐          │
│          ▼                           ▼                           ▼          │
│  ┌─────────────┐            ┌─────────────┐            ┌─────────────┐      │
│  │   Network   │            │    Video    │            │  Protocol   │      │
│  │   Module    │            │   Module    │            │   Module    │      │
│  ├─────────────┤            ├─────────────┤            ├─────────────┤      │
│  │ TcpHandler  │            │VideoDecoder │            │ProtocolParser│     │
│  │ UdpReceiver │            │VideoRenderer│            │PacketReassembly│   │
│  │StreamClient │            │FrameBuffer  │            │StateMachine │      │
│  │StreamManager│            │VideoStats   │            │             │      │
│  └─────────────┘            └─────────────┘            └─────────────┘      │
│          │                           │                           │          │
│          ▼                           ▼                           ▼          │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                      Infrastructure Layer                            │    │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────┐  │    │
│  │  │ Config       │  │ Logger       │  │ Qt Framework             │  │    │
│  │  │ (Settings)   │  │ (Logging)    │  │ (Core/Network/OpenGL)    │  │    │
│  │  └──────────────┘  └──────────────┘  └──────────────────────────┘  │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
                    ┌───────────────────────────────┐
                    │       Remote Server           │
                    │  ┌─────────┐    ┌─────────┐  │
                    │  │ TCP:8888│    │ UDP:8889│  │
                    │  │ (Command)│   │ (Video) │  │
                    │  └─────────┘    └─────────┘  │
                    └───────────────────────────────┘
```

### 核心技术栈选型及理由

| 技术 | 版本 | 选型理由 |
|------|------|----------|
| **C++** | C++17 | 现代C++特性，支持结构化绑定、optional、filesystem等，提高开发效率和代码安全性 |
| **Qt** | 5.12+ / 6.x | 跨平台GUI框架，提供丰富的网络、多线程、OpenGL支持，信号槽机制便于模块解耦 |
| **OpenCV** | 4.x | 业界标准的计算机视觉库，提供高效的图像编解码能力（JPEG/H.264等） |
| **OpenGL** | 3.0+ | 硬件加速视频渲染，支持YUV到RGB转换，降低CPU占用 |
| **qmake** | - | Qt官方构建工具，与Qt Creator集成良好，简化项目配置 |

### 模块划分与职责说明

#### 1. 网络模块 (network/)

负责客户端与服务端之间的网络通信，包括TCP命令传输和UDP视频数据接收。

| 文件 | 职责 |
|------|------|
| `tcphandler.cpp/h` | TCP命令处理器，实现命令发送、响应接收、心跳保活、自动重连机制 |
| `udpreceiver.cpp/h` | UDP视频数据接收器，处理视频帧分片接收和重组 |
| `streamclient.cpp/h` | 流客户端核心类，整合配置、状态机、流管理器等模块 |
| `streammanager.cpp/h` | 流管理器，管理多路视频流的注册、订阅、切换 |

#### 2. 视频模块 (video/)

负责视频数据的解码、渲染和显示。

| 文件 | 职责 |
|------|------|
| `videodecoder.cpp/h` | 多线程视频解码器，支持JPEG解码，使用OpenCV的imdecode |
| `videorenderer.cpp/h` | OpenGL视频渲染器，支持YUV到RGB转换，多种渲染模式 |
| `framebuffer.cpp/h` | 帧缓冲区管理，线程安全的环形缓冲区实现 |
| `videodisplaywidget.cpp/h` | 视频显示控件，封装VideoRenderer的Qt Widget |
| `videostats.cpp/h` | 视频统计信息收集和计算 |

#### 3. 协议模块 (protocol/)

负责通信协议的解析和客户端状态管理。

| 文件 | 职责 |
|------|------|
| `protocolparser.cpp/h` | 协议解析器，解析命令、响应、相机列表、流列表等 |
| `statemachine.cpp/h` | 客户端状态机，管理状态转换：Disconnected → Connecting → Connected → Registering → Streaming |
| `packetreassembly.cpp/h` | 数据包重组模块，处理UDP分片重组为完整帧 |

#### 4. UI模块 (ui/)

负责用户界面显示和交互。

| 文件 | 职责 |
|------|------|
| `mainwindow.cpp/h` | 主窗口，整合所有UI组件 |
| `connectionwidget.cpp/h` | 连接管理控件，显示连接状态和控制按钮 |
| `cameralistwidget.cpp/h` | 相机列表控件，显示可用相机和流列表 |

#### 5. 基础设施模块

| 文件 | 职责 |
|------|------|
| `config.cpp/h` | 配置管理，支持INI文件读写，单例模式 |
| `logger.cpp/h` | 日志系统，支持分级日志、文件输出、控制台输出 |
| `main.cpp` | 程序入口，初始化应用程序和客户端 |

---

## 工作流程分析

### 核心业务流程图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Client Lifecycle Flow                                │
└─────────────────────────────────────────────────────────────────────────────┘

    ┌──────────┐
    │  Start   │
    └────┬─────┘
         │
         ▼
    ┌──────────────┐
    │ Initialize   │  ──── 加载配置文件、初始化日志系统
    └────┬─────────┘
         │
         ▼
    ┌──────────────┐
    │   Start      │  ──── 启动客户端、创建网络组件
    └────┬─────────┘
         │
         ▼
    ┌──────────────┐     ┌──────────────┐
    │   Connect    │────▶│  Connecting  │
    └──────────────┘     └──────┬───────┘
                                │
                    ┌───────────┴───────────┐
                    │                       │
                    ▼                       ▼
            ┌──────────────┐        ┌──────────────┐
            │  Connected   │        │    Failed    │
            └──────┬───────┘        └──────┬───────┘
                   │                       │
                   ▼                       ▼
            ┌──────────────┐        ┌──────────────┐
            │  Register    │        │   Reconnect  │
            │   Stream     │        │   (Retry)    │
            └──────┬───────┘        └──────────────┘
                   │
                   ▼
            ┌──────────────┐
            │  Streaming   │  ──── 接收视频数据、解码、渲染
            └──────┬───────┘
                   │
        ┌──────────┴──────────┐
        │                     │
        ▼                     ▼
  ┌───────────┐        ┌──────────────┐
  │  Switch   │        │  Heartbeat   │
  │  Stream   │        │  (Keepalive) │
  └───────────┘        └──────────────┘
        │
        ▼
  ┌──────────────┐
  │   Stop/      │
  │  Disconnect  │
  └──────────────┘
```

### 连接建立流程

```
客户端                              服务端
   │                                  │
   │  1. TCP Connect (Port 8888)      │
   │ ────────────────────────────────▶│
   │                                  │
   │  2. TCP Connected                │
   │ ◀────────────────────────────────│
   │                                  │
   │  3. stream_register <name>       │
   │ ────────────────────────────────▶│
   │                                  │
   │  4. stream_register|OK|{data}    │
   │ ◀────────────────────────────────│
   │                                  │
   │  5. UDP Bind (Port 8889)         │
   │ ───────────                      │
   │                                  │
   │  6. stream_subscribe <stream_id> │
   │ ────────────────────────────────▶│
   │                                  │
   │  7. stream_subscribe|OK          │
   │ ◀────────────────────────────────│
   │                                  │
   │  8. UDP Video Data               │
   │ ◀════════════════════════════════│
   │                                  │
```

### 视频数据接收与显示流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      Video Data Pipeline                                     │
└─────────────────────────────────────────────────────────────────────────────┘

  UDP Network          PacketReassembly         VideoDecoder           Display
       │                      │                      │                    │
       │   UDP Packets        │                      │                    │
       │─────────────────────▶│                      │                    │
       │   (Fragments)        │                      │                    │
       │                      │                      │                    │
       │                      │  Reassembled Frame   │                    │
       │                      │─────────────────────▶│                    │
       │                      │   (JPEG/H.264)       │                    │
       │                      │                      │                    │
       │                      │                      │  Decoded Frame     │
       │                      │                      │───────────────────▶│
       │                      │                      │   (RGB/BGR)        │
       │                      │                      │                    │
       │                      │                      │                    │
       ▼                      ▼                      ▼                    ▼

  ┌─────────┐           ┌──────────┐          ┌──────────┐        ┌─────────┐
  │ UDP     │           │ Packet   │          │ Decode   │        │ OpenGL  │
  │ Receive │──────────▶│ Reassembly│─────────▶│ Worker   │───────▶│ Render  │
  │ Thread  │           │ Thread   │          │ Threads  │        │ Thread  │
  └─────────┘           └──────────┘          └──────────┘        └─────────┘
       │                      │                      │                    │
       │                      │                      │                    │
       ▼                      ▼                      ▼                    ▼
  Statistics            Frame Buffer            Frame Buffer         VideoStats
  (packets/sec)         (Ring Buffer)           (Queue)              (FPS, latency)
```

### 命令发送与响应流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      Command-Response Flow                                   │
└─────────────────────────────────────────────────────────────────────────────┘

    Client                                          Server
       │                                               │
       │  ┌─────────────────────────────────────────┐ │
       │  │ 1. Build Command                        │ │
       │  │    ProtocolParser::buildCommand()       │ │
       │  └─────────────────────────────────────────┘ │
       │                                               │
       │  ┌─────────────────────────────────────────┐ │
       │  │ 2. Send Command (TCP)                   │ │
       │  │    TcpHandler::sendCommand()            │ │
       │  └─────────────────────────────────────────┘ │
       │ ─────────────────────────────────────────────▶│
       │                                               │
       │  ┌─────────────────────────────────────────┐ │
       │  │ 3. Wait Response (with timeout)         │ │
       │  │    TcpHandler::sendCommandAndWait()     │ │
       │  └─────────────────────────────────────────┘ │
       │                                               │
       │ ◀─────────────────────────────────────────────│
       │  ┌─────────────────────────────────────────┐ │
       │  │ 4. Parse Response                       │ │
       │  │    ProtocolParser::parseResponse()      │ │
       │  └─────────────────────────────────────────┘ │
       │                                               │
       │  ┌─────────────────────────────────────────┐ │
       │  │ 5. Handle Response                      │ │
       │  │    - Update State                       │ │
       │  │    - Emit Signals                       │ │
       │  │    - Update Statistics                  │ │
       │  └─────────────────────────────────────────┘ │
       │                                               │
```

---

## 环境配置与部署指南

### 开发环境搭建

#### 系统要求

- **操作系统**: Ubuntu 20.04+ / Windows 10+
- **编译器**: GCC 9+ / MSVC 2019+ / Clang 10+
- **内存**: 最低 4GB，推荐 8GB+
- **显卡**: 支持 OpenGL 3.0+

#### Linux 环境配置

```bash
# 1. 安装基础开发工具
sudo apt update
sudo apt install build-essential cmake git

# 2. 安装 Qt 5
sudo apt install qt5-default qtbase5-dev qtbase5-dev-tools
sudo apt install qt5-qmake qtchooser
sudo apt install libqt5opengl5-dev libqt5network5

# 或者安装 Qt 6 (推荐)
# sudo apt install qt6-base-dev qt6-base-dev-tools

# 3. 安装 OpenCV 4
sudo apt install libopencv-dev python3-opencv

# 4. 安装 OpenGL 开发库
sudo apt install libgl1-mesa-dev libglu1-mesa-dev

# 5. 验证安装
qmake --version
pkg-config --modversion opencv4
```

#### Windows 环境配置

1. **安装 Qt**
   - 下载 [Qt Online Installer](https://www.qt.io/download)
   - 选择 Qt 5.15 或 Qt 6.x
   - 选择 MSVC 编译器组件

2. **安装 OpenCV**
   ```powershell
   # 使用 vcpkg 安装
   vcpkg install opencv4:x64-windows
   ```

3. **配置环境变量**
   - 添加 Qt bin 目录到 PATH
   - 添加 OpenCV bin 目录到 PATH

### 编译和运行

#### 使用 qmake 编译

```bash
# 进入项目目录
cd /path/to/ground_station

# 生成 Makefile
qmake ground_station.pro

# 编译 (使用多核加速)
make -j$(nproc)

# 运行
./ground_station_client
```

#### 使用 Qt Creator 编译

1. 打开 Qt Creator
2. 文件 → 打开文件或项目 → 选择 `ground_station.pro`
3. 配置项目（选择编译套件）
4. 点击 "构建" → "构建项目"
5. 点击 "运行" 启动程序

### 配置文件说明

客户端使用 INI 格式的配置文件 `client.ini`：

```ini
[Server]
# 服务器IP地址
address=127.0.0.1
# TCP命令端口
command_port=8888
# UDP数据端口
data_port=8889
# 连接超时（毫秒）
connection_timeout=5000
# 重连间隔（毫秒）
reconnect_interval=3000
# 最大重连次数
max_reconnect_attempts=5

[Buffer]
# 接收缓冲区大小（字节）
receive_buffer_size=1048576
# 发送缓冲区大小（字节）
send_buffer_size=524288
# 帧缓冲区大小（帧数）
frame_buffer_size=30
# 最大队列大小
max_queue_size=100

[Stream]
# 最大流数量
max_stream_count=2
# 流超时时间（毫秒）
stream_timeout=10000
# 心跳间隔（毫秒）
heartbeat_interval=5000
# 注册超时（毫秒）
registration_timeout=5000

[Log]
# 日志文件路径
log_file_path=logs/ground_station.log
# 日志级别 (0=Debug, 1=Info, 2=Warning, 3=Error)
log_level=1
# 是否启用控制台输出
enable_console=true
# 最大日志文件大小（字节）
max_log_file_size=10485760
# 最大日志文件数量
max_log_files=5
```

---

## 项目扩展指南

### 控制协议实现规范

#### 协议格式

**命令格式**：
```
COMMAND_NAME [arg1] [arg2] ...\n
```

**响应格式**：
```
COMMAND_NAME|STATUS|MESSAGE\n
```

或 JSON 格式：
```json
{
  "command": "COMMAND_NAME",
  "status": "OK|ERR",
  "message": "description",
  "data": { ... }
}
```

### 通信方式说明

| 通道 | 端口 | 用途 | 特点 |
|------|------|------|------|
| TCP | 8888 | 命令/控制 | 可靠传输，支持心跳保活 |
| UDP | 8889 | 视频数据 | 低延迟，支持分片传输 |

### 命令集定义

#### 流控制命令

| 命令 | 格式 | 说明 |
|------|------|------|
| `stream_register` | `stream_register <stream_name>` | 注册视频流 |
| `stream_subscribe` | `stream_subscribe <stream_id>` | 订阅视频流 |
| `stream_unsubscribe` | `stream_unsubscribe <stream_id>` | 取消订阅 |
| `stream_switch` | `stream_switch <stream_id>` | 切换视频流 |
| `stream_list` | `stream_list` | 获取流列表 |
| `stream_info` | `stream_info <stream_id>` | 获取流信息 |

#### 心跳命令

| 命令 | 格式 | 说明 |
|------|------|------|
| `ping` | `ping [timestamp]` | 心跳检测 |

#### 持久化控制命令

| 命令 | 格式 | 说明 |
|------|------|------|
| `persist_start` | `persist_start [output_path]` | 开始持久化 |
| `persist_stop` | `persist_stop` | 停止持久化 |
| `persist_pause` | `persist_pause` | 暂停持久化 |
| `persist_resume` | `persist_resume` | 恢复持久化 |
| `persist_status` | `persist_status` | 查询持久化状态 |

#### 配置命令

| 命令 | 格式 | 说明 |
|------|------|------|
| `config_get` | `config_get <key>` | 获取配置项 |
| `config_set` | `config_set <key> <value>` | 设置配置项 |
| `interval` | `interval <ms>` | 设置同步间隔 |
| `threshold` | `threshold <value>` | 设置同步阈值 |

#### 诊断命令

| 命令 | 格式 | 说明 |
|------|------|------|
| `diag_status` | `diag_status` | 系统诊断状态 |
| `log_level` | `log_level <level>` | 设置日志级别 |
| `version` | `version` | 版本查询 |
| `help` | `help [command]` | 帮助信息 |
| `shutdown` | `shutdown` | 系统关闭 |

### 扩展方法

#### 添加新命令

1. **定义命令类型** (在 `protocolparser.h` 中)：

```cpp
enum class CommandType {
    // ... 现有命令 ...
    MyNewCommand,    // 添加新命令
};
```

2. **实现命令解析** (在 `protocolparser.cpp` 中)：

```cpp
CommandType ProtocolParser::stringToCommandType(const QString& name) {
    // ... 现有映射 ...
    if (name == "my_new_command") {
        return CommandType::MyNewCommand;
    }
    return CommandType::Unknown;
}
```

3. **实现命令处理** (在 `tcphandler.cpp` 或 `streamclient.cpp` 中)：

```cpp
bool StreamClient::handleCommand(const QByteArray& data) {
    Command cmd = ProtocolParser::parseCommand(data);
    
    switch (cmd.type) {
        case CommandType::MyNewCommand:
            return handleMyNewCommand(cmd.args);
        // ...
    }
}
```

#### 添加新的视频编码格式

1. **扩展解码器** (在 `videodecoder.cpp` 中)：

```cpp
bool DecodeWorker::decodeFrame(const EncodedFrame& frame) {
    cv::Mat decoded;
    
    // 根据编码格式选择解码方式
    switch (frame.encodeFormat) {
        case EncodeFormat::JPEG:
            decoded = cv::imdecode(frame.data, cv::IMREAD_COLOR);
            break;
        case EncodeFormat::H264:
            // 使用 FFmpeg 或其他解码库
            decoded = decodeH264(frame.data);
            break;
        case EncodeFormat::MyNewFormat:
            // 实现新格式解码
            decoded = decodeMyNewFormat(frame.data);
            break;
    }
    // ...
}
```

### 代码示例

#### 自定义命令发送

```cpp
// 发送自定义命令
void sendCustomCommand(TcpHandler* handler) {
    // 方式1: 使用字符串
    handler->sendCommand("my_command arg1 arg2");
    
    // 方式2: 使用命令类型
    handler->sendCommand(CommandType::MyNewCommand, {"arg1", "arg2"});
    
    // 方式3: 等待响应
    Response response = handler->sendCommandAndWait(
        CommandType::MyNewCommand, 
        {"arg1"}, 
        5000  // 5秒超时
    );
    
    if (response.isSuccess()) {
        qDebug() << "Command succeeded:" << response.message;
    }
}
```

#### 自定义视频处理

```cpp
// 处理接收到的视频帧
void processVideoFrame(UdpReceiver* receiver) {
    QObject::connect(receiver, &UdpReceiver::frameReceived, 
        [](const VideoFrame& frame) {
            // 处理帧数据
            qDebug() << "Frame received:" << frame.frame_id
                     << "Size:" << frame.data.size()
                     << "Timestamp:" << frame.timestamp;
            
            // 可以进行自定义处理
            // 例如: 目标检测、图像增强等
        });
}
```

---

## 代码结构说明

### 目录结构

```
ground_station/
├── ground_station.pro          # qmake 项目文件
├── mainwindow.ui               # Qt Designer UI 文件
│
├── include/                    # 头文件目录
│   ├── config.h               # 配置管理
│   ├── logger.h               # 日志系统
│   │
│   ├── network/               # 网络模块头文件
│   │   ├── tcphandler.h       # TCP命令处理器
│   │   ├── udpreceiver.h      # UDP视频接收器
│   │   ├── streamclient.h     # 流客户端核心
│   │   └── streammanager.h    # 流管理器
│   │
│   ├── video/                 # 视频模块头文件
│   │   ├── videodecoder.h     # 视频解码器
│   │   ├── videorenderer.h    # OpenGL渲染器
│   │   ├── framebuffer.h      # 帧缓冲区
│   │   ├── videodisplaywidget.h # 视频显示控件
│   │   └── videostats.h       # 视频统计
│   │
│   ├── protocol/              # 协议模块头文件
│   │   ├── protocolparser.h   # 协议解析器
│   │   ├── statemachine.h     # 客户端状态机
│   │   └── packetreassembly.h # 数据包重组
│   │
│   └── ui/                    # UI模块头文件
│       ├── mainwindow.h       # 主窗口
│       ├── connectionwidget.h # 连接控件
│       └── cameralistwidget.h # 相机列表控件
│
├── src/                       # 源文件目录
│   ├── main.cpp               # 程序入口
│   ├── config.cpp             # 配置管理实现
│   ├── logger.cpp             # 日志系统实现
│   │
│   ├── network/               # 网络模块源文件
│   │   ├── tcphandler.cpp
│   │   ├── udpreceiver.cpp
│   │   ├── streamclient.cpp
│   │   └── streammanager.cpp
│   │
│   ├── video/                 # 视频模块源文件
│   │   ├── videodecoder.cpp
│   │   ├── videorenderer.cpp
│   │   ├── framebuffer.cpp
│   │   ├── videodisplaywidget.cpp
│   │   └── videostats.cpp
│   │
│   ├── protocol/              # 协议模块源文件
│   │   ├── protocolparser.cpp
│   │   ├── statemachine.cpp
│   │   └── packetreassembly.cpp
│   │
│   └── ui/                    # UI模块源文件
│       ├── mainwindow.cpp
│       ├── connectionwidget.cpp
│       └── cameralistwidget.cpp
│
└── build/                     # 构建输出目录
    ├── moc_*.cpp              # Qt MOC 生成文件
    └── ui_*.h                 # Qt UIC 生成文件
```

### 核心文件功能

| 文件 | 核心功能 | 关键类/函数 |
|------|----------|-------------|
| `main.cpp` | 程序入口，初始化流程 | `Application` 类 |
| `streamclient.cpp` | 客户端核心逻辑 | `StreamClient::initialize()`, `StreamClient::connectToServer()` |
| `tcphandler.cpp` | TCP通信管理 | `TcpHandler::sendCommand()`, `TcpHandler::sendHeartbeat()` |
| `udpreceiver.cpp` | UDP数据接收 | `UdpReceiver::start()`, `UdpReceiver::onReadyRead()` |
| `videodecoder.cpp` | 视频解码 | `VideoDecoder::decode()`, `DecodeWorker::decodeFrame()` |
| `videorenderer.cpp` | OpenGL渲染 | `VideoRenderer::paintGL()`, `VideoRenderer::updateTexture()` |
| `protocolparser.cpp` | 协议解析 | `ProtocolParser::parseCommand()`, `ProtocolParser::parseResponse()` |
| `statemachine.cpp` | 状态管理 | `StateMachine::requestStateTransition()` |

### 关键代码示例

#### 状态机状态转换

```cpp
// statemachine.cpp
bool StateMachine::isValidTransition(ClientState from, ClientState to) const {
    static const QMap<ClientState, QList<ClientState>> validTransitions = {
        {ClientState::Disconnected, {ClientState::Connecting, ClientState::Error}},
        {ClientState::Connecting, {ClientState::Connected, ClientState::Disconnected, ClientState::Error}},
        {ClientState::Connected, {ClientState::Registering, ClientState::Disconnected, ClientState::Error}},
        {ClientState::Registering, {ClientState::Streaming, ClientState::Connected, ClientState::Disconnected, ClientState::Error}},
        {ClientState::Streaming, {ClientState::Connected, ClientState::Disconnected, ClientState::Error}},
        {ClientState::Error, {ClientState::Disconnected}}
    };
    
    return validTransitions.value(from).contains(to);
}
```

#### 帧缓冲区实现

```cpp
// framebuffer.h
class FrameBuffer {
public:
    bool push(const VideoFramePtr& frame) {
        QMutexLocker locker(&m_mutex);
        
        if (m_buffer.size() >= m_maxSize) {
            m_buffer.dequeue();  // 丢弃最旧帧
        }
        m_buffer.enqueue(frame);
        m_condition.wakeAll();
        return true;
    }
    
    VideoFramePtr pop(int timeoutMs = 1000) {
        QMutexLocker locker(&m_mutex);
        
        if (m_buffer.isEmpty()) {
            m_condition.wait(&m_mutex, timeoutMs);
        }
        
        if (m_buffer.isEmpty()) {
            return nullptr;
        }
        
        return m_buffer.dequeue();
    }
    
private:
    QQueue<VideoFramePtr> m_buffer;
    mutable QMutex m_mutex;
    QWaitCondition m_condition;
    int m_maxSize;
};
```

---

## 常见问题与解决方案

### 编译问题

#### Q1: 找不到 OpenCV 头文件

**错误信息**：
```
fatal error: opencv2/opencv.hpp: No such file or directory
```

**解决方案**：
```bash
# 检查 OpenCV 安装路径
pkg-config --cflags opencv4

# 修改 .pro 文件中的路径
INCLUDEPATH += /usr/include/opencv4
```

#### Q2: Qt 版本不兼容

**错误信息**：
```
Project ERROR: Unknown module(s) in QT: opengl
```

**解决方案**：
```bash
# 安装 Qt OpenGL 模块
sudo apt install libqt5opengl5-dev

# 或在 .pro 中添加
QT += opengl
```

#### Q3: 链接错误 - 找不到符号

**错误信息**：
```
undefined reference to `cv::imdecode'
```

**解决方案**：
```bash
# 确保 .pro 中正确链接 OpenCV
LIBS += `pkg-config --libs opencv4`

# 或直接指定库
LIBS += -lopencv_core -lopencv_imgcodecs -lopencv_imgproc
```

### 运行时问题

#### Q4: 无法连接到服务器

**排查步骤**：
1. 检查服务器是否运行
2. 检查防火墙设置
3. 验证服务器地址和端口配置

```bash
# 测试端口连通性
telnet <server_ip> 8888
nc -zv <server_ip> 8888
```

#### Q5: 视频显示黑屏

**可能原因**：
- OpenGL 版本不支持
- 着色器编译失败
- 帧数据为空

**解决方案**：
```cpp
// 检查 OpenGL 支持
QSurfaceFormat format = QSurfaceFormat::defaultFormat();
qDebug() << "OpenGL version:" << format.majorVersion() << "." << format.minorVersion();

// 检查着色器编译
if (!m_shaderProgram->link()) {
    qDebug() << "Shader link error:" << m_shaderProgram->log();
}
```

#### Q6: 内存占用持续增长

**排查方法**：
```cpp
// 检查帧缓冲区大小
qDebug() << "Buffer size:" << m_frameBuffer->size();

// 确保及时释放帧数据
void clearOldFrames() {
    while (m_frameBuffer->size() > MAX_BUFFER_SIZE) {
        m_frameBuffer->pop();
    }
}
```

### 性能优化

#### 优化建议

| 优化项 | 方法 | 预期效果 |
|--------|------|----------|
| 解码性能 | 增加解码线程数 | 提高解码吞吐量 |
| 渲染性能 | 启用 VSync | 减少画面撕裂 |
| 网络性能 | 增大 UDP 缓冲区 | 减少丢包 |
| 内存占用 | 限制帧缓冲区大小 | 防止内存溢出 |

#### 配置优化

```ini
[Buffer]
# 增大接收缓冲区
receive_buffer_size=2097152

[Stream]
# 减少心跳间隔以提高响应速度
heartbeat_interval=3000
```

---

## 未来功能规划

### 短期规划 (1-3个月)

| 功能 | 优先级 | 描述 |
|------|--------|------|
| H.264/H.265 解码支持 | 高 | 集成 FFmpeg，支持更多编码格式 |
| 录制功能 | 高 | 支持视频流本地录制 |
| 多窗口布局 | 中 | 支持自定义视频布局（1x1, 2x2, 1+3等） |
| OSD 叠加 | 中 | 在视频上叠加时间戳、帧率等信息 |

### 中期规划 (3-6个月)

| 功能 | 优先级 | 描述 |
|------|--------|------|
| 遥测数据显示 | 高 | 显示飞行器姿态、位置、速度等数据 |
| 电子地图集成 | 中 | 集成地图显示飞行轨迹 |
| 云台控制 | 中 | 支持相机云台俯仰、偏航控制 |
| 音频支持 | 低 | 支持音频流接收和播放 |

### 长期规划 (6-12个月)

| 功能 | 优先级 | 描述 |
|------|--------|------|
| AI 目标检测 | 中 | 集成目标检测算法，自动标记目标 |
| 多语言支持 | 中 | 支持中英文等多语言界面 |
| 移动端支持 | 低 | 开发 Android/iOS 客户端 |
| Web 客户端 | 低 | 开发基于 WebRTC 的 Web 客户端 |

### 架构改进建议

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      Future Architecture Improvements                        │
└─────────────────────────────────────────────────────────────────────────────┘

1. 插件化架构
   - 视频解码器插件（支持动态加载不同解码器）
   - 协议插件（支持自定义通信协议）
   - UI 主题插件

2. 微服务化
   - 视频处理服务（独立进程）
   - 通信服务（独立进程）
   - 主程序作为协调器

3. 性能监控
   - 集成 Prometheus metrics
   - 实时性能仪表盘
   - 自动性能调优

4. 安全增强
   - TLS/SSL 加密通信
   - 身份认证
   - 权限管理
```

---

## 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

---

## 联系方式

- **项目主页**: https://github.com/alientek/ground_station_client
- **问题反馈**: https://github.com/alientek/ground_station_client/issues
- **邮箱**: support@alientek.com

---

*文档版本: 1.0.0 | 最后更新: 2024*
