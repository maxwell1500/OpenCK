#ifndef RECORDMIGRATOR_HPP
#define RECORDMIGRATOR_HPP

#include "../../../libs/files/esm/gameformat.hpp"

#include <QString>
#include <QVector>

class Data;

// Cross-game record migration (Phase 9.2): copies records from one loaded
// plugin into another game's plugin, carrying only what both games' record
// registries share.
//
// The TES4 family (Oblivion/Skyrim/FO4/Starfield) shares a core record set
// and subrecord grammar, while each game adds specific types (Skyrim's
// MATT/CLMT, FO4's ASRC/LTEX, Starfield's, ...). A record whose code the
// destination does not support is reported and skipped rather than copied
// under a guess: writing subrecords the destination never reads is data
// corruption, not migration.
//
// Imported records go through the destination's own collections, so their
// FormIDs are re-assigned in the destination's space; the caller can then
// run FormIdCompactor to pack them into the local range.
struct MigratedRecord
{
    GameFormat::Game from = GameFormat::Game::Unknown;
    GameFormat::Game to = GameFormat::Game::Unknown;
    QString recordCode;    // e.g. "STAT"
    QString editorId;      // the id the record landed under in the destination
    QString sourceEditorId;
    quint32 sourceFormId = 0;
};

struct MigrationReport
{
    GameFormat::Game from = GameFormat::Game::Unknown;
    GameFormat::Game to = GameFormat::Game::Unknown;
    QVector<MigratedRecord> migrated;
    // "<recordCode> <editorId>: reason" for every record not copied.
    QStringList skipped;
    // Editor IDs that collided in the destination and were suffixed.
    int renamed = 0;

    bool ok() const { return !migrated.isEmpty(); }
};

class RecordMigrator
{
public:
    /// Migrates every record whose code both games support. Returns the
    /// report; per-record outcomes are in migrated/skipped.
    static MigrationReport migrate(Data& source, Data& dest);

    /// Same, restricted to a set of 4-char record codes ("STAT", "NPC_",
    /// ...). Unknown or unsupported codes are reported as skipped.
    static MigrationReport migrate(Data& source, Data& dest,
                                   const QStringList& codes);

    /// The record codes both games support, sorted. Empty for a Morrowind
    /// (TES3) side: its records use the generic Tes3Record path, which this
    /// migrator does not translate.
    static QVector<QString> sharedRecordCodes(GameFormat::Game from,
                                              GameFormat::Game to);
};

#endif // RECORDMIGRATOR_HPP
