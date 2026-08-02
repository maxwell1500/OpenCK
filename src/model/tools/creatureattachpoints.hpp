#ifndef CREATUREATTACHPOINTS_H
#define CREATUREATTACHPOINTS_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonObject>

// CreatureAttachPoints models the Starfield CCT creature attach-point system
// (ap_CCT_*). A creature defines attach points — named slots on the skeleton
// where other assets mount — for each of the combat/basic aspects: attack,
// defense, faction, diet, size, skin, speed, temperament. As with the planet
// (PNDT) model, the field structure is captured now with JSON serialization;
// the on-disk binary encoder is deferred until a real Starfield ESM record
// is available to validate against.
struct CreatureAttachPoints
{
    QString editorId;

    // Each aspect maps to an attach-point name (empty = not set).
    struct AttachPoint
    {
        QString aspect;          // e.g. "Attack"
        QString boneName;        // e.g. "ap_CCT_Attack"
        bool enabled = true;
    };
    QVector<AttachPoint> attachPoints;

    QString diet;                // e.g. "Carnivore"
    QString size;                // e.g. "Large"
    QString temperament;         // e.g. "Aggressive"
    QString speed;               // e.g. "Fast"

    // Serializes the definition to JSON.
    QJsonObject toJson() const;

    // Loads from JSON.
    static CreatureAttachPoints fromJson(const QJsonObject& obj);

    // The canonical CCT aspects the real CK defines.
    static QStringList standardAspects();
};

#endif // CREATUREATTACHPOINTS_H
