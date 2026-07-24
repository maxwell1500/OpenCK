#include "columnvalidator.hpp"
#include "../world/collection.hpp"
#include "../world/idcollection.hpp"
#include "../world/basecollection.hpp"
#include "../world/data.hpp"
#include "logger.hpp"

#include <QMessageBox>

ColumnValidator::ValidationResult ColumnValidator::makeResult(Severity severity, const QString& field, const QString& message)
{
    return ValidationResult{severity, field, message};
}

ColumnValidator::ValidationResult ColumnValidator::validateEditorId(const QString& editorId,
                                                                     const QString& currentId,
                                                                     Data* data,
                                                                     CkId::Type type)
{
    if (editorId.isEmpty())
        return makeResult(Severity::Error, "EditorID", "Editor ID cannot be empty.");

    if (data)
    {
        const BaseCollection* collection = data->getCollectionByType(type);
        if (collection)
        {
            int idx = collection->searchId(editorId);
            if (idx >= 0 && collection->getId(idx) != currentId)
                return makeResult(Severity::Error, "EditorID",
                                  QString("A record already exists with this Editor ID."));
        }
    }

    return makeResult(Severity::Info, "", "");
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateNpc(const NpcRecord& npc, Data* data, const QString& originalEditorId)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(npc.editorId, originalEditorId, data, CkId::Type_Npc_);
    if (eid.severity != Severity::Info) results.append(eid);

    if (npc.level > 65535)
        results.append(makeResult(Severity::Error, "Level", "Level must be between 0 and 65535."));

    if (npc.race != 0 && !data)
        results.append(makeResult(Severity::Warning, "Race", "Race ID set but cannot validate without Data context."));

    for (quint32 spell : npc.spells)
    {
        if (spell == 0)
            results.append(makeResult(Severity::Warning, "Spells", "Spell list contains zero FormID."));
    }
    for (quint32 item : npc.inventoryItems)
    {
        if (item == 0)
            results.append(makeResult(Severity::Warning, "Inventory", "Inventory contains zero FormID."));
    }

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateWeapon(const WeaponRecord& weapon, Data* data, const QString& originalEditorId)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(weapon.editorId, originalEditorId, data, CkId::Type_Weap_);
    if (eid.severity != Severity::Info) results.append(eid);

    if (weapon.damage > 99999.0f)
        results.append(makeResult(Severity::Error, "Damage", "Damage must be between 0 and 99999."));
    if (weapon.speed < 0.1f || weapon.speed > 10.0f)
        results.append(makeResult(Severity::Warning, "Speed", "Speed is outside recommended range (0.1-10.0)."));
    if (weapon.reach < 0.1f || weapon.reach > 10.0f)
        results.append(makeResult(Severity::Warning, "Reach", "Reach is outside recommended range (0.1-10.0)."));
    if (weapon.weight > 9999.0f)
        results.append(makeResult(Severity::Error, "Weight", "Weight must be between 0 and 9999."));
    if (weapon.value > 999999)
        results.append(makeResult(Severity::Error, "Value", "Value must be between -999999 and 999999."));

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateQuest(const QuestRecord& quest, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(quest.editorId, "", data, CkId::Type_Quest_);
    if (eid.severity != Severity::Info) results.append(eid);

    for (quint32 stage : quest.stageIds)
    {
        if (stage == 0)
            results.append(makeResult(Severity::Warning, "Stages", "Stage list contains zero ID."));
    }
    for (quint32 script : quest.scriptIds)
    {
        if (script == 0)
            results.append(makeResult(Severity::Warning, "Scripts", "Script list contains zero ID."));
    }

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateFact(const FactRecord& fact, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(fact.editorId, "", data, CkId::Type_Fact_);
    if (eid.severity != Severity::Info) results.append(eid);

    if (fact.ranks.isEmpty())
        results.append(makeResult(Severity::Warning, "Ranks", "Facts should have at least one rank defined."));
    if (fact.factionName.isEmpty())
        results.append(makeResult(Severity::Warning, "Faction Name", "Faction name is empty."));

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateCell(const CellRecord& cell, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(cell.editorId, "", data, CkId::Type_Cel_);
    if (eid.severity != Severity::Info) results.append(eid);

    if (cell.lockLevel > 100)
        results.append(makeResult(Severity::Error, "Lock Level", "Lock level must be between 0 and 100."));

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateRef(const RefrRecord& ref, Data* data)
{
    QVector<ValidationResult> results;

    if (ref.posX < -999999.0f || ref.posX > 999999.0f)
        results.append(makeResult(Severity::Warning, "Position X", "X position is outside typical range."));
    if (ref.posY < -999999.0f || ref.posY > 999999.0f)
        results.append(makeResult(Severity::Warning, "Position Y", "Y position is outside typical range."));
    if (ref.posZ < -999999.0f || ref.posZ > 999999.0f)
        results.append(makeResult(Severity::Warning, "Position Z", "Z position is outside typical range."));
    if (ref.rotX < 0.0f || ref.rotX > 360.0f)
        results.append(makeResult(Severity::Warning, "Rotation X", "Rotation X should be between 0 and 360."));
    if (ref.rotY < 0.0f || ref.rotY > 360.0f)
        results.append(makeResult(Severity::Warning, "Rotation Y", "Rotation Y should be between 0 and 360."));
    if (ref.rotZ < 0.0f || ref.rotZ > 360.0f)
        results.append(makeResult(Severity::Warning, "Rotation Z", "Rotation Z should be between 0 and 360."));
    if (ref.scale < 0.0f || ref.scale > 10.0f)
        results.append(makeResult(Severity::Warning, "Scale", "Scale is outside typical range (0-10)."));
    if (ref.lockLevel > 100)
        results.append(makeResult(Severity::Error, "Lock Level", "Lock level must be between 0 and 100."));

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateLocation(const LocationRecord& loc, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(loc.editorId, "", data, CkId::Type_LOCT_);
    if (eid.severity != Severity::Info) results.append(eid);

    if (loc.x < 0 || loc.x > 999999)
        results.append(makeResult(Severity::Warning, "X Coordinate", "X coordinate is outside typical range."));
    if (loc.y < 0 || loc.y > 999999)
        results.append(makeResult(Severity::Warning, "Y Coordinate", "Y coordinate is outside typical range."));
    if (loc.z < 0 || loc.z > 999999)
        results.append(makeResult(Severity::Warning, "Z Coordinate", "Z coordinate is outside typical range."));

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateRace(const RaceRecord& race, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(race.editorId, "", data, CkId::Type_Race_);
    if (eid.severity != Severity::Info) results.append(eid);

    if (race.raceFlags > 9999)
        results.append(makeResult(Severity::Warning, "Race Flags", "Race flags are outside typical range."));

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateClass(const ClassRecord& cls, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(cls.editorId, "", data, CkId::Type_Class_);
    if (eid.severity != Severity::Info) results.append(eid);

    if (cls.serviceFlags > 9999)
        results.append(makeResult(Severity::Warning, "ServiceFlags", "Service flags value is outside typical range."));

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateGlobal(const GlobalVariable& glob, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(glob.editorId, "", data, CkId::Type_Glob_);
    if (eid.severity != Severity::Info) results.append(eid);

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validatePackage(const PackageRecord& pkg, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(pkg.editorId, "", data, CkId::Type_Pack_);
    if (eid.severity != Severity::Info) results.append(eid);

    if (pkg.packageType > 9999)
        results.append(makeResult(Severity::Warning, "Package Type", "Package type is outside typical range."));
    if (pkg.targetType > 9999)
        results.append(makeResult(Severity::Warning, "Target Type", "Target type is outside typical range."));

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateSpell(const SpellRecord& spell, Data* data, const QString& originalEditorId)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(spell.editorId, originalEditorId, data, CkId::Type_Spel_);
    if (eid.severity != Severity::Info) results.append(eid);

    if (spell.cost > 999999)
        results.append(makeResult(Severity::Error, "Cost", "Cost must be between 0 and 999999."));
    if (spell.castingSound > 9999)
        results.append(makeResult(Severity::Warning, "Casting Sound", "Casting sound ID is outside typical range."));

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateMagic(const MagicRecord& magic, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(magic.editorId, "", data, CkId::Type_Magic_);
    if (eid.severity != Severity::Info) results.append(eid);

    if (magic.schools > 9999)
        results.append(makeResult(Severity::Warning, "Schools", "Schools value is outside typical range."));
    if (magic.damageType > 9999)
        results.append(makeResult(Severity::Warning, "Damage Type", "Damage type ID is outside typical range."));

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateArmor(const ArmorRecord& armor, Data* data, const QString& originalEditorId)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(armor.editorId, originalEditorId, data, CkId::Type_Armor_);
    if (eid.severity != Severity::Info) results.append(eid);

    if (armor.armorRating > 9999)
        results.append(makeResult(Severity::Error, "Armor Rating", "Armor rating must be between 0 and 9999."));
    if (armor.weight > 9999.0f)
        results.append(makeResult(Severity::Error, "Weight", "Weight must be between 0 and 9999."));
    if (armor.value > 999999)
        results.append(makeResult(Severity::Error, "Value", "Value must be between -999999 and 999999."));
    if (armor.health > 9999.0f)
        results.append(makeResult(Severity::Error, "Health", "Health must be between 0 and 9999."));

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateMaterial(const MaterialRecord& mat, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(mat.editorId, "", data, CkId::Type_Material_);
    if (eid.severity != Severity::Info) results.append(eid);

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateDial(const DialRecord& dial, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(dial.editorId, "", data, CkId::Type_Dial_);
    if (eid.severity != Severity::Info) results.append(eid);

    for (quint32 id : dial.responseIds)
    {
        if (id == 0)
            results.append(makeResult(Severity::Warning, "Response IDs", "Response list contains zero ID."));
    }

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateInfo(const InfoRecord& info, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(info.editorId, "", data, CkId::Type_Info_);
    if (eid.severity != Severity::Info) results.append(eid);

    if (info.targetId == 0)
        results.append(makeResult(Severity::Warning, "Target ID", "Target ID is zero."));

    for (quint32 id : info.scriptIds)
    {
        if (id == 0)
            results.append(makeResult(Severity::Warning, "Script IDs", "Script list contains zero ID."));
    }

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateWorldspace(const WorldspaceRecord& ws, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(ws.editorId, "", data, CkId::Type_WRLD_);
    if (eid.severity != Severity::Info) results.append(eid);

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateEnch(const EnchRecord& ench, Data* data, const QString& originalEditorId)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(ench.editorId, originalEditorId, data, CkId::Type_Ench_);
    if (eid.severity != Severity::Info) results.append(eid);

    if (ench.costLimit > 99999)
        results.append(makeResult(Severity::Error, "Cost Limit", "Cost limit must be between 0 and 99999."));
    if (ench.charges > 9999)
        results.append(makeResult(Severity::Error, "Charges", "Charges must be between 0 and 9999."));

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateBook(const BookRecord& book, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(book.editorId, "", data, CkId::Type_Book_);
    if (eid.severity != Severity::Info) results.append(eid);

    if (book.pageCount > 99999)
        results.append(makeResult(Severity::Error, "Page Count", "Page count must be between 0 and 99999."));

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateAlch(const AlchRecord& alch, Data* data, const QString& originalEditorId)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(alch.editorId, originalEditorId, data, CkId::Type_Alch_);
    if (eid.severity != Severity::Info) results.append(eid);

    if (alch.weight > 9999.0f)
        results.append(makeResult(Severity::Error, "Weight", "Weight must be between 0 and 9999."));
    if (alch.value > 999999)
        results.append(makeResult(Severity::Error, "Value", "Value must be between -999999 and 999999."));

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateIngr(const IngrRecord& ingr, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(ingr.editorId, "", data, CkId::Type_Ingr_);
    if (eid.severity != Severity::Info) results.append(eid);

    if (ingr.weight > 9999.0f)
        results.append(makeResult(Severity::Error, "Weight", "Weight must be between 0 and 9999."));
    if (ingr.value > 999999)
        results.append(makeResult(Severity::Error, "Value", "Value must be between -999999 and 999999."));

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateMisc(const MiscRecord& misc, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(misc.editorId, "", data, CkId::Type_Misc_);
    if (eid.severity != Severity::Info) results.append(eid);

    if (misc.weight > 9999.0f)
        results.append(makeResult(Severity::Error, "Weight", "Weight must be between 0 and 9999."));
    if (misc.value > 999999)
        results.append(makeResult(Severity::Error, "Value", "Value must be between 0 and 999999."));

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateActi(const ActiRecord& acti, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(acti.editorId, "", data, CkId::Type_Acti_);
    if (eid.severity != Severity::Info) results.append(eid);
    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateStat(const StatRecord& stat, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(stat.editorId, "", data, CkId::Type_Stat_);
    if (eid.severity != Severity::Info) results.append(eid);
    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validatePerk(const PerkRecord& perk, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(perk.editorId, "", data, CkId::Type_PerK_);
    if (eid.severity != Severity::Info) results.append(eid);

    for (quint32 cond : perk.conditions)
    {
        if (cond == 0)
            results.append(makeResult(Severity::Warning, "Conditions", "Conditions list contains zero ID."));
    }

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateCont(const ContRecord& cont, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(cont.editorId, "", data, CkId::Type_Cont_);
    if (eid.severity != Severity::Info) results.append(eid);

    if (cont.contents > 9999)
        results.append(makeResult(Severity::Warning, "Contents", "Contents count is outside typical range."));
    if (cont.weight > 9999.0f)
        results.append(makeResult(Severity::Error, "Weight", "Weight must be between 0 and 9999."));
    if (cont.value > 999999)
        results.append(makeResult(Severity::Error, "Value", "Value must be between -999999 and 999999."));

    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateTree(const TreeRecord& tree, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(tree.editorId, "", data, CkId::Type_Tree_);
    if (eid.severity != Severity::Info) results.append(eid);
    return results;
}

QVector<ColumnValidator::ValidationResult> ColumnValidator::validateLcrt(const LocationRefType& lcrt, Data* data)
{
    QVector<ValidationResult> results;

    auto eid = validateEditorId(lcrt.editorId, "", data, CkId::Type_Lcrt_);
    if (eid.severity != Severity::Info) results.append(eid);
    return results;
}
