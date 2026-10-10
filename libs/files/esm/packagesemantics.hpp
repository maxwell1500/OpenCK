#pragma once

#include <QString>
#include <QVector>
#include <QDateTime>

#include "Packagerecord.hpp"

// Semantic view of an AI package (PACK record). The record layer keeps PKDT
// payloads verbatim so an unedited plugin round-trips byte-for-byte; this
// header decodes the fields editors and validators actually need, and
// re-encodes them without disturbing the bytes the editor never touched.
//
// Package layout reference (Skyrim/Fallout, little-endian):
//   PKDT: u32 type | u32 flags | u32 flags-or-interrupt-override, then a
//         per-type union whose shape depends on `type`.
//   PLDT: u32 target type | u32 target-count-ish data ...
//   PTDT: a target reference per entry.
namespace openck
{

/// The package families a PACK can declare. Values are the on-disk enum.
enum class PackageKind
{
    Unknown = 0,
    Find,        // 1 - follow a reference
    Follow,      // 2 - follow a reference (Skyrim: Follow NPC)
    Escort,      // 3 - escort a reference
    Eat,         // 4 - eat at a reference
    Sleep,       // 5 - sleep at a reference
    Wander,      // 6 - wander around a location
    Travel,      // 7 - travel between locations
    Accompany,   // 8 - accompany a reference
    UseItemAt,   // 9 - use an item at a reference
    Ambush,      // 10
    FleeNonCombat, // 11
    CastMagic,   // 12 - cast a spell at a reference
    Combat,      // 13 - standard combat behaviour
    Process,     // 14 - use idle marker(s) in an area
    Spectator,   // 15
    Alarm,       // 16
    Activate,    // 17 - activate a reference
    Sandbox,     // 18 - sandbox behaviour
    Patrol,      // 19 - patrol a route
    Dialoque,    // 20 (Skyrim) - dialogue-enabled waiting/following
    UseWeapon,   // 21
    Summon,      // 22
    ForceGreet,  // 23
    Unarmed,     // 24
    Sit,         // 25 - sit at a reference
    Recoil,      // 26
    Pause,       // 27
    Flee,        // 28
    Lock,        // 29
    Patrol_Alt,  // 30
    NumTypes
};

QString packageKindName(PackageKind kind);
PackageKind packageKindFromU32(quint32 value);
bool packageKindIsRoad(quint32 value);   // travel/patrol use XALG road data

/// A decoded PKDT payload: the common header plus the union members that have
/// a stable meaning across games.
struct PackageSchedule
{
    // The sentinels below are what the engine writes for "unconstrained";
    // defaulting to them means a package with no schedule gate reads as one.
    quint8 month = 0xFF;      // 0xFF = ignore
    quint8 weekday = 0xFF;    // 0xFF = ignore
    quint8 date = 0xFF;       // 0xFF = ignore
    quint8 hour = 0xFF;       // 0xFF = ignore
    int minute = -1;          // -1 = ignore, 0..119 = timer value
};

struct PackageData
{
    quint32 type = 0;
    quint32 flags = 0;
    /// True when the third u32 slot holds an interrupt override rather than
    /// package flags (oblivion/skyrim differ by version).
    bool hasInterruptOverride = false;
    quint32 interruptOverride = 0;
    PackageSchedule schedule;
    QVector<quint32> parameters;
    bool doAll = false;               // "Perform all" flag (Skyrim: once/top)
    int randomIndex = -1;             // currently unused; reserved for -1
    quint32 patrolFlags = 0;
};

/// Decodes the first PKDT payload. Returns false (and fills `ok` false) when
/// the blob is too short to hold the common header.
PackageData decodePackageData(const PackageRecord& record, bool* ok = nullptr);

/// A single schedule gate the engine evaluates before a procedure runs.
struct ScheduleCheck
{
    QString name;      // human label, e.g. "Tuesday"
    bool active = false; // active = this constraint is in force right now
};

/// True when the schedule constraint is set (an explicit month/weekday/date/
/// hour rather than the "any" sentinel).
bool scheduleConstraintSet(const PackageSchedule& schedule);

/// Resolves a decoded schedule into the constraints the engine would apply,
/// for editor display. `now`, when valid, is used to mark which are active.
QVector<ScheduleCheck> scheduleChecks(const PackageSchedule& schedule,
                                      const QDateTime& now = QDateTime());

/// Warnings for a package's decoded data (missing target for a type that
/// requires one, road data expected for travel/patrol, and so on).
enum class PackageIssueSeverity
{
    Warning,
    Error
};
struct PackageIssue
{
    PackageIssueSeverity severity = PackageIssueSeverity::Warning;
    QString message;
};
QVector<PackageIssue> validatePackageData(const PackageRecord& record,
                                          const PackageData& data);

/// Re-encodes `data` into `record`, preserving every payload byte the editor
/// does not own and appending a new PKDT when the record had none.
void encodePackageData(PackageRecord& record, const PackageData& data);

} // namespace openck
