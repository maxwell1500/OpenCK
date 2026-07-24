#ifndef PARTICLERENDERER_HPP
#define PARTICLERENDERER_HPP

#include <QVector>
#include <QVector3D>
#include <QColor>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

class ParticleSystem;

class ParticleRenderer
{
public:
    ParticleRenderer();
    ~ParticleRenderer();

    void render(ParticleSystem* system, const QMatrix4x4& modelView,
                const QMatrix4x4& projection);
    void setAdditive(bool additive) { m_additive = additive; }

private:
    void rebuildVBO(ParticleSystem* system);
    void initShader();

    QOpenGLBuffer m_vbo;
    QOpenGLShaderProgram* m_shader = nullptr;
    int m_vertexCount = 0;
    bool m_additive = false;
};

#endif
