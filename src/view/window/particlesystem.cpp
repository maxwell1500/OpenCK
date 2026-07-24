#include "particlesystem.hpp"

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include "../../libs/files/nif/particle/particleeffects.hpp"

static float randFloat(float min, float max)
{
    return min + (max - min) * static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

ParticleSystem::ParticleSystem(QObject* parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &ParticleSystem::tick);
}

void ParticleSystem::setSettings(const ParticleSystemData* settings)
{
    if (!settings) return;

    m_emissionRate = settings->emissionRate;
    m_lifetime = settings->lifetime;
    m_minSpeed = settings->minSpeed;
    m_maxSpeed = settings->maxSpeed;
    m_xSpread = settings->xSpread;
    m_ySpread = settings->ySpread;
    m_zSpread = settings->zSpread;
    m_xVelocity = settings->xVelocity;
    m_yVelocity = settings->yVelocity;
    m_zVelocity = settings->zVelocity;
    m_gravityStrength = settings->gravityStrength;
    m_maxParticles = settings->maxParticles;
    m_startSize = settings->startSize;
    m_endSize = settings->endSize;
    m_startR = settings->startR;
    m_startG = settings->startG;
    m_startB = settings->startB;
    m_startA = settings->startA;
    m_endR = settings->endR;
    m_endG = settings->endG;
    m_endB = settings->endB;
    m_endA = settings->endA;
}

void ParticleSystem::start()
{
    if (m_running) return;
    m_running = true;
    m_timer.start(33);
}

void ParticleSystem::stop()
{
    m_running = false;
    m_timer.stop();
}

void ParticleSystem::pause()
{
    m_running = false;
    m_timer.stop();
}

void ParticleSystem::reset()
{
    m_particles.clear();
    m_emissionAccum = 0.0f;
    m_running = false;
    m_timer.stop();
    emit updated();
}

void ParticleSystem::tick()
{
    if (!m_running) return;

    const float dt = 0.033f * m_speed;

    float particlesPerTick = m_emissionRate * dt;
    m_emissionAccum += particlesPerTick;

    int activeCount = 0;
    for (const auto& p : m_particles) {
        if (p.alive) ++activeCount;
    }

    while (m_emissionAccum >= 1.0f && activeCount < m_maxParticles) {
        emitParticle();
        m_emissionAccum -= 1.0f;
        ++activeCount;
    }

    for (auto& p : m_particles) {
        if (p.alive) {
            updateParticle(p, dt);
        }
    }

    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(),
            [](const Particle& p) { return !p.alive; }),
        m_particles.end());

    emit updated();
}

void ParticleSystem::emitParticle()
{
    Particle p;
    p.position = m_emitterPos;
    p.age = 0.0f;
    p.lifetime = m_lifetime * randFloat(0.8f, 1.2f);
    p.alive = true;
    p.size = m_startSize;
    p.color = QColor::fromRgbF(m_startR, m_startG, m_startB, m_startA);

    float spreadX = m_xSpread * 3.14159265f / 180.0f;
    float spreadY = m_ySpread * 3.14159265f / 180.0f;
    float spreadZ = m_zSpread * 3.14159265f / 180.0f;

    float rx = randFloat(-spreadX, spreadX);
    float ry = randFloat(-spreadY, spreadY);
    float rz = randFloat(-spreadZ, spreadZ);

    QVector3D dir;
    dir.setX(std::sin(ry) * std::cos(rz));
    dir.setY(std::sin(rx) * std::cos(rz));
    dir.setZ(std::cos(rx) * std::cos(ry));
    if (dir.lengthSquared() < 0.001f) {
        dir = QVector3D(0.0f, 1.0f, 0.0f);
    }
    dir.normalize();

    float speed = randFloat(m_minSpeed, m_maxSpeed);
    p.velocity = dir * speed;
    p.velocity += QVector3D(m_xVelocity, m_yVelocity, m_zVelocity);

    m_particles.append(p);
}

void ParticleSystem::updateParticle(Particle& p, float dt)
{
    p.age += dt;
    if (p.age >= p.lifetime) {
        p.alive = false;
        return;
    }

    p.velocity += QVector3D(0.0f, -m_gravityStrength * dt, 0.0f);
    p.position += p.velocity * dt;

    float t = p.age / p.lifetime;
    float r = m_startR + (m_endR - m_startR) * t;
    float g = m_startG + (m_endG - m_startG) * t;
    float b = m_startB + (m_endB - m_startB) * t;
    float a = m_startA + (m_endA - m_startA) * t;
    p.color = QColor::fromRgbF(qBound(0.0f, r, 1.0f), qBound(0.0f, g, 1.0f),
                               qBound(0.0f, b, 1.0f), qBound(0.0f, a, 1.0f));
    p.size = m_startSize + (m_endSize - m_startSize) * t;
}
