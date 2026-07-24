#include "particlerenderer.hpp"
#include "particlesystem.hpp"
#include "logger.hpp"

#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

struct ParticleVertex {
    float x, y, z;
    float r, g, b, a;
    float size;
};

ParticleRenderer::ParticleRenderer()
    : m_vbo(QOpenGLBuffer::VertexBuffer)
{
}

ParticleRenderer::~ParticleRenderer()
{
    if (m_vbo.isCreated()) m_vbo.destroy();
    delete m_shader;
}

void ParticleRenderer::initShader()
{
    if (m_shader) return;

    m_shader = new QOpenGLShaderProgram();

    const QString vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec4 aColor;
        layout(location = 2) in float aSize;

        uniform mat4 viewProj;

        out vec4 vColor;

        void main()
        {
            gl_Position = viewProj * vec4(aPosition, 1.0);
            vColor = aColor;
            gl_PointSize = aSize * 100.0;
        }
    )";

    const QString fragmentShaderSource = R"(
        #version 330 core
        in vec4 vColor;
        out vec4 FragColor;

        void main()
        {
            vec2 coord = gl_PointCoord - vec2(0.5);
            float dist = length(coord);
            if (dist > 0.5) discard;
            FragColor = vColor;
        }
    )";

    if (!m_shader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        LOG_ERROR("Particle vertex shader failed: " + m_shader->log());
        return;
    }
    if (!m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        LOG_ERROR("Particle fragment shader failed: " + m_shader->log());
        return;
    }
    if (!m_shader->link()) {
        LOG_ERROR("Particle shader link failed: " + m_shader->log());
        return;
    }
}

void ParticleRenderer::rebuildVBO(ParticleSystem* system)
{
    if (!system) return;

    const QVector<Particle>& particles = system->particles();

    QVector<ParticleVertex> verts;
    verts.reserve(particles.size());

    for (const auto& p : particles) {
        if (!p.alive) continue;
        ParticleVertex v;
        v.x = p.position.x();
        v.y = p.position.y();
        v.z = p.position.z();
        v.r = p.color.redF();
        v.g = p.color.greenF();
        v.b = p.color.blueF();
        v.a = p.color.alphaF();
        v.size = p.size;
        verts.append(v);
    }

    m_vertexCount = verts.size();
    if (m_vertexCount == 0) return;

    if (!m_vbo.isCreated()) m_vbo.create();

    m_vbo.bind();
    m_vbo.allocate(verts.constData(), verts.size() * sizeof(ParticleVertex));
    m_vbo.release();
}

void ParticleRenderer::render(ParticleSystem* system, const QMatrix4x4& modelView,
                              const QMatrix4x4& projection)
{
    if (!system) return;

    initShader();
    if (!m_shader || !m_shader->isLinked()) return;

    rebuildVBO(system);
    if (m_vertexCount == 0) return;

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    if (!f) return;

    GLboolean wasDepthMask = GL_TRUE;
    f->glGetBooleanv(GL_DEPTH_WRITEMASK, &wasDepthMask);
    GLboolean depthTestWasEnabled = GL_TRUE;
    f->glGetBooleanv(GL_DEPTH_TEST, &depthTestWasEnabled);
    GLboolean blendWasEnabled = GL_FALSE;
    f->glGetBooleanv(GL_BLEND, &blendWasEnabled);
    GLint prevBlendSrc = GL_SRC_ALPHA, prevBlendDst = GL_ONE_MINUS_SRC_ALPHA;
    f->glGetIntegerv(GL_BLEND_SRC_RGB, &prevBlendSrc);
    f->glGetIntegerv(GL_BLEND_DST_RGB, &prevBlendDst);

    f->glEnable(GL_BLEND);

    if (m_additive) {
        f->glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    } else {
        f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    f->glDepthMask(GL_FALSE);
    f->glDisable(GL_DEPTH_TEST);

    m_shader->bind();

    QMatrix4x4 viewProj = projection * modelView;
    m_shader->setUniformValue("viewProj", viewProj);

    m_vbo.bind();

    const int stride = sizeof(ParticleVertex);
    m_shader->setAttributeBuffer(0, GL_FLOAT, offsetof(ParticleVertex, x), 3, stride);
    m_shader->enableAttributeArray(0);
    m_shader->setAttributeBuffer(1, GL_FLOAT, offsetof(ParticleVertex, r), 4, stride);
    m_shader->enableAttributeArray(1);
    m_shader->setAttributeBuffer(2, GL_FLOAT, offsetof(ParticleVertex, size), 1, stride);
    m_shader->enableAttributeArray(2);

    f->glDrawArrays(GL_POINTS, 0, m_vertexCount);

    m_shader->disableAttributeArray(0);
    m_shader->disableAttributeArray(1);
    m_shader->disableAttributeArray(2);

    m_vbo.release();
    m_shader->release();

    f->glDepthMask(wasDepthMask);
    if (depthTestWasEnabled) f->glEnable(GL_DEPTH_TEST); else f->glDisable(GL_DEPTH_TEST);
    if (blendWasEnabled) f->glEnable(GL_BLEND); else f->glDisable(GL_BLEND);
    f->glBlendFunc(prevBlendSrc, prevBlendDst);
}
