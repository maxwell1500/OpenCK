#ifndef PARTICLESIMULATION_HPP
#define PARTICLESIMULATION_HPP

#include <QVector>

// Headless, deterministic particle simulation core. The viewport's
// ParticleSystem owns one of these and drives it from a QTimer; keeping the
// math free of Qt widgets/OpenGL lets the emission and integration logic be
// unit-tested without a GL context.
//
// The RNG is a small reproducible LCG (not std::rand) so tests are stable
// across platforms and runs: seed with setSeed() before stepping.

struct ParticleSimParams
{
    float emissionRate = 10.0f;
    float lifetime = 5.0f;
    float minSpeed = 1.0f;
    float maxSpeed = 5.0f;
    float xSpread = 0.0f;   // degrees
    float ySpread = 0.0f;   // degrees
    float zSpread = 0.0f;   // degrees
    float xVelocity = 0.0f;
    float yVelocity = 0.0f;
    float zVelocity = 0.0f;
    float gravityStrength = 0.0f;
    float startSize = 1.0f;
    float endSize = 0.0f;
    float startR = 1.0f, startG = 1.0f, startB = 1.0f, startA = 1.0f;
    float endR = 1.0f, endG = 1.0f, endB = 1.0f, endA = 0.0f;
    int maxParticles = 100;
};

struct SimParticle
{
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float vx = 0.0f, vy = 0.0f, vz = 0.0f;
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
    float size = 1.0f;
    float age = 0.0f;
    float lifetime = 1.0f;
    bool alive = true;
};

class ParticleSimulation
{
public:
    void configure(const ParticleSimParams& params);
    const ParticleSimParams& params() const { return m_params; }

    void reset();
    void setEmitterPosition(float x, float y, float z);

    /// Advances the simulation by dt seconds. Spawns particles at the rate
    /// in the params, integrates the alive ones, and removes dead ones.
    void step(float dt);

    const QVector<SimParticle>& particles() const { return m_particles; }
    int aliveCount() const;
    int totalSpawned() const { return m_totalSpawned; }

    /// Resets the RNG to a known state for deterministic output.
    void setSeed(quint32 seed);

private:
    float randFloat(float lo, float hi);
    void emitParticle();

    ParticleSimParams m_params;
    QVector<SimParticle> m_particles;
    float m_emissionAccum = 0.0f;
    float m_emitterX = 0.0f, m_emitterY = 0.0f, m_emitterZ = 0.0f;
    int m_totalSpawned = 0;
    quint32 m_rng = 1u;
};

#endif // PARTICLESIMULATION_HPP
