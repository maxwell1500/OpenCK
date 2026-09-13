#ifndef PARTICLESYSTEM_HPP
#define PARTICLESYSTEM_HPP

#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QColor>
#include <QTimer>

#include "../../model/tools/particlesimulation.hpp"

struct Particle {
    QVector3D position;
    QVector3D velocity;
    QColor color;
    float size;
    float age;
    float lifetime;
    bool alive;
};

struct ParticleSystemData;

// Qt/GL-facing wrapper around the headless ParticleSimulation: owns the
// update timer, mirrors the simulation's particles into a renderer-friendly
// form, and emits updated() so the viewport can repaint. All simulation math
// lives in ParticleSimulation so it can be unit-tested without a GL context.
class ParticleSystem : public QObject
{
    Q_OBJECT
public:
    explicit ParticleSystem(QObject* parent = nullptr);

    void setSettings(const ParticleSystemData* settings);
    void start();
    void stop();
    void pause();
    void reset();
    void setSpeed(float speed) { m_speed = speed; }
    bool isRunning() const { return m_running; }

    const QVector<Particle>& particles() const { return m_particles; }

    ParticleSimulation& simulation() { return m_sim; }
    const ParticleSimulation& simulation() const { return m_sim; }

signals:
    void updated();

private slots:
    void tick();

private:
    void syncParticles();

    ParticleSimulation m_sim;
    QVector<Particle> m_particles;
    QTimer m_timer;
    bool m_running = false;
    float m_speed = 1.0f;
};

#endif
