#include "particlesimulation.hpp"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float kPi = 3.14159265358979323846f;

float lerp(float a, float b, float t) { return a + (b - a) * t; }
float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
} // namespace

void ParticleSimulation::configure(const ParticleSimParams& params)
{
    m_params = params;
}

void ParticleSimulation::reset()
{
    m_particles.clear();
    m_emissionAccum = 0.0f;
    m_totalSpawned = 0;
}

void ParticleSimulation::setEmitterPosition(float x, float y, float z)
{
    m_emitterX = x;
    m_emitterY = y;
    m_emitterZ = z;
}

void ParticleSimulation::setSeed(quint32 seed)
{
    m_rng = seed ? seed : 1u;
}

float ParticleSimulation::randFloat(float lo, float hi)
{
    // Numerical Recipes LCG (reproducible across platforms).
    m_rng = m_rng * 1664525u + 1013904223u;
    const float unit = static_cast<float>((m_rng >> 8) & 0x00FFFFFFu)
        / static_cast<float>(0x01000000u);
    return lo + (hi - lo) * unit;
}

int ParticleSimulation::aliveCount() const
{
    int n = 0;
    for (const auto& p : m_particles)
        if (p.alive) ++n;
    return n;
}

void ParticleSimulation::emitParticle()
{
    SimParticle p;
    p.x = m_emitterX;
    p.y = m_emitterY;
    p.z = m_emitterZ;
    p.age = 0.0f;
    p.lifetime = m_params.lifetime * randFloat(0.8f, 1.2f);
    p.alive = true;
    p.size = m_params.startSize;
    p.r = m_params.startR;
    p.g = m_params.startG;
    p.b = m_params.startB;
    p.a = m_params.startA;

    const float spreadX = m_params.xSpread * kPi / 180.0f;
    const float spreadY = m_params.ySpread * kPi / 180.0f;
    const float spreadZ = m_params.zSpread * kPi / 180.0f;

    const float rx = randFloat(-spreadX, spreadX);
    const float ry = randFloat(-spreadY, spreadY);
    const float rz = randFloat(-spreadZ, spreadZ);

    float dx = std::sin(ry) * std::cos(rz);
    float dy = std::sin(rx) * std::cos(rz);
    float dz = std::cos(rx) * std::cos(ry);
    float len = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (len < 0.001f)
    {
        dx = 0.0f;
        dy = 1.0f;
        dz = 0.0f;
        len = 1.0f;
    }
    dx /= len;
    dy /= len;
    dz /= len;

    const float speed = randFloat(m_params.minSpeed, m_params.maxSpeed);
    p.vx = dx * speed + m_params.xVelocity;
    p.vy = dy * speed + m_params.yVelocity;
    p.vz = dz * speed + m_params.zVelocity;

    m_particles.append(p);
    ++m_totalSpawned;
}

void ParticleSimulation::step(float dt)
{
    if (dt <= 0.0f)
        return;

    m_emissionAccum += m_params.emissionRate * dt;

    int active = aliveCount();
    while (m_emissionAccum >= 1.0f && active < m_params.maxParticles)
    {
        emitParticle();
        m_emissionAccum -= 1.0f;
        ++active;
    }

    for (auto& p : m_particles)
    {
        if (!p.alive)
            continue;

        p.age += dt;
        if (p.age >= p.lifetime)
        {
            p.alive = false;
            continue;
        }

        p.vy += -m_params.gravityStrength * dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.z += p.vz * dt;

        const float t = p.age / p.lifetime;
        p.r = clamp01(lerp(m_params.startR, m_params.endR, t));
        p.g = clamp01(lerp(m_params.startG, m_params.endG, t));
        p.b = clamp01(lerp(m_params.startB, m_params.endB, t));
        p.a = clamp01(lerp(m_params.startA, m_params.endA, t));
        p.size = lerp(m_params.startSize, m_params.endSize, t);
    }

    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(),
            [](const SimParticle& p) { return !p.alive; }),
        m_particles.end());
}
