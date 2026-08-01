#include "particleprojectilebindings.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "../../files/log/logger.hpp"

QVector<ParticleProjectileBindings::Binding> ParticleProjectileBindings::parseJson(const QByteArray& json)
{
    QVector<Binding> result;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError)
    {
        LOG_WARNING(QString("ParticleProjectileBindings: JSON error: %1")
            .arg(err.errorString()));
        return result;
    }

    QJsonArray arr;
    if (doc.isArray())
    {
        arr = doc.array();
    }
    else if (doc.isObject())
    {
        arr = doc.object().value(QStringLiteral("bindings")).toArray();
    }

    for (const QJsonValue& v : arr)
    {
        if (!v.isObject()) continue;
        const QJsonObject obj = v.toObject();
        Binding binding;
        binding.particleVariable = obj.value(QStringLiteral("particleVariable")).toString();
        binding.projectileAttribute = obj.value(QStringLiteral("projectileAttribute")).toString();
        if (binding.particleVariable.isEmpty() && binding.projectileAttribute.isEmpty())
            continue;
        result.append(binding);
    }
    return result;
}

bool ParticleProjectileBindings::loadFile(const QString& path, QVector<Binding>& out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_WARNING(QString("ParticleProjectileBindings::loadFile: cannot open %1").arg(path));
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();

    out = parseJson(data);
    LOG_DEBUG(QString("ParticleProjectileBindings: parsed %1 bindings from %2")
        .arg(out.size()).arg(path));
    return true;
}

QStringList ParticleProjectileBindings::knownProjectileVariables()
{
    return { QStringLiteral("BeamLength"),
             QStringLiteral("BeamLifeTime"),
             QStringLiteral("HasHit"),
             QStringLiteral("Velocity"),
             QStringLiteral("HitLocation") };
}
