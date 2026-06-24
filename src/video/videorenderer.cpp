/**
 * @file videorenderer.cpp
 * @brief Video Renderer Implementation
 *
 * Implements OpenGL-based video rendering functionality, including:
 * - Single stream renderer (VideoRenderer)
 * - Multi-stream renderer manager (MultiStreamRenderer)
 * - Multiple rendering modes (Stretch, KeepAspectRatio, KeepAspectRatioByExpanding)
 * - FPS statistics
 * - Texture uploading and shader rendering
 *
 * Architecture Design:
 * - VideoRenderer: Single-stream renderer based on QOpenGLWidget
 * - MultiStreamRenderer: Manages multiple VideoRenderer instances
 * - Uses shader programs for texture mapping
 * - Supports YUV and RGB formats
 */

#include "videorenderer.h"
#include <QMutexLocker>
#include <QPainter>

namespace ground_station {

// ==================== VideoRenderer Implementation ====================

/**
 * @brief Constructor
 * @param parent Parent widget pointer
 */
VideoRenderer::VideoRenderer(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_shaderProgram(nullptr)
    , m_textureId(0)
    , m_vboId(0)
    , m_vaoId(0)
    , m_videoStats(nullptr)
    , m_streamId(-1)
    , m_renderMode(RenderMode::KeepAspectRatio)
    , m_backgroundColor(Qt::black)
    , m_vsyncEnabled(true)
    , m_frameCount(0)
    , m_renderFps(0.0)
{
    m_renderTimer.start();
    m_fpsTimer.start();
    
    // Set update behavior to partial update
    setUpdateBehavior(QOpenGLWidget::PartialUpdate);
}

/**
 * @brief Destructor
 *
 * Cleans up all resources in OpenGL context
 */
VideoRenderer::~VideoRenderer()
{
    // Clean up resources in OpenGL context
    makeCurrent();
    
    if (m_textureId) {
        glDeleteTextures(1, &m_textureId);
        m_textureId = 0;
    }
    
    if (m_vboId) {
        glDeleteBuffers(1, &m_vboId);
        m_vboId = 0;
    }
    
    if (m_vaoId) {
        glDeleteVertexArrays(1, &m_vaoId);
        m_vaoId = 0;
    }
    
    if (m_shaderProgram) {
        delete m_shaderProgram;
        m_shaderProgram = nullptr;
    }
    
    doneCurrent();
}

void VideoRenderer::setFrameBuffer(const QSharedPointer<FrameBuffer> &buffer)
{
    m_frameBuffer = buffer;
}

void VideoRenderer::setVideoStats(VideoStats *stats)
{
    m_videoStats = stats;
}

void VideoRenderer::setStreamId(int streamId)
{
    m_streamId = streamId;
}

void VideoRenderer::setRenderMode(RenderMode mode)
{
    m_renderMode = mode;
    update();
}

void VideoRenderer::setBackgroundColor(const QColor &color)
{
    m_backgroundColor = color;
    update();
}

void VideoRenderer::updateFrame(const VideoFramePtr &frame)
{
    if (!frame || !frame->isValid) {
        return;
    }

    QMutexLocker locker(&m_frameMutex);
    m_pendingFrame = frame;
    
    // Request update
    update();
}

VideoFramePtr VideoRenderer::currentFrame() const
{
    QMutexLocker locker(&m_frameMutex);
    return m_currentFrame;
}

double VideoRenderer::renderFps() const
{
    return m_renderFps;
}

void VideoRenderer::clearFrame()
{
    QMutexLocker locker(&m_frameMutex);
    m_currentFrame.reset();
    m_pendingFrame.reset();
    update();
}

void VideoRenderer::setVSyncEnabled(bool enabled)
{
    m_vsyncEnabled = enabled;
    // Note: QOpenGLWidget's VSync is controlled by QSurfaceFormat
    // This only records the setting, actual effect needs to be set before creation
}

bool VideoRenderer::isVSyncEnabled() const
{
    return m_vsyncEnabled;
}

void VideoRenderer::initializeGL()
{
    initializeOpenGLFunctions();
    
    // Initialize shaders
    if (!initShaders()) {
        emit renderError("Shader initialization failed");
        return;
    }
    
    // Initialize vertex buffer
    if (!initVertexBuffer()) {
        emit renderError("Vertex buffer initialization failed");
        return;
    }
    
    // Create texture
    glGenTextures(1, &m_textureId);
    glBindTexture(GL_TEXTURE_2D, m_textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Set clear color
    glClearColor(m_backgroundColor.redF(), 
                 m_backgroundColor.greenF(), 
                 m_backgroundColor.blueF(), 
                 m_backgroundColor.alphaF());
    
    qDebug() << "VideoRenderer OpenGL initialized";
}

void VideoRenderer::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void VideoRenderer::paintGL()
{
    qint64 startTime = m_renderTimer.elapsed();
    
    // Clear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Check if there is a pending frame
    {
        QMutexLocker locker(&m_frameMutex);
        if (m_pendingFrame && m_pendingFrame->isValid) {
            m_currentFrame = m_pendingFrame;
            m_pendingFrame.reset();
        }
    }
    
    // Render current frame
    if (m_currentFrame && m_currentFrame->isValid) {
        renderFrame();
    }
    
    // Update FPS statistics
    updateFpsStats();
    
    // Record rendering time
    qint64 endTime = m_renderTimer.elapsed();
    double renderTimeMs = static_cast<double>(endTime - startTime);
    
    // Notify statistics object
    if (m_videoStats && m_currentFrame) {
        m_videoStats->recordFrameRendered(m_streamId, m_currentFrame->frameId, renderTimeMs);
    }
    
    // Emit frame rendered signal
    if (m_currentFrame) {
        emit frameRendered(m_currentFrame->frameId, renderTimeMs);
    }
}

const char *VideoRenderer::vertexShaderSource()
{
    return R"(
        attribute vec2 a_position;
        attribute vec2 a_texCoord;
        varying vec2 v_texCoord;
        
        void main() {
            gl_Position = vec4(a_position, 0.0, 1.0);
            v_texCoord = a_texCoord;
        }
    )";
}

const char *VideoRenderer::fragmentShaderSource()
{
    return R"(
        uniform sampler2D u_texture;
        varying vec2 v_texCoord;
        
        void main() {
            gl_FragColor = texture2D(u_texture, v_texCoord);
        }
    )";
}

bool VideoRenderer::initShaders()
{
    m_shaderProgram = new QOpenGLShaderProgram(this);
    
    // Compile vertex shader
    if (!m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource())) {
        qCritical() << "Vertex shader compilation failed:" << m_shaderProgram->log();
        return false;
    }
    
    // Compile fragment shader
    if (!m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource())) {
        qCritical() << "Fragment shader compilation failed:" << m_shaderProgram->log();
        return false;
    }
    
    // Link shader program
    if (!m_shaderProgram->link()) {
        qCritical() << "Shader program link failed:" << m_shaderProgram->log();
        return false;
    }
    
    return true;
}

bool VideoRenderer::initVertexBuffer()
{
    // Vertex data: position (x, y) + texture coordinate (u, v)
    // Full-screen quad
    GLfloat vertices[] = {
        // Position        Texture coordinate
        -1.0f,  1.0f,  0.0f, 0.0f,  // Top-left
        -1.0f, -1.0f,  0.0f, 1.0f,  // Bottom-left
         1.0f, -1.0f,  1.0f, 1.0f,  // Bottom-right
         1.0f,  1.0f,  1.0f, 0.0f,  // Top-right
    };
    
    // Index data
    GLuint indices[] = {
        0, 1, 2,  // First triangle
        0, 2, 3,  // Second triangle
    };
    
    // Create VAO
    glGenVertexArrays(1, &m_vaoId);
    glBindVertexArray(m_vaoId);
    
    // Create VBO
    glGenBuffers(1, &m_vboId);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Set vertex attributes
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    
    // Texture coordinate attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    
    // Create index buffer
    GLuint iboId;
    glGenBuffers(1, &iboId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glBindVertexArray(0);
    
    return true;
}

void VideoRenderer::updateTexture(const cv::Mat &image)
{
    if (image.empty()) {
        return;
    }
    
    glBindTexture(GL_TEXTURE_2D, m_textureId);
    
    // Determine format based on channel count
    GLenum format = GL_BGR;
    GLenum internalFormat = GL_RGB;
    
    if (image.channels() == 1) {
        format = GL_LUMINANCE;
        internalFormat = GL_LUMINANCE;
    } else if (image.channels() == 3) {
        format = GL_BGR;
        internalFormat = GL_RGB;
    } else if (image.channels() == 4) {
        format = GL_BGRA;
        internalFormat = GL_RGBA;
    }
    
    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, 
                 image.cols, image.rows, 0, 
                 format, GL_UNSIGNED_BYTE, image.data);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

QRectF VideoRenderer::calculateRenderRect(const QSizeF &frameSize, const QSizeF &widgetSize)
{
    if (frameSize.isEmpty() || widgetSize.isEmpty()) {
        return QRectF(-1, -1, 2, 2);  // Full screen
    }
    
    double frameAspect = frameSize.width() / frameSize.height();
    double widgetAspect = widgetSize.width() / widgetSize.height();
    
    double x, y, w, h;
    
    switch (m_renderMode) {
    case RenderMode::Stretch:
        // Stretch to fill entire viewport
        return QRectF(-1, -1, 2, 2);
        
    case RenderMode::KeepAspectRatio:
        // Keep aspect ratio, center display
        if (frameAspect > widgetAspect) {
            // Video is wider
            w = 2.0;
            h = 2.0 * widgetAspect / frameAspect;
        } else {
            // Video is taller
            h = 2.0;
            w = 2.0 * frameAspect / widgetAspect;
        }
        x = -w / 2.0;
        y = -h / 2.0;
        return QRectF(x, y, w, h);
        
    case RenderMode::KeepAspectRatioByExpanding:
        // Keep aspect ratio and expand
        if (frameAspect > widgetAspect) {
            // Video is wider, expand width
            h = 2.0;
            w = 2.0 * frameAspect / widgetAspect;
        } else {
            // Video is taller, expand height
            w = 2.0;
            h = 2.0 * widgetAspect / frameAspect;
        }
        x = -w / 2.0;
        y = -h / 2.0;
        return QRectF(x, y, w, h);
    }
    
    return QRectF(-1, -1, 2, 2);
}

void VideoRenderer::renderFrame()
{
    if (!m_currentFrame || m_currentFrame->data.empty()) {
        return;
    }
    
    // Update texture
    updateTexture(m_currentFrame->data);
    
    // Bind shader program
    m_shaderProgram->bind();
    
    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureId);
    m_shaderProgram->setUniformValue("u_texture", 0);
    
    // Calculate render rectangle
    QSizeF frameSize(m_currentFrame->width, m_currentFrame->height);
    QSizeF widgetSize(width(), height());
    QRectF renderRect = calculateRenderRect(frameSize, widgetSize);
    
    // Update vertex data (adjust based on render mode)
    GLfloat vertices[] = {
        // Position                          Texture coordinate
        static_cast<GLfloat>(renderRect.left()),  static_cast<GLfloat>(renderRect.top()),     0.0f, 0.0f,  // Top-left
        static_cast<GLfloat>(renderRect.left()),  static_cast<GLfloat>(renderRect.bottom()),  0.0f, 1.0f,  // Bottom-left
        static_cast<GLfloat>(renderRect.right()), static_cast<GLfloat>(renderRect.bottom()),  1.0f, 1.0f,  // Bottom-right
        static_cast<GLfloat>(renderRect.right()), static_cast<GLfloat>(renderRect.top()),     1.0f, 0.0f,  // Top-right
    };
    
    // Update VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_vboId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    
    // Draw
    glBindVertexArray(m_vaoId);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Release
    m_shaderProgram->release();
    glBindTexture(GL_TEXTURE_2D, 0);
}

void VideoRenderer::updateFpsStats()
{
    m_frameCount++;
    
    qint64 elapsed = m_fpsTimer.elapsed();
    if (elapsed >= 1000) {  // Update every second
        m_renderFps = static_cast<double>(m_frameCount) * 1000.0 / static_cast<double>(elapsed);
        m_frameCount = 0;
        m_fpsTimer.restart();
    }
}

// ==================== MultiStreamRenderer Implementation ====================

MultiStreamRenderer::MultiStreamRenderer(QObject *parent)
    : QObject(parent)
    , m_videoStats(nullptr)
    , m_renderMode(VideoRenderer::RenderMode::KeepAspectRatio)
{
}

MultiStreamRenderer::~MultiStreamRenderer()
{
    clearAll();
}

VideoRenderer* MultiStreamRenderer::addStream(int streamId, QWidget *parentWidget)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_renderers.contains(streamId)) {
        qWarning() << "Video stream" << streamId << "already exists";
        return m_renderers[streamId];
    }
    
    VideoRenderer *renderer = new VideoRenderer(parentWidget);
    renderer->setStreamId(streamId);
    renderer->setRenderMode(m_renderMode);
    
    if (m_videoStats) {
        renderer->setVideoStats(m_videoStats);
    }
    
    m_renderers[streamId] = renderer;
    return renderer;
}

void MultiStreamRenderer::removeStream(int streamId)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_renderers.contains(streamId)) {
        return;
    }
    
    VideoRenderer *renderer = m_renderers.take(streamId);
    renderer->deleteLater();
}

VideoRenderer* MultiStreamRenderer::getRenderer(int streamId) const
{
    QMutexLocker locker(&m_mutex);
    return m_renderers.value(streamId, nullptr);
}

bool MultiStreamRenderer::hasStream(int streamId) const
{
    QMutexLocker locker(&m_mutex);
    return m_renderers.contains(streamId);
}

QList<int> MultiStreamRenderer::streamIds() const
{
    QMutexLocker locker(&m_mutex);
    return m_renderers.keys();
}

void MultiStreamRenderer::setVideoStats(VideoStats *stats)
{
    QMutexLocker locker(&m_mutex);
    m_videoStats = stats;
    
    // Update all renderers
    for (auto renderer : m_renderers) {
        renderer->setVideoStats(stats);
    }
}

void MultiStreamRenderer::setRenderMode(VideoRenderer::RenderMode mode)
{
    QMutexLocker locker(&m_mutex);
    m_renderMode = mode;
    
    // Update all renderers
    for (auto renderer : m_renderers) {
        renderer->setRenderMode(mode);
    }
}

void MultiStreamRenderer::clearAll()
{
    QMutexLocker locker(&m_mutex);
    
    for (auto renderer : m_renderers) {
        renderer->deleteLater();
    }
    m_renderers.clear();
}

} // namespace ground_station