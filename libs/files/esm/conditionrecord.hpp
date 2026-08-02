#ifndef CONDITIONRECORD_H
#define CONDITIONRECORD_H

#include <QByteArray>
#include <QString>
#include <QVector>

// CtdaConditions models the CTDA condition subrecords shared by PACK, SCEN,
// DIAL, INFO, QUST and other records. Each condition picks a comparison
// operator, a game function (by function index), two function parameters, a
// run-on target and a reference form ID; the flags carry the OR/AND
// join and use-alias behavior. Conditions are packed/parsed byte-exactly so
// the editor can round-trip real plugins without loss.
struct CtdaCondition
{
    enum class Comparison
    {
        EqualTo = 0,
        NotEqualTo = 1,
        GreaterThan = 2,
        GreaterThanOrEqualTo = 3,
        LessThan = 4,
        LessThanOrEqualTo = 5
    };

    enum class RunOn
    {
        Subject = 0,
        Target = 1,
        Reference = 2,
        CombatTarget = 3,
        LinkedReference = 4,
        QuestAlias = 5,
        PackageData = 6,
        EventData = 7,
        CommandTarget = 8
    };

    Comparison comparison = Comparison::EqualTo;
    quint8 flags = 0;          // bit0 = OR join, bit2 = use aliases, bit3 = use pack data
    quint32 functionId = 0;
    quint32 param1 = 0;        // first function parameter (e.g. actor value / global)
    quint32 param2 = 0;        // second function parameter
    RunOn runOn = RunOn::Subject;
    quint32 reference = 0;     // form ID the condition runs on (0 = none)
    quint32 unk1 = 0;
    quint32 unk2 = 0;

    // Extended fields present in the 36-byte layout (Fallout 4 / Starfield).
    quint32 unk3 = 0;
    quint32 unk4 = 0;

    bool useOr() const { return (flags & 0x01) != 0; }
    void setUseOr(bool orJoin) { flags = orJoin ? (flags | 0x01) : (flags & ~0x01); }

    static QString comparisonName(Comparison comparison);
    static QString runOnName(RunOn runOn);

    // Byte layout: the 28-byte Skyrim form, or 36-byte FO4/SF form when
    // extendedBytes is true. Fixed 28 bytes is assumed for parsing when the
    // payload is exactly 28; 36-byte payloads set extendedBytes.
    bool extendedBytes = false;

    // Packs to the 28- or 36-byte binary layout.
    QByteArray pack() const;

    // Parses a single condition from the payload. Accepts 28- or 36-byte
    // inputs; returns false for other sizes.
    static bool unpack(const QByteArray& bytes, CtdaCondition& out);

    // Loads a counted CTDA list (header count + packed conditions).
    static QVector<CtdaCondition> unpackList(const QByteArray& bytes);
    static QByteArray packList(const QVector<CtdaCondition>& conditions);
};

inline bool operator==(const CtdaCondition& l, const CtdaCondition& r)
{
    return l.comparison == r.comparison && l.flags == r.flags
        && l.functionId == r.functionId && l.param1 == r.param1
        && l.param2 == r.param2 && l.runOn == r.runOn
        && l.reference == r.reference && l.unk1 == r.unk1 && l.unk2 == r.unk2
        && l.unk3 == r.unk3 && l.unk4 == r.unk4
        && l.extendedBytes == r.extendedBytes;
}

#endif // CONDITIONRECORD_H
