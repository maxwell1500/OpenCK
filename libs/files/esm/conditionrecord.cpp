#include "conditionrecord.hpp"

#include <cstring>

namespace
{
constexpr int kSkyrimBytes = 20;   // real Skyrim CTDA short form
constexpr int kBaseBytes = 28;     // editor layout with reference + unk1/unk2
constexpr int kFalloutBytes = 32;  // real Fallout 4 / Starfield CTDA
constexpr int kExtendedBytes = 36; // editor extended layout
constexpr int kHeaderBytes = 4;    // count prefix of a packed CTDA list
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
    if (!raw.isEmpty())
        return raw;

    // Editor layout: 28 bytes through unk1, 36 bytes adding unk2/unk3.
    // Unk4 has no byte slot in either layout; it is only preserved via
    // raw when a 40-byte payload was parsed.
    const int size = extendedBytes ? kExtendedBytes : kBaseBytes;
    QByteArray bytes(size, Qt::Uninitialized);
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
    if (size >= 32)
        memcpy(p + 28, &unk2, 4);
    if (size >= 36)
        memcpy(p + 32, &unk3, 4);
    return bytes;
}

bool CtdaCondition::unpack(const QByteArray& bytes, CtdaCondition& out)
{
    const int size = bytes.size();
    if (size != kSkyrimBytes && size != kBaseBytes
        && size != kFalloutBytes && size != kExtendedBytes)
        return false;

    const quint8* p = reinterpret_cast<const quint8*>(bytes.constData());
    out.comparison = static_cast<Comparison>(p[0] & 0x07);
    out.flags = p[2];
    out.functionId = 0;
    out.param1 = 0;
    out.param2 = 0;
    out.runOn = RunOn::Subject;
    out.reference = 0;
    out.unk1 = 0;
    out.unk2 = 0;
    out.unk3 = 0;
    out.unk4 = 0;

    if (size >= 8)  memcpy(&out.functionId, p + 4, 4);
    if (size >= 12) memcpy(&out.param1, p + 8, 4);
    if (size >= 16) memcpy(&out.param2, p + 12, 4);
    if (size >= 20) memcpy(&out.runOn, p + 16, 4);
    if (size >= 24) memcpy(&out.reference, p + 20, 4);
    if (size >= 28) memcpy(&out.unk1, p + 24, 4);
    if (size >= 32) memcpy(&out.unk2, p + 28, 4);

    out.extendedBytes = (size == kExtendedBytes);
    if (out.extendedBytes)
    {
        memcpy(&out.unk3, p + 32, 4);
    }

    out.raw = bytes;
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
    // Lists are uniform within a record: pick the stride that divides the
    // payload exactly, preferring the editor/FO4 sizes.
    const int strides[] = { kExtendedBytes, kBaseBytes, kFalloutBytes, kSkyrimBytes };
    int stride = 0;
    for (int s : strides)
    {
        if (payload > 0 && payload % s == 0)
        {
            stride = s;
            break;
        }
    }
    if (stride == 0)
        stride = kBaseBytes;
    for (quint32 i = 0; i < count; i++)
    {
        if (offset + stride > bytes.size())
            break;
        CtdaCondition condition;
        if (!unpack(bytes.mid(offset, stride), condition))
            break;
        offset += stride;
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
