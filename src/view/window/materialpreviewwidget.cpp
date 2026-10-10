#include "materialpreviewwidget.hpp"

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLContext>
#include <QImage>
#include <QFileInfo>
#include "../../libs/files/log/logger.hpp"


MaterialPreviewWidget::MaterialPreviewWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
}

MaterialPreviewWidget::~MaterialPreviewWidget()
{
    if (m_shader)
        delete m_shader;
    if (m_texture)
    {
        if (!context()) {
            delete m_texture;
        } else {
            makeCurrent();
            delete m_texture;
            doneCurrent();
        }
        m_texture = nullptr;
    }
}

void MaterialPreviewWidget::ensureShader()
{
    if (m_shader || !context() || !context()->isValid())
        return;

    m_shader = new QOpenGLShaderProgram(this);

    const QString vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        out vec2 vUV;
        void main()
        {
            vUV = aPos * 0.5 + 0.5;
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";

    const QString fragmentShaderSource = R"(
        #version 330 core
        in vec2 vUV;
        out vec4 FragColor;

        uniform sampler2D tex;
        uniform vec3 uAlbedoTint;
        uniform float uRoughness;
        uniform float uMetalness;
        uniform vec3 uEmissive;

        void main()
        {
            vec3 texColor = texture(tex, vUV).rgb;
            vec3 albedo = texColor * uAlbedoTint;

            vec3 normal = vec3(0.0, 0.0, 1.0);
            vec3 lightDir = normalize(vec3(0.4, 0.8, 0.6));
            vec3 viewDir = vec3(0.0, 0.0, 1.0);

            float diffuse = max(dot(normal, lightDir), 0.0f);
            vec3 halfV = normalize(lightDir + viewDir);
            float shininess = mix(128.0, 2.0, uRoughness);
            float specular = pow(max(dot(normal, halfV), 0.0f), shininess);

            vec3 color = albedo * (0.25 + 0.75 * diffuse)
                + specular * mix(vec3(1.0), albedo, uMetalness) * (1.0 - uRoughness);
            color += uEmissive;
            FragColor = vec4(color, 1.0);
        }
    )";

    m_shader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    if (!m_shader->link())
    {
        LOG_WARNING(QString("MaterialPreviewWidget: shader link failed: %1")
            .arg(m_shader->log()));
        delete m_shader;
        m_shader = nullptr;
    }

    // Fullscreen quad: position.xy, uv.xy.
    const float quad[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };
    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(quad, sizeof(quad));

    m_vbo.release();
    m_vao.create();
    m_vao.bind();
    m_vao.release();

    // 1x1 neutral placeholder texture.
    QImage placeholder(1, 1, QImage::Format_RGB888);
    placeholder.fill(QColor(255, 255, 255));
    m_texture = new QOpenGLTexture(placeholder);
    m_hasTexture = false;
    m_glReady = (m_shader != nullptr);
    applyPendingTexture();
    LOG_DEBUG("MaterialPreviewWidget: OpenGL preview ready");
}

void MaterialPreviewWidget::applyPendingTexture()
{
    if (m_pendingTextureOp == TextureOp::None)
        return;

    if (m_texture)
    {
        delete m_texture;
        m_texture = nullptr;
    }
    m_hasTexture = false;

    if (m_pendingTextureOp == TextureOp::Set)
    {
        m_texture = new QOpenGLTexture(m_pendingTexture.convertToFormat(QImage::Format_RGBA8888));
        m_hasTexture = true;
    }
    m_pendingTextureOp = TextureOp::None;
}

void MaterialPreviewWidget::initializeGL()
{
    ensureShader();
    if (!m_glReady)
    {
        if (!m_glUnavailableReported)
        {
            m_glUnavailableReported = true;
            emit glUnavailable();
        }
    }
}

void MaterialPreviewWidget::resizeGL(int, int)
{
}

void MaterialPreviewWidget::paintGL()
{
    QOpenGLContext* ctx = context();
    if (!ctx || !ctx->isValid())
    {
        if (!m_glUnavailableReported)
        {
            m_glUnavailableReported = true;
            emit glUnavailable();
        }
        return;
    }

    ensureShader();
    glClear(GL_COLOR_BUFFER_BIT);
    if (!m_glReady || !m_shader)
        return;

    m_shader->bind();
    m_shader->setUniformValue("uAlbedoTint",
        QVector3D(m_albedo.redF(), m_albedo.greenF(), m_albedo.blueF()));
    m_shader->setUniformValue("uRoughness", m_roughness);
    m_shader->setUniformValue("uMetalness", m_metalness);
    m_shader->setUniformValue("uEmissive", m_emissive);

    if (m_texture)
        m_texture->bind(0);
    m_shader->setUniformValue("tex", 0);

    m_vao.bind();
    m_vbo.bind();
    m_shader->setAttributeBuffer(0, GL_FLOAT, 0, 2, 2 * sizeof(float));
    m_shader->enableAttributeArray(0);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    m_shader->disableAttributeArray(0);
    m_vbo.release();
    m_vao.release();
    m_shader->release();
}

bool MaterialPreviewWidget::setTexture(const QString& path)
{
    QImage image;
    if (QFileInfo(path).suffix().compare(QLatin1String("dds"), Qt::CaseInsensitive) == 0)
        image = QImage(path, "DDS");
    else
        image = QImage(path);

    if (image.isNull())
    {
        LOG_WARNING(QString("MaterialPreviewWidget: cannot decode texture %1").arg(path));
        return false;
    }

    m_pendingTexture = image;
    m_pendingTextureOp = TextureOp::Set;
    LOG_DEBUG(QString("MaterialPreviewWidget: texture loaded %1 (%2x%3)")
        .arg(path).arg(image.width()).arg(image.height()));
    update();
    return true;
}

void MaterialPreviewWidget::clearTexture()
{
    m_pendingTextureOp = TextureOp::Clear;
    update();
}

void MaterialPreviewWidget::setAlbedo(const QColor& color)
{
    m_albedo = color;
    update();
}

void MaterialPreviewWidget::setRoughness(float r)
{
    m_roughness = qBound(0.0f, r, 1.0f);
    update();
}

void MaterialPreviewWidget::setMetalness(float m)
{
    m_metalness = qBound(0.0f, m, 1.0f);
    update();
}

void MaterialPreviewWidget::setEmissive(const QColor& color)
{
    m_emissive = QVector3D(color.redF(), color.greenF(), color.blueF());
    update();
}
