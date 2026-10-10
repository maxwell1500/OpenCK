#include "recordmigrator.hpp"
#include "logger.hpp"

#include "../../../libs/files/esm/gameformat.hpp"
#include "../world/collection.hpp"
#include "../world/data.hpp"

#include <QFileInfo>

namespace {

// Codes whose typed record the TES4 family shares. The registry in
// GameFormat decides the final per-game compatibility; this list only says
// which CkId::Type the migrator knows how to move.
struct CodeType
{
    const char* code;
    CkId::Type type;
};

const QVector<CodeType>& migratableTypes()
{
    static const QVector<CodeType> types = {
        { "GMST", CkId::Type_Gmst },
        { "NPC_", CkId::Type_Npc_ },
        { "WEAP", CkId::Type_Weap_ },
        { "ARMO", CkId::Type_Armor_ },
        { "SPEL", CkId::Type_Spel_ },
        { "MGEF", CkId::Type_Magic_ },
        { "QUST", CkId::Type_Quest_ },
        { "DIAL", CkId::Type_Dial_ },
        { "INFO", CkId::Type_Info_ },
        { "GLOB", CkId::Type_Glob_ },
        { "LCRT", CkId::Type_Lcrt_ },
        { "PACK", CkId::Type_Pack_ },
        { "TREE", CkId::Type_Tree_ },
        { "ALCH", CkId::Type_Alch_ },
        { "INGR", CkId::Type_Ingr_ },
        { "CONT", CkId::Type_Cont_ },
        { "ENCH", CkId::Type_Ench_ },
        { "BOOK", CkId::Type_Book_ },
        { "MISC", CkId::Type_Misc_ },
        { "ACTI", CkId::Type_Acti_ },
        { "STAT", CkId::Type_Stat_ },
        { "RACE", CkId::Type_Race_ },
        { "CLAS", CkId::Type_Class_ },
        { "FACT", CkId::Type_Fact_ },
        { "PERK", CkId::Type_PerK_ },
        { "CELL", CkId::Type_Cel_ },
        { "WRLD", CkId::Type_WRLD_ },
    };
    return types;
}

QString codeOf(CkId::Type type)
{
    const NAME name = Data::recordNameForType(type);
    if (name == 0)
    {
        return QString();
    }
    QString out;
    out += QChar((name >> 24) & 0xFF);
    out += QChar((name >> 16) & 0xFF);
    out += QChar((name >> 8) & 0xFF);
    out += QChar(name & 0xFF);
    return out;
}

template <typename Rec>
void migrateCollection(Collection<Rec>& src, Collection<Rec>& dst,
                       CkId::Type type, const QString& code,
                       MigrationReport& report)
{
    const int count = src.size();
    for (int i = 0; i < count; ++i)
    {
        const QString sourceId = src.getId(i);
        if (sourceId.isEmpty())
        {
            report.skipped.append(
                QString("%1 (unnamed): record carries no EditorID").arg(code));
            continue;
        }

        // The destination keys records by lowercased EditorID, so a name
        // that already exists there gets a suffix instead of overwriting.
        QString destId = sourceId;
        int suffix = 1;
        while (dst.searchId(destId) != -1)
        {
            destId = QString("%1_migrated%2").arg(sourceId).arg(suffix++);
            ++report.renamed;
        }

        // Deep-copy through the record layer, then append under the resolved
        // id: the destination's collection assigns its own FormID.
        std::unique_ptr<BaseRecord> copy = src.cloneRecordAt(i);
        if (!copy)
        {
            report.skipped.append(
                QString("%1 %2: record could not be copied").arg(code, sourceId));
            continue;
        }

        try
        {
            Record<Rec>& record = dynamic_cast<Record<Rec>&>(*copy);
            IdAccessor<Rec>().setId(record.get(), destId);
            dst.appendRecord(record, type);
        }
        catch (const std::exception& e)
        {
            report.skipped.append(
                QString("%1 %2: %3").arg(code, sourceId, QString::fromUtf8(e.what())));
            continue;
        }

        MigratedRecord migrated;
        migrated.recordCode = code;
        migrated.editorId = destId;
        migrated.sourceEditorId = sourceId;
        migrated.sourceFormId = src.getFormId(i);
        report.migrated.append(migrated);
    }
}

} // namespace

QVector<QString> RecordMigrator::sharedRecordCodes(GameFormat::Game from,
                                                   GameFormat::Game to)
{
    QVector<QString> codes;

    // The TES4 family (Oblivion/Skyrim/FO4/Starfield) shares one typed
    // record core and subrecord grammar, so every code in migratableTypes()
    // moves between them. Game-*specific* codes (Skyrim's MATT, FO4's ASRC,
    // Starfield's SHOU, ...) are deliberately not in that list: they would
    // need per-game subrecord translation the destination never reads.
    // Morrowind (TES3) keeps its records in the generic Tes3Record path,
    // which this migrator does not translate, so a TES3 side shares nothing.
    auto isTes4 = [](GameFormat::Game game)
    {
        return game == GameFormat::Game::Oblivion
            || game == GameFormat::Game::Skyrim
            || game == GameFormat::Game::Fallout4
            || game == GameFormat::Game::Starfield;
    };
    if (!isTes4(from) || !isTes4(to))
    {
        return codes;
    }

    for (const CodeType& entry : migratableTypes())
    {
        const QString code = codeOf(entry.type);
        if (!code.isEmpty())
        {
            codes.append(code);
        }
    }
    return codes;
}

MigrationReport RecordMigrator::migrate(Data& source, Data& dest,
                                        const QStringList& codes)
{
    MigrationReport report;
    report.from = source.currentGame();
    report.to = dest.currentGame();

    const QVector<QString> shared = sharedRecordCodes(report.from, report.to);
    const QStringList wanted =
        codes.isEmpty() ? QStringList(shared.begin(), shared.end()) : codes;

    for (const QString& code : wanted)
    {
        // Find the type for this code from the migratable list; unknown
        // codes are reported, never guessed at.
        CkId::Type type = CkId::Type_None;
        for (const CodeType& entry : migratableTypes())
        {
            if (codeOf(entry.type).compare(code, Qt::CaseInsensitive) == 0)
            {
                type = entry.type;
                break;
            }
        }
        if (type == CkId::Type_None)
        {
            report.skipped.append(
                QString("%1: not a migratable record code").arg(code));
            continue;
        }
        if (!codes.isEmpty() && !shared.contains(code, Qt::CaseInsensitive))
        {
            report.skipped.append(
                QString("%1: not shared between %2 and %3")
                    .arg(code, GameFormat::gameName(report.from),
                         GameFormat::gameName(report.to)));
            continue;
        }

        BaseCollection* sourceCol = source.getCollectionByType(type);
        BaseCollection* destCol = dest.getCollectionByType(type);
        if (!sourceCol || !destCol)
        {
            report.skipped.append(
                QString("%1: no collection on one side").arg(code));
            continue;
        }

        // The typed records are shared C++ types across the TES4 family, so
        // a collection of the same CkId::Type can receive them directly.
        switch (type)
        {
#define OPENCK_MIGRATE(caseLabel, recType)                                    \
    case caseLabel:                                                           \
        migrateCollection<recType>(                                           \
            *static_cast<Collection<recType>*>(sourceCol),                    \
            *static_cast<Collection<recType>*>(destCol), type, code, report); \
        break;
            OPENCK_MIGRATE(CkId::Type_Gmst, GameSetting)
            OPENCK_MIGRATE(CkId::Type_Npc_, NpcRecord)
            OPENCK_MIGRATE(CkId::Type_Weap_, WeaponRecord)
            OPENCK_MIGRATE(CkId::Type_Armor_, ArmorRecord)
            OPENCK_MIGRATE(CkId::Type_Spel_, SpellRecord)
            OPENCK_MIGRATE(CkId::Type_Magic_, MagicRecord)
            OPENCK_MIGRATE(CkId::Type_Quest_, QuestRecord)
            OPENCK_MIGRATE(CkId::Type_Dial_, DialRecord)
            OPENCK_MIGRATE(CkId::Type_Info_, InfoRecord)
            OPENCK_MIGRATE(CkId::Type_Glob_, GlobalVariable)
            OPENCK_MIGRATE(CkId::Type_Lcrt_, LocationRefType)
            OPENCK_MIGRATE(CkId::Type_Pack_, PackageRecord)
            OPENCK_MIGRATE(CkId::Type_Tree_, TreeRecord)
            OPENCK_MIGRATE(CkId::Type_Alch_, AlchRecord)
            OPENCK_MIGRATE(CkId::Type_Ingr_, IngrRecord)
            OPENCK_MIGRATE(CkId::Type_Cont_, ContRecord)
            OPENCK_MIGRATE(CkId::Type_Ench_, EnchRecord)
            OPENCK_MIGRATE(CkId::Type_Book_, BookRecord)
            OPENCK_MIGRATE(CkId::Type_Misc_, MiscRecord)
            OPENCK_MIGRATE(CkId::Type_Acti_, ActiRecord)
            OPENCK_MIGRATE(CkId::Type_Stat_, StatRecord)
            OPENCK_MIGRATE(CkId::Type_Race_, RaceRecord)
            OPENCK_MIGRATE(CkId::Type_Class_, ClassRecord)
            OPENCK_MIGRATE(CkId::Type_Fact_, FactRecord)
            OPENCK_MIGRATE(CkId::Type_PerK_, PerkRecord)
            OPENCK_MIGRATE(CkId::Type_Cel_, CellRecord)
            OPENCK_MIGRATE(CkId::Type_WRLD_, WorldspaceRecord)
#undef OPENCK_MIGRATE
        default:
            report.skipped.append(
                QString("%1: record copy not wired for this type").arg(code));
            break;
        }
    }
    return report;
}

MigrationReport RecordMigrator::migrate(Data& source, Data& dest)
{
    return migrate(source, dest, QStringList());
}
