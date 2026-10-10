#pragma once

#include <QOpenGLWidget>
#include <QColor>
#include <QVector3D>
#include <QString>
#include <QImage>

#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

// Live PBR-ish preview of a material: a textured quad lit with a
// Blinn-Phong approximation (albedo tint, roughness, metalness, emissive).
// Falls back gracefully when no OpenGL context is available (signals
// glUnavailable and renders a plain placeholder).
class MaterialPreviewWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit MaterialPreviewWidget(QWidget* parent = nullptr);
    ~MaterialPreviewWidget() override;

    // Loads the texture (DDS/PNG/TGA/TIF) shown in the preview. Returns
    // false when the image could not be decoded (previous texture kept).
    bool setTexture(const QString& path);
    void clearTexture();

    void setAlbedo(const QColor& color);
    void setRoughness(float r);   // 0..1
    void setMetalness(float m);   // 0..1
    void setEmissive(const QColor& color);

signals:
    void glUnavailable();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    void ensureShader();
    void applyPendingTexture();

    QOpenGLShaderProgram* m_shader = nullptr;
    QOpenGLBuffer m_vbo;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLTexture* m_texture = nullptr;
    bool m_hasTexture = false;
    bool m_glReady = false;
    bool m_glUnavailableReported = false;

    // QOpenGLTexture create/destroy require a current GL context, so set/clear
    // operations only decode and store the image; the paint path applies them.
    enum class TextureOp { None, Set, Clear };
    TextureOp m_pendingTextureOp = TextureOp::None;
    QImage m_pendingTexture;

    QColor m_albedo = QColor(160, 160, 160);
    float m_roughness = 0.5f;
    float m_metalness = 0.0f;
    QVector3D m_emissive = QVector3D(0.0f, 0.0f, 0.0f);
};
