#include "conditionrecord.hpp"

#include <cstring>

namespace
{
constexpr int kBaseBytes = 28;
constexpr int kExtendedBytes = 36;
constexpr int kHeaderBytes = 4;
}

QString CtdaCondition::comparisonName(Comparison comparison)
{
    switch (comparison)
    {
    case Comparison::EqualTo:                  return QStringLiteral("==");
    case Comparison::NotEqualTo:               return QStringLiteral("!=");
    case Comparison::GreaterThan:              return QStringLiteral(">");
    case Comparison::GreaterThanOrEqualTo:     return QStringLiteral(">=");
    case Comparison::LessThan:                 return QStringLiteral("<");
    case Comparison::LessThanOrEqualTo:        return QStringLiteral("<=");
    }
    return QStringLiteral("==");
}

QString CtdaCondition::runOnName(RunOn runOn)
{
    switch (runOn)
    {
    case RunOn::Subject:          return QStringLiteral("Subject");
    case RunOn::Target:           return QStringLiteral("Target");
    case RunOn::Reference:        return QStringLiteral("Reference");
    case RunOn::CombatTarget:     return QStringLiteral("Combat Target");
    case RunOn::LinkedReference:  return QStringLiteral("Linked Reference");
    case RunOn::QuestAlias:       return QStringLiteral("Quest Alias");
    case RunOn::PackageData:      return QStringLiteral("Package Data");
    case RunOn::EventData:        return QStringLiteral("Event Data");
    case RunOn::CommandTarget:    return QStringLiteral("Command Target");
    }
    return QStringLiteral("Subject");
}

QByteArray CtdaCondition::pack() const
{
    QByteArray bytes(extendedBytes ? kExtendedBytes : kBaseBytes, Qt::Uninitialized);
    quint8* p = reinterpret_cast<quint8*>(bytes.data());
    p[0] = static_cast<quint8>(comparison);
    p[1] = 0;
    p[2] = flags;
    p[3] = 0;
    memcpy(p + 4, &functionId, 4);
    memcpy(p + 8, &param1, 4);
    memcpy(p + 12, &param2, 4);
    memcpy(p + 16, &runOn, 4);
    memcpy(p + 20, &reference, 4);
    memcpy(p + 24, &unk1, 4);
    memcpy(p + 28, &unk2, 4);
    if (extendedBytes)
    {
        memcpy(p + 32, &unk3, 4);
        memcpy(p + 36, &unk4, 4);
    }
    return bytes;
}

bool CtdaCondition::unpack(const QByteArray& bytes, CtdaCondition& out)
{
    const int size = bytes.size();
    if (size != kBaseBytes && size != kExtendedBytes)
        return false;

    const quint8* p = reinterpret_cast<const quint8*>(bytes.constData());
    out.comparison = static_cast<Comparison>(p[0] & 0x07);
    out.flags = p[2];
    memcpy(&out.functionId, p + 4, 4);
    memcpy(&out.param1, p + 8, 4);
    memcpy(&out.param2, p + 12, 4);
    memcpy(&out.runOn, p + 16, 4);
    memcpy(&out.reference, p + 20, 4);
    memcpy(&out.unk1, p + 24, 4);
    memcpy(&out.unk2, p + 28, 4);
    out.extendedBytes = (size == kExtendedBytes);
    if (out.extendedBytes)
    {
        memcpy(&out.unk3, p + 32, 4);
        memcpy(&out.unk4, p + 36, 4);
    }
    return true;
}

QVector<CtdaCondition> CtdaCondition::unpackList(const QByteArray& bytes)
{
    QVector<CtdaCondition> conditions;
    if (bytes.size() < kHeaderBytes)
        return conditions;

    const quint8* p = reinterpret_cast<const quint8*>(bytes.constData());
    quint32 count = 0;
    memcpy(&count, p, 4);

    int offset = kHeaderBytes;
    const int payload = bytes.size() - kHeaderBytes;
    // Lists are uniform within a record: choose the stride that divides the
    // payload exactly (36-byte FO4/SF, else 28-byte Skyrim).
    const int stride = (payload > 0 && payload % kExtendedBytes == 0)
        ? kExtendedBytes : kBaseBytes;
    for (quint32 i = 0; i < count; i++)
    {
        if (offset + stride > bytes.size())
            break;
        CtdaCondition condition;
        if (!unpack(bytes.mid(offset, stride), condition))
            break;
        offset += condition.extendedBytes ? kExtendedBytes : kBaseBytes;
        conditions.append(condition);
    }
    return conditions;
}

QByteArray CtdaCondition::packList(const QVector<CtdaCondition>& conditions)
{
    QByteArray bytes;
    const quint32 count = static_cast<quint32>(conditions.size());
    bytes.append(reinterpret_cast<const char*>(&count), 4);
    for (const CtdaCondition& condition : conditions)
        bytes.append(condition.pack());
    return bytes;
}
