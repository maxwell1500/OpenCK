#include <QtTest>
#include "../../src/model/tools/particlesimulation.hpp"

class TestParticleSimulation : public QObject
{
    Q_OBJECT

private slots:
    void testEmission();
    void testMaxParticles();
    void testLifetimeExpiry();
    void testGravity();
    void testColorAndSizeOverLifetime();
    void testVelocityBias();
    void testDeterministicWithSeed();
    void testResetClears();
};

void TestParticleSimulation::testEmission()
{
    ParticleSimulation sim;
    ParticleSimParams p;
    p.emissionRate = 10.0f; // 10/sec
    p.maxParticles = 100;
    sim.configure(p);
    sim.setSeed(1234);

    QCOMPARE(sim.aliveCount(), 0);
    sim.step(1.0f); // 10 particles
    QCOMPARE(sim.aliveCount(), 10);
    QCOMPARE(sim.totalSpawned(), 10);
}

void TestParticleSimulation::testMaxParticles()
{
    ParticleSimulation sim;
    ParticleSimParams p;
    p.emissionRate = 1000.0f;
    p.lifetime = 100.0f;
    p.maxParticles = 25;
    sim.configure(p);
    sim.setSeed(7);

    sim.step(1.0f);
    QCOMPARE(sim.aliveCount(), 25);
}

void TestParticleSimulation::testLifetimeExpiry()
{
    ParticleSimulation sim;
    ParticleSimParams p;
    p.emissionRate = 10.0f;
    p.lifetime = 1.0f;      // with 0.8..1.2 jitter
    p.minSpeed = 0.0f;
    p.maxSpeed = 0.0f;
    p.maxParticles = 1000;
    sim.configure(p);
    sim.setSeed(42);

    sim.step(0.5f);         // spawn some
    QVERIFY(sim.aliveCount() > 0);
    sim.step(5.0f);         // everything ages past lifetime
    QCOMPARE(sim.aliveCount(), 0);
}

void TestParticleSimulation::testGravity()
{
    ParticleSimulation sim;
    ParticleSimParams p;
    p.emissionRate = 0.0f;  // no new emission
    p.lifetime = 100.0f;
    p.gravityStrength = 10.0f;
    p.minSpeed = 0.0f;
    p.maxSpeed = 0.0f;
    p.maxParticles = 10;
    sim.configure(p);
    sim.setSeed(1);
    sim.setEmitterPosition(0.0f, 0.0f, 0.0f);

    // Manually spawn by stepping with a tiny rate trick: use emission then reset rate.
    ParticleSimParams emitParams;
    emitParams.emissionRate = 1.0f;
    emitParams.lifetime = 100.0f;
    emitParams.gravityStrength = 10.0f;
    emitParams.minSpeed = 0.0f;
    emitParams.maxSpeed = 0.0f;
    emitParams.maxParticles = 10;
    sim.configure(emitParams);
    sim.step(1.0f);
    QCOMPARE(sim.aliveCount(), 1);

    const float y0 = sim.particles()[0].y;
    sim.step(1.0f);
    const float y1 = sim.particles()[0].y;
    QVERIFY(y1 < y0);       // gravity pulls down
}

void TestParticleSimulation::testColorAndSizeOverLifetime()
{
    ParticleSimulation sim;
    ParticleSimParams p;
    p.emissionRate = 1.0f;
    p.lifetime = 10.0f;
    p.minSpeed = 0.0f;
    p.maxSpeed = 0.0f;
    p.maxParticles = 10;
    p.startA = 1.0f;
    p.endA = 0.0f;
    p.startSize = 2.0f;
    p.endSize = 0.0f;
    sim.configure(p);
    sim.setSeed(3);

    sim.step(1.0f);
    QCOMPARE(sim.aliveCount(), 1);
    const SimParticle first = sim.particles()[0];
    QVERIFY(first.size <= 2.0f);
    QVERIFY(first.a <= 1.0f);

    sim.step(1.0f);
    const SimParticle second = sim.particles()[0];
    QVERIFY(second.size < first.size);
    QVERIFY(second.a < first.a);
    QVERIFY(second.size < 2.0f);
    QVERIFY(second.a < 1.0f);
}

void TestParticleSimulation::testVelocityBias()
{
    ParticleSimulation sim;
    ParticleSimParams p;
    p.emissionRate = 1.0f;
    p.lifetime = 100.0f;
    p.minSpeed = 0.0f;
    p.maxSpeed = 0.0f;
    p.xVelocity = 5.0f;
    p.maxParticles = 10;
    sim.configure(p);
    sim.setSeed(9);

    sim.step(1.0f);
    QCOMPARE(sim.aliveCount(), 1);
    QVERIFY(sim.particles()[0].vx >= 5.0f);
}

void TestParticleSimulation::testDeterministicWithSeed()
{
    ParticleSimParams p;
    p.emissionRate = 20.0f;
    p.lifetime = 3.0f;
    p.xSpread = 90.0f;
    p.ySpread = 90.0f;
    p.maxParticles = 100;

    ParticleSimulation a;
    a.configure(p);
    a.setSeed(2024);
    a.step(1.0f);

    ParticleSimulation b;
    b.configure(p);
    b.setSeed(2024);
    b.step(1.0f);

    QCOMPARE(a.aliveCount(), b.aliveCount());
    for (int i = 0; i < a.particles().size(); ++i)
    {
        QCOMPARE(a.particles()[i].x, b.particles()[i].x);
        QCOMPARE(a.particles()[i].vx, b.particles()[i].vx);
        QCOMPARE(a.particles()[i].lifetime, b.particles()[i].lifetime);
    }
}

void TestParticleSimulation::testResetClears()
{
    ParticleSimulation sim;
    ParticleSimParams p;
    p.emissionRate = 10.0f;
    p.maxParticles = 100;
    sim.configure(p);
    sim.setSeed(5);

    sim.step(1.0f);
    QVERIFY(sim.aliveCount() > 0);
    sim.reset();
    QCOMPARE(sim.aliveCount(), 0);
    QCOMPARE(sim.totalSpawned(), 0);
}

QTEST_MAIN(TestParticleSimulation)
#include "test_particlesimulation.moc"
