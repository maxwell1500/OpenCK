#include "particlesystem.hpp"

#include "../../libs/files/nif/particle/particleeffects.hpp"

ParticleSystem::ParticleSystem(QObject* parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &ParticleSystem::tick);
}

void ParticleSystem::setSettings(const ParticleSystemData* settings)
{
    if (!settings) return;

    ParticleSimParams params;
    params.emissionRate = settings->emissionRate;
    params.lifetime = settings->lifetime;
    params.minSpeed = settings->minSpeed;
    params.maxSpeed = settings->maxSpeed;
    params.xSpread = settings->xSpread;
    params.ySpread = settings->ySpread;
    params.zSpread = settings->zSpread;
    params.xVelocity = settings->xVelocity;
    params.yVelocity = settings->yVelocity;
    params.zVelocity = settings->zVelocity;
    params.gravityStrength = settings->gravityStrength;
    params.maxParticles = settings->maxParticles;
    params.startSize = settings->startSize;
    params.endSize = settings->endSize;
    params.startR = settings->startR;
    params.startG = settings->startG;
    params.startB = settings->startB;
    params.startA = settings->startA;
    params.endR = settings->endR;
    params.endG = settings->endG;
    params.endB = settings->endB;
    params.endA = settings->endA;

    m_sim.configure(params);
    m_sim.reset();
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
    m_sim.reset();
    m_particles.clear();
    m_running = false;
    m_timer.stop();
    emit updated();
}

void ParticleSystem::tick()
{
    if (!m_running) return;

    m_sim.step(0.033f * m_speed);
    syncParticles();
    emit updated();
}

void ParticleSystem::syncParticles()
{
    const QVector<SimParticle>& sim = m_sim.particles();
    m_particles.clear();
    m_particles.reserve(sim.size());
    for (const auto& p : sim)
    {
        if (!p.alive) continue;
        Particle out;
        out.position = QVector3D(p.x, p.y, p.z);
        out.velocity = QVector3D(p.vx, p.vy, p.vz);
        out.color = QColor::fromRgbF(p.r, p.g, p.b, p.a);
        out.size = p.size;
        out.age = p.age;
        out.lifetime = p.lifetime;
        out.alive = p.alive;
        m_particles.append(out);
    }
}
