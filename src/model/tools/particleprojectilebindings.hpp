#ifndef PARTICLEPROJECTILEBINDINGS_HPP
#define PARTICLEPROJECTILEBINDINGS_HPP

#include <QString>
#include <QStringList>
#include <QVector>

// Parses projectile variable bindings for particle emitters (the emitter
// projectile bindings bundle used by the Creation Kit). Each binding maps a
// particle variable to a projectile attribute. Known projectile variables:
// BeamLength, BeamLifeTime, HasHit, Velocity, etc.
struct ParticleProjectileBindings
{
    struct Binding
    {
        QString particleVariable;
        QString projectileAttribute;
    };

    // Parses JSON of the form:
    //   { "bindings": [ { "particleVariable": "BeamLength",
    //                     "projectileAttribute": "Distance" }, ... ] }
    // Unknown fields are ignored. Returns the parsed bindings.
    static QVector<Binding> parseJson(const QByteArray& json);

    // Loads and parses the given .pofx / .json file. Returns false if the
    // file cannot be read.
    static bool loadFile(const QString& path, QVector<Binding>& out);

    // The well-known projectile variable names the real CK exposes.
    static QStringList knownProjectileVariables();
};

#endif // PARTICLEPROJECTILEBINDINGS_HPP
