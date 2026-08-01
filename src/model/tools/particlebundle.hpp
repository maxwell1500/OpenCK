#ifndef PARTICLEBUNDLE_HPP
#define PARTICLEBUNDLE_HPP

#include <QString>
#include <QVector>
#include <QJsonObject>

// Parses particle effect bundle files (.pofx). The real format is a JSON
// bundle of named effect nodes, each describing one emitter's properties.
// OpenCK reads the subset relevant to the particle editor UI: node name,
// parent bundle, lifetime/age, alpha curve, velocity, gravity, drag,
// rotation, ribbon mode, UV scroll, and attractors.
struct ParticleBundle
{
    struct Node
    {
        QString name;
        QString bundle;            // owning bundle name
        float age = 1.0f;          // lifetime (seconds)
        float alphaByCurve = 1.0f;
        float velocity = 0.0f;
        float gravity = 0.0f;
        float drag = 0.0f;
        float rotationSpeed = 0.0f;
        bool ribbon = false;
        bool uvScroll = false;
        QString texture;
    };

    QString name;
    QVector<Node> nodes;

    // Parses a .pofx JSON object (or array of objects). Each object may
    // have a "name"/"bundle" and nested "nodes"/"emitters" arrays.
    static ParticleBundle parse(const QByteArray& json);

    // Loads and parses the given file. Returns false if unreadable.
    static bool loadFile(const QString& path, ParticleBundle& out);
};

#endif // PARTICLEBUNDLE_HPP
