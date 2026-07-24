#ifndef PARTICLESYSTEM_HPP
#define PARTICLESYSTEM_HPP

#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QColor>
#include <QTimer>

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

signals:
    void updated();

private slots:
    void tick();

private:
    void emitParticle();
    void updateParticle(Particle& p, float dt);

    QVector<Particle> m_particles;
    QTimer m_timer;
    bool m_running = false;
    float m_speed = 1.0f;
    float m_emissionAccum = 0.0f;

    float m_emissionRate = 10.0f;
    float m_lifetime = 5.0f;
    float m_minSpeed = 1.0f;
    float m_maxSpeed = 5.0f;
    float m_xSpread = 0.0f;
    float m_ySpread = 0.0f;
    float m_zSpread = 0.0f;
    float m_xVelocity = 0.0f;
    float m_yVelocity = 0.0f;
    float m_zVelocity = 0.0f;
    float m_gravityStrength = 0.0f;
    float m_startSize = 1.0f;
    float m_endSize = 0.0f;
    float m_startR = 1, m_startG = 1, m_startB = 1, m_startA = 1;
    float m_endR = 1, m_endG = 1, m_endB = 1, m_endA = 0;
    int m_maxParticles = 100;
    QVector3D m_emitterPos;
};

#endif
