#include "animationeventvalidator.hpp"

#include <algorithm>

QVector<AnimationEventValidator::EventRule> AnimationEventValidator::builtinRules()
{
    QVector<EventRule> rules;
    const auto add = [&](const char* name, std::initializer_list<const char*> types) {
        EventRule rule;
        rule.name = QString::fromLatin1(name);
        for (const char* t : types)
            rule.validAnimationTypes.append(QString::fromLatin1(t));
        rules.append(rule);
    };

    add("SyncRightFoot", {"Run", "Walk", "Jog", "Sprint"});
    add("SyncLeftFoot", {"Run", "Walk", "Jog", "Sprint"});
    add("WeaponFire", {"FireSingle", "FireAuto", "PowerAttack", "BowShoot"});
    add("WeaponSwing", {"MeleeAttack", "PowerAttack", "Bash"});
    add("HitFrame", {"MeleeAttack", "PowerAttack"});
    add("Impact", {"MeleeAttack", "PowerAttack", "Bash"});
    add("Reload", {"Reload", "ReloadLoop"});
    add("Draw", {"Draw", "Equip"});
    add("Sheathe", {"Sheathe", "Unequip"});
    add("Cast", {"SpellCast", "PowerCast"});
    add("Jump", {"Jump", "Fall"});
    add("Land", {"Jump", "Fall", "Land"});
    add("Voice", {"Voice", "IdleVoice", "Dialogue"});
    add("Footstep", {"Run", "Walk", "Jog", "Sprint", "Idle"});
    add("SoundPlay", {"Run", "Walk", "Idle", "MeleeAttack", "FireSingle"});

    return rules;
}

bool AnimationEventValidator::isEventValidForAnimation(
    const QString& eventName, const QString& animationType)
{
    const QVector<EventRule> rules = builtinRules();
    for (const EventRule& rule : rules)
    {
        if (rule.name.compare(eventName, Qt::CaseInsensitive) != 0)
            continue;
        for (const QString& t : rule.validAnimationTypes)
            if (t.compare(animationType, Qt::CaseInsensitive) == 0)
                return true;
        return false;  // found the rule; the animation type is not allowed
    }
    return true;  // unknown events are allowed
}

QVector<AnimationEventValidator::EventRule> AnimationEventValidator::matchingRules(
    const QString& eventName)
{
    QVector<EventRule> matches;
    const QVector<EventRule> rules = builtinRules();
    for (const EventRule& rule : rules)
    {
        if (rule.name.compare(eventName, Qt::CaseInsensitive) == 0
            || rule.name.startsWith(eventName, Qt::CaseInsensitive)
            || eventName.startsWith(rule.name, Qt::CaseInsensitive))
            matches.append(rule);
    }
    return matches;
}

QStringList AnimationEventValidator::matchingRuleNames(const QString& eventName)
{
    QStringList names;
    const QVector<EventRule> matches = matchingRules(eventName);
    for (const EventRule& rule : matches)
        names.append(rule.name);
    return names;
}
