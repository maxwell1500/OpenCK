#include "packagesemantics.hpp"

#include <QObject>
#include <QStringList>

namespace openck
{

/// Translation helpers. The library is not a QObject, so the strings live
/// here rather than inside a class.
namespace PackageSemanticsStrings
{
QString any(const QString& field);
QString indexed(const QString& field, int value);
QString unknownKind(quint32 type);
QString kindNeedsReference(const QString& kindName);
QString kindExpectsRoad(const QString& kindName);
} // namespace PackageSemanticsStrings

namespace
{

constexpr int kPkdtCommonSize = 12;   // type, flags, interrupt-or-extra
constexpr int kScheduleOffset = 12;  // month, weekday, date, hour
constexpr int kMinuteOffset = 16;   // single byte, 0..119, or 0xFF for "any minute"
constexpr int kDoAllOffset = 17;    // "perform all / once" flag
constexpr quint8 kAnyMinute = 0xFF;

constexpr quint8 kUnusedMonth = 0xFF;
constexpr quint8 kUnusedWeekday = 0xFF;
constexpr quint8 kUnusedDate = 0xFF;
constexpr quint8 kAnyHour = 0xFF;

bool readU8At(const QByteArray& bytes, int offset, quint8* out)
{
    if (offset < 0 || offset + 1 > bytes.size())
    {
        return false;
    }
    *out = static_cast<quint8>(bytes.at(offset));
    return true;
}

bool readU32At(const QByteArray& bytes, int offset, quint32* out)
{
    if (offset < 0 || offset + 4 > bytes.size())
    {
        return false;
    }
    *out = static_cast<quint32>(static_cast<quint8>(bytes.at(offset)))
        | (static_cast<quint32>(static_cast<quint8>(bytes.at(offset + 1))) << 8)
        | (static_cast<quint32>(static_cast<quint8>(bytes.at(offset + 2))) << 16)
        | (static_cast<quint32>(static_cast<quint8>(bytes.at(offset + 3))) << 24);
    return true;
}

void writeU32At(QByteArray& bytes, int offset, quint32 value)
{
    if (offset < 0 || offset + 4 > bytes.size())
    {
        return;
    }
    bytes[offset] = static_cast<char>(value & 0xFF);
    bytes[offset + 1] = static_cast<char>((value >> 8) & 0xFF);
    bytes[offset + 2] = static_cast<char>((value >> 16) & 0xFF);
    bytes[offset + 3] = static_cast<char>((value >> 24) & 0xFF);
}

} // namespace

QString PackageSemanticsStrings::any(const QString& field)
{
    return QObject::tr("%1 (any)").arg(field);
}

QString PackageSemanticsStrings::indexed(const QString& field, int value)
{
    return QObject::tr("%1 %2").arg(field).arg(value);
}

QString PackageSemanticsStrings::unknownKind(quint32 type)
{
    return QObject::tr("Unknown package type %1.").arg(type);
}

QString PackageSemanticsStrings::kindNeedsReference(const QString& kindName)
{
    return QObject::tr("'%1' packages must reference a target (PTDT), but this "
                       "package has none.").arg(kindName);
}

QString PackageSemanticsStrings::kindExpectsRoad(const QString& kindName)
{
    return QObject::tr("'%1' expects road/travel flags (XALG) that this "
                       "package does not set.").arg(kindName);
}

bool scheduleConstraintSet(const PackageSchedule& schedule)
{
    return schedule.month != kUnusedMonth || schedule.weekday != kUnusedWeekday
        || schedule.date != kUnusedDate || schedule.hour != kAnyHour;
}

QVector<ScheduleCheck> scheduleChecks(const PackageSchedule& schedule,
                                      const QDateTime& now)
{
    const QStringList months = {
        QStringLiteral("January"), QStringLiteral("February"),
        QStringLiteral("March"), QStringLiteral("April"),
        QStringLiteral("May"), QStringLiteral("June"),
        QStringLiteral("July"), QStringLiteral("August"),
        QStringLiteral("September"), QStringLiteral("October"),
        QStringLiteral("November"), QStringLiteral("December")};
    const QStringList weekdays = {
        QStringLiteral("Sunday"), QStringLiteral("Monday"),
        QStringLiteral("Tuesday"), QStringLiteral("Wednesday"),
        QStringLiteral("Thursday"), QStringLiteral("Friday"),
        QStringLiteral("Saturday")};

    QVector<ScheduleCheck> checks;
    const bool haveNow = now.isValid();

    auto append = [&checks, haveNow](const QString& name, bool set, bool matches) {
        ScheduleCheck c;
        if (!set)
        {
            c.name = PackageSemanticsStrings::any(name);
            c.active = false;
        }
        else
        {
            c.name = name;
            c.active = haveNow && matches;
        }
        checks.append(c);
    };

    if (schedule.month != kUnusedMonth)
    {
        const int index = static_cast<int>(schedule.month);
        append(months.value(index - 1,
                            PackageSemanticsStrings::indexed(QStringLiteral("Month"), index)),
               true, haveNow && now.date().month() == index);
    }
    if (schedule.weekday != kUnusedWeekday)
    {
        const int index = static_cast<int>(schedule.weekday);
        append(weekdays.value(index, PackageSemanticsStrings::indexed(QStringLiteral("Weekday"), index)),
               true, haveNow && now.date().dayOfWeek() == index);
    }
    if (schedule.date != kUnusedDate)
    {
        append(PackageSemanticsStrings::indexed(QStringLiteral("Day of month"), schedule.date),
               true, haveNow && now.date().day() == schedule.date);
    }
    if (schedule.hour != kAnyHour)
    {
        append(PackageSemanticsStrings::indexed(QStringLiteral("Hour"), schedule.hour),
               true, haveNow && now.time().hour() == schedule.hour);
    }
    if (schedule.minute >= 0)
    {
        append(PackageSemanticsStrings::indexed(QStringLiteral("Minute"), schedule.minute),
               true, haveNow && now.time().minute() == schedule.minute);
    }
    return checks;
}

QVector<PackageIssue> validatePackageData(const PackageRecord& record,
                                          const PackageData& data)
{
    QVector<PackageIssue> issues;
    const PackageKind kind = packageKindFromU32(data.type);
    if (kind == PackageKind::Unknown)
    {
        issues.append({PackageIssueSeverity::Warning,
                       PackageSemanticsStrings::unknownKind(data.type)});
        return issues;
    }

    const bool needsReference =
        kind == PackageKind::Find || kind == PackageKind::Escort
        || kind == PackageKind::Eat || kind == PackageKind::Sleep
        || kind == PackageKind::Accompany || kind == PackageKind::UseItemAt
        || kind == PackageKind::CastMagic || kind == PackageKind::Activate
        || kind == PackageKind::Sit || kind == PackageKind::ForceGreet;
    if (needsReference && record.targetIds.isEmpty())
    {
        issues.append({PackageIssueSeverity::Error,
                       PackageSemanticsStrings::kindNeedsReference(packageKindName(kind))});
    }

    if (packageKindIsRoad(data.type) && data.flags == 0)
    {
        issues.append({PackageIssueSeverity::Warning,
                       PackageSemanticsStrings::kindExpectsRoad(packageKindName(kind))});
    }

    if (scheduleConstraintSet(data.schedule) && data.schedule.minute < 0)
    {
        issues.append({PackageIssueSeverity::Warning,
                       QStringLiteral("The schedule sets a date or hour but no minute; the engine may hold it open.")});
    }
    return issues;
}



QString packageKindName(PackageKind kind)
{
    switch (kind)
    {
    case PackageKind::Find:        return QStringLiteral("Find");
    case PackageKind::Follow:      return QStringLiteral("Follow");
    case PackageKind::Escort:      return QStringLiteral("Escort");
    case PackageKind::Eat:         return QStringLiteral("Eat");
    case PackageKind::Sleep:       return QStringLiteral("Sleep");
    case PackageKind::Wander:      return QStringLiteral("Wander");
    case PackageKind::Travel:      return QStringLiteral("Travel");
    case PackageKind::Accompany:    return QStringLiteral("Accompany");
    case PackageKind::UseItemAt:   return QStringLiteral("UseItemAt");
    case PackageKind::Ambush:      return QStringLiteral("Ambush");
    case PackageKind::FleeNonCombat: return QStringLiteral("FleeNonCombat");
    case PackageKind::CastMagic:   return QStringLiteral("CastMagic");
    case PackageKind::Combat:      return QStringLiteral("Combat");
    case PackageKind::Process:     return QStringLiteral("Process");
    case PackageKind::Spectator:   return QStringLiteral("Spectator");
    case PackageKind::Alarm:       return QStringLiteral("Alarm");
    case PackageKind::Activate:    return QStringLiteral("Activate");
    case PackageKind::Sandbox:     return QStringLiteral("Sandbox");
    case PackageKind::Patrol:      return QStringLiteral("Patrol");
    case PackageKind::Dialoque:    return QStringLiteral("Dialoque");
    case PackageKind::UseWeapon:   return QStringLiteral("UseWeapon");
    case PackageKind::Summon:      return QStringLiteral("Summon");
    case PackageKind::ForceGreet:  return QStringLiteral("ForceGreet");
    case PackageKind::Unarmed:     return QStringLiteral("Unarmed");
    case PackageKind::Sit:         return QStringLiteral("Sit");
    case PackageKind::Recoil:      return QStringLiteral("Recoil");
    case PackageKind::Pause:       return QStringLiteral("Pause");
    case PackageKind::Flee:        return QStringLiteral("Flee");
    case PackageKind::Lock:        return QStringLiteral("Lock");
    case PackageKind::Patrol_Alt:  return QStringLiteral("Patrol (alt)");
    case PackageKind::Unknown:
    case PackageKind::NumTypes:    break;
    }
    return QStringLiteral("Unknown");
}

PackageKind packageKindFromU32(quint32 value)
{
    switch (value)
    {
    case 1:  return PackageKind::Find;
    case 2:  return PackageKind::Follow;
    case 3:  return PackageKind::Escort;
    case 4:  return PackageKind::Eat;
    case 5:  return PackageKind::Sleep;
    case 6:  return PackageKind::Wander;
    case 7:  return PackageKind::Travel;
    case 8:  return PackageKind::Accompany;
    case 9:  return PackageKind::UseItemAt;
    case 10: return PackageKind::Ambush;
    case 11: return PackageKind::FleeNonCombat;
    case 12: return PackageKind::CastMagic;
    case 13: return PackageKind::Combat;
    case 14: return PackageKind::Process;
    case 15: return PackageKind::Spectator;
    case 16: return PackageKind::Alarm;
    case 17: return PackageKind::Activate;
    case 18: return PackageKind::Sandbox;
    case 19: return PackageKind::Patrol;
    case 20: return PackageKind::Dialoque;
    case 21: return PackageKind::UseWeapon;
    case 22: return PackageKind::Summon;
    case 23: return PackageKind::ForceGreet;
    case 24: return PackageKind::Unarmed;
    case 25: return PackageKind::Sit;
    case 26: return PackageKind::Recoil;
    case 27: return PackageKind::Pause;
    case 28: return PackageKind::Flee;
    case 29: return PackageKind::Lock;
    case 30: return PackageKind::Patrol_Alt;
    default: return PackageKind::Unknown;
    }
}

bool packageKindIsRoad(quint32 value)
{
    const PackageKind kind = packageKindFromU32(value);
    return kind == PackageKind::Travel || kind == PackageKind::Patrol
        || kind == PackageKind::Patrol_Alt;
}

PackageData decodePackageData(const PackageRecord& record, bool* ok)
{
    PackageData data;
    bool okOut = false;
    do
    {
        if (record.pkdtRaws.isEmpty())
        {
            break;
        }
        const QByteArray bytes = record.pkdtRaws.constFirst();
        if (bytes.size() < kPkdtCommonSize)
        {
            break;
        }
        quint32 type = 0;
        quint32 flags = 0;
        if (!readU32At(bytes, 0, &type) || !readU32At(bytes, 4, &flags))
        {
            break;
        }
        data.type = type;
        data.flags = flags;
        okOut = true;

        // A third u32 slot only exists in Oblivion/Fallout payloads. Reading
        // it as an interrupt override is version-dependent, so the code
        // records its presence rather than asserting its meaning.
        quint32 third = 0;
        if (readU32At(bytes, 8, &third))
        {
            data.interruptOverride = third;
            data.hasInterruptOverride = third != 0;
        }

        quint8 month = kUnusedMonth;
        quint8 weekday = kUnusedWeekday;
        quint8 date = kUnusedDate;
        quint8 hour = kAnyHour;
        const bool anySchedule = readU8At(bytes, kScheduleOffset, &month)
            && readU8At(bytes, kScheduleOffset + 1, &weekday)
            && readU8At(bytes, kScheduleOffset + 2, &date)
            && readU8At(bytes, kScheduleOffset + 3, &hour);
        if (anySchedule)
        {
            data.schedule.month = month;
            data.schedule.weekday = weekday;
            data.schedule.date = date;
            data.schedule.hour = hour;
        }

        quint8 minuteRaw = 0;
        if (readU8At(bytes, kMinuteOffset, &minuteRaw))
        {
            // 0xFF is the "any minute" sentinel; anything below is a minute
            // value the engine waits for.
            data.schedule.minute =
                minuteRaw == kAnyMinute ? -1 : static_cast<int>(minuteRaw);
        }
        quint8 doAll = 0;
        if (readU8At(bytes, kDoAllOffset, &doAll))
        {
            data.doAll = doAll != 0;
        }

        data.parameters = record.parameters;
    } while (false);

    if (ok)
    {
        *ok = okOut;
    }
    return data;
}

void encodePackageData(PackageRecord& record, const PackageData& data)
{
    if (record.pkdtRaws.isEmpty())
    {
        // A package with no payload yet gets the smallest buffer that can
        // hold every field the editor owns.
        QByteArray bytes(kDoAllOffset + 1, '\0');
        writeU32At(bytes, 0, data.type);
        writeU32At(bytes, 4, data.flags);
        writeU32At(bytes, 8, data.interruptOverride);
        bytes[kScheduleOffset] = static_cast<char>(data.schedule.month);
        bytes[kScheduleOffset + 1] = static_cast<char>(data.schedule.weekday);
        bytes[kScheduleOffset + 2] = static_cast<char>(data.schedule.date);
        bytes[kScheduleOffset + 3] = static_cast<char>(data.schedule.hour);
        // Only the bytes the model owns are written; anything past kDoAllOffset
        // belongs to the game and is left exactly as it was found.
        bytes[kMinuteOffset] = static_cast<char>(
            data.schedule.minute < 0 ? kAnyMinute : data.schedule.minute);
        bytes[kDoAllOffset] = static_cast<char>(data.doAll ? 1 : 0);
        record.pkdtRaws.append(bytes);
        record.pkdtRaw = bytes;
        record.hasPkdt = true;
        return;
    }

    QByteArray bytes = record.pkdtRaws.constFirst();
    const int needed = kDoAllOffset + 1;
    if (bytes.size() < needed)
    {
        bytes.resize(needed);
    }
    writeU32At(bytes, 0, data.type);
    writeU32At(bytes, 4, data.flags);
    writeU32At(bytes, 8, data.interruptOverride);
    bytes[kScheduleOffset] = static_cast<char>(data.schedule.month);
    bytes[kScheduleOffset + 1] = static_cast<char>(data.schedule.weekday);
    bytes[kScheduleOffset + 2] = static_cast<char>(data.schedule.date);
    bytes[kScheduleOffset + 3] = static_cast<char>(data.schedule.hour);
    // Only the bytes the model owns are written; anything past kDoAllOffset
    // belongs to the game and is left exactly as it was found.
    bytes[kMinuteOffset] = static_cast<char>(
        data.schedule.minute < 0 ? kAnyMinute : data.schedule.minute);
    bytes[kDoAllOffset] = static_cast<char>(data.doAll ? 1 : 0);
    // Later payloads keep their bytes untouched: a multi-entry package must
    // still round-trip exactly.
    record.pkdtRaws[0] = bytes;
    record.pkdtRaw = bytes;
    record.hasPkdt = true;
}

} // namespace openck
