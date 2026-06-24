/**
 * @file videorenderer.h
 * @brief 视频渲染器头文件
 *
 * 提供基于OpenGL的视频帧渲染功能，支持多种渲染模式和帧率统计。
 * 包含单流渲染器和多流渲染器两个类。
 *
 * @author GroundStation Team
 * @date 2024
 */

#ifndef GROUND_STATION_VIDEORENDERER_H
#define GROUND_STATION_VIDEORENDERER_H

#include <QColor>
#include <QElapsedTimer>
#include <QMap>
#include <QMutex>
#include <QOpenGLFunctions_3_0>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QRectF>
#include <QSizeF>
#include <QWidget>

#include "framebuffer.h"
#include "videostats.h"

namespace ground_station {

/**
 * @class VideoRenderer
 * @brief 视频渲染器类
 *
 * 基于OpenGL实现视频帧渲染，支持多种渲染模式、帧率统计和垂直同步。
 */
class VideoRenderer : public QOpenGLWidget, protected QOpenGLFunctions_3_0
{
    Q_OBJECT

public:
    /**
     * @enum RenderMode
     * @brief 渲染模式枚举
     */
    enum class RenderMode {
        Stretch,                      ///< 拉伸填充
        KeepAspectRatio,              ///< 保持宽高比
        KeepAspectRatioByExpanding    ///< 保持宽高比并扩展
    };

    /**
     * @brief 构造函数
     * @param parent 父控件指针
     */
    explicit VideoRenderer(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~VideoRenderer() override;

    /**
     * @brief 设置帧缓冲区
     * @param buffer 帧缓冲区指针
     */
    void setFrameBuffer(const QSharedPointer<FrameBuffer> &buffer);

    /**
     * @brief 设置视频统计对象
     * @param stats 视频统计对象指针
     */
    void setVideoStats(VideoStats *stats);

    /**
     * @brief 设置流ID
     * @param streamId 流ID
     */
    void setStreamId(int streamId);

    /**
     * @brief 设置渲染模式
     * @param mode 渲染模式
     */
    void setRenderMode(RenderMode mode);

    /**
     * @brief 设置背景颜色
     * @param color 背景颜色
     */
    void setBackgroundColor(const QColor &color);

    /**
     * @brief 更新视频帧
     * @param frame 视频帧指针
     */
    void updateFrame(const VideoFramePtr &frame);

    /**
     * @brief 获取当前帧
     * @return 当前帧指针
     */
    VideoFramePtr currentFrame() const;

    /**
     * @brief 获取渲染帧率
     * @return 渲染帧率
     */
    double renderFps() const;

    /**
     * @brief 清除当前帧
     */
    void clearFrame();

    /**
     * @brief 设置垂直同步
     * @param enabled 是否启用
     */
    void setVSyncEnabled(bool enabled);

    /**
     * @brief 获取垂直同步状态
     * @return 是否启用垂直同步
     */
    bool isVSyncEnabled() const;

signals:
    /**
     * @brief 帧渲染完成信号
     * @param frameId 帧ID
     * @param renderTimeMs 渲染时间（毫秒）
     */
    void frameRendered(quint64 frameId, double renderTimeMs);

    /**
     * @brief 渲染错误信号
     * @param error 错误消息
     */
    void renderError(const QString &error);

protected:
    /**
     * @brief 初始化OpenGL
     */
    void initializeGL() override;

    /**
     * @brief 调整大小处理
     * @param width 宽度
     * @param height 高度
     */
    void resizeGL(int width, int height) override;

    /**
     * @brief 绘制处理
     */
    void paintGL() override;

private:
    /**
     * @brief 获取顶点着色器源码
     * @return 着色器源码
     */
    const char *vertexShaderSource();

    /**
     * @brief 获取片段着色器源码
     * @return 着色器源码
     */
    const char *fragmentShaderSource();

    /**
     * @brief 初始化着色器
     * @return 是否成功
     */
    bool initShaders();

    /**
     * @brief 初始化顶点缓冲区
     * @return 是否成功
     */
    bool initVertexBuffer();

    /**
     * @brief 更新纹理
     * @param image 图像数据
     */
    void updateTexture(const cv::Mat &image);

    /**
     * @brief 渲染帧
     */
    void renderFrame();

    /**
     * @brief 计算渲染区域
     * @param frameSize 帧大小
     * @param widgetSize 控件大小
     * @return 渲染区域
     */
    QRectF calculateRenderRect(const QSizeF &frameSize, const QSizeF &widgetSize);

    /**
     * @brief 更新帧率统计
     */
    void updateFpsStats();

    QOpenGLShaderProgram *m_shaderProgram;       // 着色器程序
    GLuint m_textureId;                          // 纹理ID
    GLuint m_vboId;                              // 顶点缓冲区ID
    GLuint m_vaoId;                              // 顶点数组ID

    QSharedPointer<FrameBuffer> m_frameBuffer;   // 帧缓冲区
    VideoFramePtr m_currentFrame;                // 当前帧
    VideoFramePtr m_pendingFrame;                // 待处理帧
    mutable QMutex m_frameMutex;                 // 帧互斥锁

    VideoStats *m_videoStats;                    // 视频统计对象
    int m_streamId;                              // 流ID

    RenderMode m_renderMode;                     // 渲染模式
    QColor m_backgroundColor;                    // 背景颜色
    bool m_vsyncEnabled;                         // 垂直同步标志

    QElapsedTimer m_renderTimer;                 // 渲染计时器
    QElapsedTimer m_fpsTimer;                    // 帧率计时器
    int m_frameCount;                            // 帧计数
    double m_renderFps;                          // 渲染帧率
};

/**
 * @class MultiStreamRenderer
 * @brief 多流渲染器类
 *
 * 管理多个视频流的渲染器，支持动态添加和移除流。
 */
class MultiStreamRenderer : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit MultiStreamRenderer(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~MultiStreamRenderer() override;

    /**
     * @brief 添加流
     * @param streamId 流ID
     * @param parentWidget 父控件
     * @return 渲染器指针
     */
    VideoRenderer *addStream(int streamId, QWidget *parentWidget = nullptr);

    /**
     * @brief 移除流
     * @param streamId 流ID
     */
    void removeStream(int streamId);

    /**
     * @brief 获取渲染器
     * @param streamId 流ID
     * @return 渲染器指针
     */
    VideoRenderer *getRenderer(int streamId) const;

    /**
     * @brief 检查流是否存在
     * @param streamId 流ID
     * @return 是否存在
     */
    bool hasStream(int streamId) const;

    /**
     * @brief 获取所有流ID
     * @return 流ID列表
     */
    QList<int> streamIds() const;

    /**
     * @brief 设置视频统计对象
     * @param stats 视频统计对象
     */
    void setVideoStats(VideoStats *stats);

    /**
     * @brief 设置渲染模式
     * @param mode 渲染模式
     */
    void setRenderMode(VideoRenderer::RenderMode mode);

    /**
     * @brief 清除所有流
     */
    void clearAll();

private:
    mutable QMutex m_mutex;                        // 互斥锁
    QMap<int, VideoRenderer *> m_renderers;        // 渲染器映射
    VideoStats *m_videoStats;                      // 视频统计对象
    VideoRenderer::RenderMode m_renderMode;        // 渲染模式
};

} // namespace ground_station

#endif // GROUND_STATION_VIDEORENDERER_H
