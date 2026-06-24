/**
 * @file videodisplaywidget.h
 * @brief 视频显示控件头文件
 *
 * 提供基于OpenGL的视频渲染和视频显示界面，支持：
 * - 实时视频帧渲染
 * - 全屏显示
 * - 截图功能
 * - 录制控制
 * - 相机信息和统计显示
 */

#ifndef GROUND_STATION_VIDEODISPLAYWIDGET_H
#define GROUND_STATION_VIDEODISPLAYWIDGET_H

#include <QWidget>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QImage>
#include <QMutex>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace ground_station {

/**
 * @class GLVideoRenderer
 * @brief OpenGL视频渲染器
 *
 * 使用OpenGL着色器进行视频帧渲染，支持高效的纹理更新和渲染
 */
class GLVideoRenderer : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit GLVideoRenderer(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~GLVideoRenderer();

    /**
     * @brief 设置视频帧
     * @param frame 要显示的视频帧
     */
    void setVideoFrame(const QImage &frame);
    
    /**
     * @brief 清空帧显示
     */
    void clearFrame();

protected:
    /**
     * @brief 初始化OpenGL
     */
    void initializeGL() override;
    
    /**
     * @brief 窗口大小改变时调整渲染
     * @param w 新宽度
     * @param h 新高度
     */
    void resizeGL(int w, int h) override;
    
    /**
     * @brief 绘制帧
     */
    void paintGL() override;

private:
    /**
     * @brief 初始化着色器程序
     */
    void initShaders();
    
    /**
     * @brief 初始化纹理
     */
    void initTextures();
    
    /**
     * @brief 更新纹理数据
     * @param image 新图像
     */
    void updateTexture(const QImage &image);

private:
    QOpenGLShaderProgram *m_program;    ///< 着色器程序
    QOpenGLTexture *m_texture;          ///< 纹理对象
    QImage m_currentFrame;              ///< 当前帧
    QMutex m_frameMutex;                ///< 帧互斥锁
    bool m_frameUpdated;                ///< 帧更新标志
    int m_textureUniform;               ///< 纹理uniform位置
    int m_vertexAttr;                   ///< 顶点属性位置
    int m_texCoordAttr;                 ///< 纹理坐标属性位置
};

/**
 * @class VideoDisplayWidget
 * @brief 视频显示控件
 *
 * 包含视频渲染区域、工具栏和信息显示，提供完整的视频查看体验
 */
class VideoDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param title 显示标题
     * @param parent 父对象指针
     */
    explicit VideoDisplayWidget(const QString &title, QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~VideoDisplayWidget();

    /**
     * @brief 更新视频帧
     * @param frame 新的视频帧
     */
    void updateFrame(const QImage &frame);
    
    /**
     * @brief 清空显示
     */
    void clearDisplay();
    
    /**
     * @brief 设置标题
     * @param title 标题文本
     */
    void setTitle(const QString &title);
    
    /**
     * @brief 设置相机信息
     * @param cameraId 相机ID
     * @param cameraName 相机名称
     */
    void setCameraInfo(const QString &cameraId, const QString &cameraName);
    
    /**
     * @brief 设置统计信息
     * @param fps 帧率
     * @param bitrate 比特率（bps）
     */
    void setStatistics(int fps, qint64 bitrate);
    
    /**
     * @brief 设置全屏模式
     * @param enabled true=全屏，false=窗口模式
     */
    void setFullscreenEnabled(bool enabled);

signals:
    /**
     * @brief 请求全屏信号
     */
    void fullscreenRequested();
    
    /**
     * @brief 请求截图信号
     */
    void snapshotRequested();
    
    /**
     * @brief 录制状态切换信号
     * @param recording true=正在录制，false=停止录制
     */
    void recordingToggled(bool recording);

private slots:
    /**
     * @brief 全屏按钮点击处理
     */
    void onFullscreenClicked();
    
    /**
     * @brief 截图按钮点击处理
     */
    void onSnapshotClicked();
    
    /**
     * @brief 录制按钮切换处理
     * @param checked 是否选中
     */
    void onRecordingToggled(bool checked);

private:
    /**
     * @brief 设置UI布局
     */
    void setupUi();
    
    /**
     * @brief 设置信号槽连接
     */
    void setupConnections();
    
    /**
     * @brief 更新信息标签
     */
    void updateInfoLabel();

private:
    QVBoxLayout *m_mainLayout;      ///< 主布局
    QHBoxLayout *m_toolbarLayout;   ///< 工具栏布局

    QLabel *m_titleLabel;           ///< 标题标签
    QPushButton *m_fullscreenBtn;   ///< 全屏按钮
    QPushButton *m_snapshotBtn;     ///< 截图按钮
    QPushButton *m_recordBtn;       ///< 录制按钮

    GLVideoRenderer *m_videoRenderer; ///< OpenGL视频渲染器

    QLabel *m_infoLabel;            ///< 信息标签
    QLabel *m_statsLabel;           ///< 统计标签

    QString m_title;                ///< 标题
    QString m_cameraId;             ///< 相机ID
    QString m_cameraName;           ///< 相机名称
    bool m_isRecording;             ///< 是否正在录制
    bool m_isFullscreen;            ///< 是否全屏
};

} // namespace ground_station

#endif // GROUND_STATION_VIDEODISPLAYWIDGET_H