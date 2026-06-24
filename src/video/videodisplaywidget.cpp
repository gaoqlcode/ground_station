/**
 * @file videodisplaywidget.cpp
 * @brief 视频显示控件实现
 *
 * 本文件实现了GLVideoRenderer和VideoDisplayWidget类，用于视频的显示和渲染。
 * 主要功能包括：
 * - OpenGL视频渲染
 * - 视频帧的格式转换和显示
 * - 全屏、截图、录制等交互功能
 * - 统计信息显示
 *
 * @author GroundStation Team
 * @version 1.0
 * @date 2024
 */

#include "videodisplaywidget.h"

#include <QApplication>
#include <QScreen>

namespace ground_station {

// ==================== GLVideoRenderer 实现 ====================

GLVideoRenderer::GLVideoRenderer(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_program(nullptr)
    , m_texture(nullptr)
    , m_frameUpdated(false)
    , m_textureUniform(-1)
    , m_vertexAttr(-1)
    , m_texCoordAttr(-1)
{
    setMinimumSize(320, 240);
}

GLVideoRenderer::~GLVideoRenderer()
{
    makeCurrent();
    if (m_texture) {
        delete m_texture;
        m_texture = nullptr;
    }
    if (m_program) {
        delete m_program;
        m_program = nullptr;
    }
    doneCurrent();
}

void GLVideoRenderer::setVideoFrame(const QImage &frame)
{
    QMutexLocker locker(&m_frameMutex);
    m_currentFrame = frame.convertToFormat(QImage::Format_RGB888);
    m_frameUpdated = true;
    update();
}

/**
 * @brief 清除帧
 *
 * 清除当前显示的视频帧
 */
void GLVideoRenderer::clearFrame()
{
    QMutexLocker locker(&m_frameMutex);
    m_currentFrame = QImage();
    m_frameUpdated = true;
    update();
}

/**
 * @brief 初始化OpenGL
 *
 * 初始化OpenGL函数、着色器和纹理
 */
void GLVideoRenderer::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    initShaders();
    initTextures();
}

/**
 * @brief 调整OpenGL视口
 *
 * @param w 宽度
 * @param h 高度
 */
void GLVideoRenderer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void GLVideoRenderer::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMutexLocker locker(&m_frameMutex);

    if (m_currentFrame.isNull()) {
        // 绘制黑色背景
        return;
    }

    if (m_frameUpdated && m_texture) {
        updateTexture(m_currentFrame);
        m_frameUpdated = false;
    }

    if (!m_program || !m_texture) {
        return;
    }

    m_program->bind();
    m_texture->bind();

    // 计算保持宽高比的渲染区域
    float widgetAspect = (float)width() / height();
    float frameAspect = (float)m_currentFrame.width() / m_currentFrame.height();

    float left, right, top, bottom;
    if (widgetAspect > frameAspect) {
        // 宽度适应
        float scale = frameAspect / widgetAspect;
        left = (1.0f - scale) / 2.0f;
        right = 1.0f - left;
        top = 0.0f;
        bottom = 1.0f;
    } else {
        // 高度适应
        float scale = widgetAspect / frameAspect;
        left = 0.0f;
        right = 1.0f;
        top = (1.0f - scale) / 2.0f;
        bottom = 1.0f - top;
    }

    // 顶点坐标
    GLfloat vertices[] = {
        -1.0f + left * 2.0f,  1.0f - top * 2.0f,     // 左上
        -1.0f + right * 2.0f, 1.0f - top * 2.0f,     // 右上
        -1.0f + right * 2.0f, 1.0f - bottom * 2.0f,  // 右下
        -1.0f + left * 2.0f,  1.0f - bottom * 2.0f   // 左下
    };

    // 纹理坐标
    GLfloat texCoords[] = {
        0.0f, 0.0f,  // 左上
        1.0f, 0.0f,  // 右上
        1.0f, 1.0f,  // 右下
        0.0f, 1.0f   // 左下
    };

    m_program->enableAttributeArray(m_vertexAttr);
    m_program->enableAttributeArray(m_texCoordAttr);
    m_program->setAttributeArray(m_vertexAttr, vertices, 2);
    m_program->setAttributeArray(m_texCoordAttr, texCoords, 2);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    m_program->disableAttributeArray(m_vertexAttr);
    m_program->disableAttributeArray(m_texCoordAttr);

    m_texture->release();
    m_program->release();
}

void GLVideoRenderer::initShaders()
{
    m_program = new QOpenGLShaderProgram(this);

    // 顶点着色器
    const char *vertexShaderSource =
        "attribute vec2 vertex;\n"
        "attribute vec2 texCoord;\n"
        "varying vec2 texCoordVarying;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(vertex, 0.0, 1.0);\n"
        "    texCoordVarying = texCoord;\n"
        "}\n";

    // 片段着色器
    const char *fragmentShaderSource =
        "uniform sampler2D texture;\n"
        "varying vec2 texCoordVarying;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = texture2D(texture, texCoordVarying);\n"
        "}\n";

    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qWarning("顶点着色器编译失败: %s", m_program->log().toUtf8().constData());
        return;
    }

    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qWarning("片段着色器编译失败: %s", m_program->log().toUtf8().constData());
        return;
    }

    if (!m_program->link()) {
        qWarning("着色器程序链接失败: %s", m_program->log().toUtf8().constData());
        return;
    }

    m_vertexAttr = m_program->attributeLocation("vertex");
    m_texCoordAttr = m_program->attributeLocation("texCoord");
    m_textureUniform = m_program->uniformLocation("texture");
}

void GLVideoRenderer::initTextures()
{
    m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_texture->setMinificationFilter(QOpenGLTexture::Linear);
    m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);
}

void GLVideoRenderer::updateTexture(const QImage &image)
{
    if (!m_texture || image.isNull()) {
        return;
    }

    m_texture->destroy();
    m_texture->create();
    m_texture->setSize(image.width(), image.height());
    m_texture->setFormat(QOpenGLTexture::RGB8_UNorm);
    m_texture->allocateStorage();
    m_texture->setData(QOpenGLTexture::RGB, QOpenGLTexture::UInt8, image.bits());
}

// ==================== VideoDisplayWidget 实现 ====================

VideoDisplayWidget::VideoDisplayWidget(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_toolbarLayout(nullptr)
    , m_titleLabel(nullptr)
    , m_fullscreenBtn(nullptr)
    , m_snapshotBtn(nullptr)
    , m_recordBtn(nullptr)
    , m_videoRenderer(nullptr)
    , m_infoLabel(nullptr)
    , m_statsLabel(nullptr)
    , m_title(title)
    , m_isRecording(false)
    , m_isFullscreen(false)
{
    setupUi();
    setupConnections();
}

/**
 * @brief 析构函数
 */
VideoDisplayWidget::~VideoDisplayWidget()
{
}

void VideoDisplayWidget::setupUi()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(2, 2, 2, 2);
    m_mainLayout->setSpacing(2);

    // 工具栏
    m_toolbarLayout = new QHBoxLayout();
    m_toolbarLayout->setSpacing(5);

    m_titleLabel = new QLabel(m_title, this);
    m_titleLabel->setProperty("heading", true);

    m_fullscreenBtn = new QPushButton(tr("全屏"), this);
    m_fullscreenBtn->setMinimumSize(80, 36);

    m_snapshotBtn = new QPushButton(tr("截图"), this);
    m_snapshotBtn->setMinimumSize(80, 36);

    m_recordBtn = new QPushButton(tr("录制"), this);
    m_recordBtn->setMinimumSize(80, 36);
    m_recordBtn->setCheckable(true);

    m_toolbarLayout->addWidget(m_titleLabel);
    m_toolbarLayout->addStretch();
    m_toolbarLayout->addWidget(m_snapshotBtn);
    m_toolbarLayout->addWidget(m_recordBtn);
    m_toolbarLayout->addWidget(m_fullscreenBtn);

    m_mainLayout->addLayout(m_toolbarLayout);

    // 视频渲染器
    m_videoRenderer = new GLVideoRenderer(this);
    m_videoRenderer->setStyleSheet("background-color: black; border: 1px solid #333;");
    m_mainLayout->addWidget(m_videoRenderer, 1);

    // 信息栏
    QHBoxLayout *infoLayout = new QHBoxLayout();
    infoLayout->setSpacing(10);

    m_infoLabel = new QLabel(tr("未连接"), this);
    m_statsLabel = new QLabel(tr("FPS: 0 | 码率: 0 KB/s"), this);

    infoLayout->addWidget(m_infoLabel);
    infoLayout->addStretch();
    infoLayout->addWidget(m_statsLabel);

    m_mainLayout->addLayout(infoLayout);
}

void VideoDisplayWidget::setupConnections()
{
    connect(m_fullscreenBtn, &QPushButton::clicked, this, &VideoDisplayWidget::onFullscreenClicked);
    connect(m_snapshotBtn, &QPushButton::clicked, this, &VideoDisplayWidget::onSnapshotClicked);
    connect(m_recordBtn, &QPushButton::toggled, this, &VideoDisplayWidget::onRecordingToggled);
}

void VideoDisplayWidget::updateFrame(const QImage &frame)
{
    m_videoRenderer->setVideoFrame(frame);
}

void VideoDisplayWidget::clearDisplay()
{
    m_videoRenderer->clearFrame();
    m_infoLabel->setText(tr("未连接"));
    m_statsLabel->setText(tr("FPS: 0 | 码率: 0 KB/s"));
}

void VideoDisplayWidget::setTitle(const QString &title)
{
    m_title = title;
    m_titleLabel->setText(title);
}

void VideoDisplayWidget::setCameraInfo(const QString &cameraId, const QString &cameraName)
{
    m_cameraId = cameraId;
    m_cameraName = cameraName;
    updateInfoLabel();
}

void VideoDisplayWidget::setStatistics(int fps, qint64 bitrate)
{
    m_statsLabel->setText(tr("FPS: %1 | 码率: %2 KB/s").arg(fps).arg(bitrate));
}

void VideoDisplayWidget::setFullscreenEnabled(bool enabled)
{
    m_fullscreenBtn->setEnabled(enabled);
}

void VideoDisplayWidget::onFullscreenClicked()
{
    m_isFullscreen = !m_isFullscreen;
    if (m_isFullscreen) {
        m_fullscreenBtn->setText(tr("退出"));
    } else {
        m_fullscreenBtn->setText(tr("全屏"));
    }
    emit fullscreenRequested();
}

void VideoDisplayWidget::onSnapshotClicked()
{
    emit snapshotRequested();
}

void VideoDisplayWidget::onRecordingToggled(bool checked)
{
    m_isRecording = checked;
    if (checked) {
        m_recordBtn->setText(tr("停止"));
        m_recordBtn->setStyleSheet("background-color: #ff4444;");
    } else {
        m_recordBtn->setText(tr("录制"));
        m_recordBtn->setStyleSheet("");
    }
    emit recordingToggled(checked);
}

void VideoDisplayWidget::updateInfoLabel()
{
    if (!m_cameraId.isEmpty()) {
        m_infoLabel->setText(tr("相机: %1 (%2)").arg(m_cameraName).arg(m_cameraId));
    } else {
        m_infoLabel->setText(tr("未连接"));
    }
}
} // namespace ground_station
