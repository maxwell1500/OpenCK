#pragma once

#include "../world/ckid.hpp"
#include "../world/record.hpp"
#include <QVector>
#include <QString>

// Forward declarations for record types
struct NpcRecord;
struct WeaponRecord;
struct SpellRecord;
struct ArmorRecord;
struct EnchRecord;
struct AlchRecord;
struct QuestRecord;
struct FactRecord;
struct CellRecord;
struct RefrRecord;
struct LocationRecord;
struct RaceRecord;
struct ClassRecord;
struct GlobalVariable;
struct PackageRecord;
struct MagicRecord;
struct MaterialRecord;
struct DialRecord;
struct InfoRecord;
struct WorldspaceRecord;
struct BookRecord;
struct IngrRecord;
struct MiscRecord;
struct ActiRecord;
struct StatRecord;
struct PerkRecord;
struct ContRecord;
struct TreeRecord;
struct LocationRefType;

class Data;

class ColumnValidator
{
public:
    enum class Severity { Info, Warning, Error };

    struct ValidationResult
    {
        Severity severity;
        QString field;
        QString message;
    };

    static QVector<ValidationResult> validateNpc(const NpcRecord& npc, Data* data, const QString& originalEditorId = "");
    static QVector<ValidationResult> validateWeapon(const WeaponRecord& weapon, Data* data, const QString& originalEditorId = "");
    static QVector<ValidationResult> validateSpell(const SpellRecord& spell, Data* data, const QString& originalEditorId = "");
    static QVector<ValidationResult> validateArmor(const ArmorRecord& armor, Data* data, const QString& originalEditorId = "");
    static QVector<ValidationResult> validateEnch(const EnchRecord& ench, Data* data, const QString& originalEditorId = "");
    static QVector<ValidationResult> validateAlch(const AlchRecord& alch, Data* data, const QString& originalEditorId = "");

    static QVector<ValidationResult> validateQuest(const QuestRecord& quest, Data* data);
    static QVector<ValidationResult> validateFact(const FactRecord& fact, Data* data);
    static QVector<ValidationResult> validateCell(const CellRecord& cell, Data* data);
    static QVector<ValidationResult> validateRef(const RefrRecord& ref, Data* data);
    static QVector<ValidationResult> validateLocation(const LocationRecord& loc, Data* data);
    static QVector<ValidationResult> validateRace(const RaceRecord& race, Data* data);
    static QVector<ValidationResult> validateClass(const ClassRecord& cls, Data* data);
    static QVector<ValidationResult> validateGlobal(const GlobalVariable& glob, Data* data);
    static QVector<ValidationResult> validatePackage(const PackageRecord& pkg, Data* data);
    static QVector<ValidationResult> validateMagic(const MagicRecord& magic, Data* data);
    static QVector<ValidationResult> validateMaterial(const MaterialRecord& mat, Data* data);
    static QVector<ValidationResult> validateDial(const DialRecord& dial, Data* data);
    static QVector<ValidationResult> validateInfo(const InfoRecord& info, Data* data);
    static QVector<ValidationResult> validateWorldspace(const WorldspaceRecord& ws, Data* data);
    static QVector<ValidationResult> validateBook(const BookRecord& book, Data* data);
    static QVector<ValidationResult> validateIngr(const IngrRecord& ingr, Data* data);
    static QVector<ValidationResult> validateMisc(const MiscRecord& misc, Data* data);
    static QVector<ValidationResult> validateActi(const ActiRecord& acti, Data* data);
    static QVector<ValidationResult> validateStat(const StatRecord& stat, Data* data);
    static QVector<ValidationResult> validatePerk(const PerkRecord& perk, Data* data);
    static QVector<ValidationResult> validateCont(const ContRecord& cont, Data* data);
    static QVector<ValidationResult> validateTree(const TreeRecord& tree, Data* data);
    static QVector<ValidationResult> validateLcrt(const LocationRefType& lcrt, Data* data);

private:
    static ValidationResult makeResult(Severity severity, const QString& field, const QString& message);

    static ValidationResult validateEditorId(const QString& editorId,
                                              const QString& currentId,
                                              Data* data,
                                              CkId::Type type);
};
