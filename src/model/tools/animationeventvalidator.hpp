#ifndef ANIMATIONEVENTVALIDATOR_H
#define ANIMATIONEVENTVALIDATOR_H

#include <QString>
#include <QStringList>
#include <QVector>

// AnimationEventValidator checks animation event names against the sets the
// animation graph expects (e.g. SyncRightFoot on Run/Walk/Jog, WeaponFire on
// FireSingle/FireAuto, HitFrame on MeleeAttack). It exposes the canonical
// event sets so the editor can warn about events used on the wrong animation
// type.
struct AnimationEventValidator
{
    // A named event set with the animation types it is valid for.
    struct EventRule
    {
        QString name;                 // e.g. "SyncRightFoot"
        QStringList validAnimationTypes;  // e.g. {"Run","Walk","Jog"}
    };

    // The canonical event rules the editor cross-checks against.
    static QVector<EventRule> builtinRules();

    // True when the event may be used with the given animation type.
    static bool isEventValidForAnimation(const QString& eventName,
                                         const QString& animationType);

    // Returns every rule matching an event (by exact name or prefix).
    static QVector<EventRule> matchingRules(const QString& eventName);

    // Returns the names of rules matching an event.
    static QStringList matchingRuleNames(const QString& eventName);

    // Lists the animation types that accept the event.
    static QStringList validTypesFor(const QString& eventName);
};

#endif // ANIMATIONEVENTVALIDATOR_H
