#include "objectwindow.hpp"

#include "../world/data.hpp"
#include "../world/collection.hpp"
#include "../world/idcollection.hpp"
#include "../tools/objectwindowfilter.hpp"

#include <QJsonObject>
#include <QMimeData>
#include <QMap>
#include <QDataStream>
ObjectWindowModel::ObjectWindowModel(QObject* parent)
    : QAbstractItemModel(parent),
      mData(nullptr)
{
    mMaterializeTimer.setInterval(20);
    mMaterializeTimer.setSingleShot(false);
    connect(&mMaterializeTimer, &QTimer::timeout, this, &ObjectWindowModel::materializeTick);
}

void ObjectWindowModel::setData(Data* data)
{
    beginResetModel();

    mMaterializeTimer.stop();
    mJob.active = false;
    mData = data;
    mCategories.clear();
    mGroups.clear();
    mFilter.clear();

    if (mData)
    {
        initCategories(mData);
    }

    endResetModel();
}

void ObjectWindowModel::initCategories(Data* data)
{
    const bool isMorrowind = data->currentGame() == GameFormat::Game::Morrowind;

    auto addCategory = [this, data, isMorrowind](const QString& name, CkId::Type typeId) {
        Category cat;
        cat.name = name;
        cat.typeId = static_cast<int>(typeId);
        cat.totalRecords = 0;
        cat.parsedCount = 0;

        if (const BaseCollection* coll = data->getCollectionByType(typeId))
        {
            cat.totalRecords = coll->size();
            cat.parsedCount = cat.totalRecords;
            const int pending = data->masterIndexCount(typeId);
            if (pending > 0)
            {
                cat.totalRecords = cat.parsedCount + pending;
                cat.pendingMaterialize = true;
            }

            for (int i = 0; i < cat.parsedCount; i++)
            {
                VisibleRecord rec;
                rec.actualIndex = i;
                rec.editorId = coll->getId(i);
                const quint32 fid = coll->getFormId(i);
                rec.formId = (fid != 0) ? formatFormId(fid) : QString();
                cat.visibleRecords.append(rec);
            }
        }

        if (isMorrowind && cat.totalRecords == 0)
            return;

        mCategories.append(cat);
    };

        addCategory("NPC", CkId::Type_Npc_);

    addCategory("Armor", CkId::Type_Armor_);
    addCategory("Weapon", CkId::Type_Weap_);
    addCategory("Alchemy", CkId::Type_Alch_);
    addCategory("Ingredient", CkId::Type_Ingr_);
    addCategory("Book", CkId::Type_Book_);
    addCategory("Misc", CkId::Type_Misc_);
    addCategory("Container", CkId::Type_Cont_);
    addCategory("Enchantment", CkId::Type_Ench_);
    addCategory("Spell", CkId::Type_Spel_);
    addCategory("Magic Effect", CkId::Type_Magic_);

    addCategory("Static", CkId::Type_Stat_);
    addCategory("Activator", CkId::Type_Acti_);
    addCategory("Tree", CkId::Type_Tree_);

    addCategory("Quest", CkId::Type_Quest_);
    addCategory("Package", CkId::Type_Pack_);
    addCategory("Global", CkId::Type_Glob_);
    addCategory("Game Setting", CkId::Type_Gmst);
    addCategory("Perk", CkId::Type_PerK_);
    addCategory("Class", CkId::Type_Class_);
    addCategory("Faction", CkId::Type_Fact_);
    addCategory("Race", CkId::Type_Race_);

    addCategory("Sound", CkId::Type_Soun_);

    addCategory("Dialogue", CkId::Type_Dial_);
    addCategory("Info", CkId::Type_Info_);

    addCategory("Weather", CkId::Type_Wthr_);
    addCategory("Land Texture", CkId::Type_Ltex_);
    addCategory("Location Reference Type", CkId::Type_Lcrt_);
    addCategory("Cell", CkId::Type_Cel_);
    addCategory("Worldspace", CkId::Type_WRLD_);
    addCategory("Location", CkId::Type_LOCT_);
    addCategory("Planet", CkId::Type_Plnt_);
    addCategory("Reference", CkId::Type_Refr_);
    addCategory("Material", CkId::Type_Material_);
    addCategory("Landscape", CkId::Type_Land_);

    addCategory("Creature", CkId::Type_Crea_);
    addCategory("Leveled Actor", CkId::Type_Lvlc_);
    addCategory("Leveled NPC", CkId::Type_Lvln_);
    addCategory("Actor Values", CkId::Type_Avif_);
    addCategory("Voice Types", CkId::Type_Vtyp_);

    addCategory("Ammo", CkId::Type_Ammo_);
    addCategory("Key", CkId::Type_Keym_);
    addCategory("Soul Gem", CkId::Type_Slgm_);
    addCategory("Scroll", CkId::Type_Scrl_);
    addCategory("Potion", CkId::Type_Alch_);
    addCategory("Leveled Item", CkId::Type_Lvli_);
    addCategory("Constructible Object", CkId::Type_Cobj_);
    addCategory("Outfit", CkId::Type_Otft_);

    addCategory("Movable Static", CkId::Type_Mstt_);
    addCategory("Static Collection", CkId::Type_Scol_);
    addCategory("Door", CkId::Type_Door_);
    addCategory("Furniture", CkId::Type_Furn_);
    addCategory("Flora", CkId::Type_Flor_);
    addCategory("Grass", CkId::Type_Grass_);
    addCategory("Debris", CkId::Type_Debr_);
    addCategory("Hazard", CkId::Type_Hazd_);
    addCategory("Idle Marker", CkId::Type_Idlm_);
    addCategory("Light", CkId::Type_Ligh_);
    addCategory("Acoustic Space", CkId::Type_Aspc_);
    addCategory("Image Space", CkId::Type_Imgs_);

    addCategory("Combat Style", CkId::Type_Csty_);
    addCategory("Encounter Zone", CkId::Type_Eczn_);
    addCategory("Body Part", CkId::Type_Bptd_);
    addCategory("Head Part", CkId::Type_Hdpt_);
    addCategory("Keyword", CkId::Type_Kywd_);
    addCategory("Camera Path", CkId::Type_Cpth_);
    addCategory("Camera Shot", CkId::Type_Cams_);
    addCategory("Impact Data", CkId::Type_Ipct_);
    addCategory("Lens Flare", CkId::Type_Lens_);
    addCategory("Speech Challenge", CkId::Type_Spch_);

    addCategory("Music Type", CkId::Type_Must_);
    addCategory("Music Track", CkId::Type_Musc_);

    addCategory("Action", CkId::Type_Aact_);
    addCategory("Audio Modifier", CkId::Type_Aamd_);
    addCategory("Animated Prop", CkId::Type_Aapd_);
    addCategory("Actor Reference", CkId::Type_Achr_);
    addCategory("Add-On Node", CkId::Type_Addn_);
    addCategory("Affinity Event", CkId::Type_Affe_);
    addCategory("Ambient Sound", CkId::Type_Ambs_);
    addCategory("Audio Model", CkId::Type_Amdl_);
    addCategory("Ambient Occlusion Float", CkId::Type_Aopf_);
    addCategory("Shader Params", CkId::Type_Aops_);
    addCategory("Audio Reverb Unit", CkId::Type_Aoru_);
    addCategory("Armor Addon", CkId::Type_Arma_);
    addCategory("Art Object 2", CkId::Type_Arto_);
    addCategory("Acoustic Space", CkId::Type_Aspc_);
    addCategory("Atmosphere", CkId::Type_Atmo_);
    addCategory("Audio Visual Data", CkId::Type_Avmd_);
    addCategory("Biome", CkId::Type_Biom_);
    addCategory("Behavior", CkId::Type_Bmmo_);
    addCategory("Behavior Mod", CkId::Type_Bmod_);
    addCategory("Bounds", CkId::Type_Bnds_);
    addCategory("Behavior Pattern", CkId::Type_Bptd_);
    addCategory("Camera Shot", CkId::Type_Cams_);
    addCategory("Challenge", CkId::Type_Chal_);
    addCategory("Character Float Table", CkId::Type_Cldf_);
    addCategory("Character Data", CkId::Type_Cndf_);
    addCategory("Collision", CkId::Type_Coll_);
    addCategory("Camera Path", CkId::Type_Cpth_);
    addCategory("Culling Data", CkId::Type_Dlbr_);
    addCategory("Curve Table 3", CkId::Type_Cur3_);
    addCategory("Curve Table", CkId::Type_Curv_);
    addCategory("Debris Object", CkId::Type_Dfob_);
    addCategory("Damage Type", CkId::Type_Dmgt_);
    addCategory("Default Object", CkId::Type_Dobj_);
    addCategory("Effect Shader 2", CkId::Type_Efsq_);
    addCategory("Equipment Slot", CkId::Type_Equp_);

    addCategory("Furnishing Keyword", CkId::Type_Ffkw_);
    addCategory("Fog Volume", CkId::Type_Fogv_);
    addCategory("Force Field", CkId::Type_Forc_);
    addCategory("Footstep Set", CkId::Type_Fstp_);
    addCategory("Footstep Sound", CkId::Type_Fsts_);
    addCategory("FX Particle", CkId::Type_Fxpd_);
    addCategory("Biome Mask", CkId::Type_Gbfm_);
    addCategory("Biome Type", CkId::Type_Gbft_);
    addCategory("Gravity Camera", CkId::Type_Gcvr_);
    addCategory("Image Space Adapter", CkId::Type_Imad_);
    addCategory("Interior Data", CkId::Type_Innr_);
    addCategory("Resource Property", CkId::Type_Ires_);
    addCategory("Keyword Set", CkId::Type_Kssm_);
    addCategory("Layer", CkId::Type_Layr_);
    addCategory("Lens", CkId::Type_Lens_);
    addCategory("Landscape Grid Data", CkId::Type_Lgdi_);
    addCategory("Light Template", CkId::Type_Lgtm_);
    addCategory("Lens Modifier Set", CkId::Type_Lmsw_);
    addCategory("Leveled Biome", CkId::Type_Lvlb_);
    addCategory("Leveled Node", CkId::Type_Lvln_);
    addCategory("Leveled Perk", CkId::Type_Lvlp_);
    addCategory("Leveled Structure", CkId::Type_Lvsc_);
    addCategory("Material Attachment", CkId::Type_Maam_);
    addCategory("Mesh Renderer", CkId::Type_Mrhp_);
    addCategory("Mount Point", CkId::Type_Mtpt_);
    addCategory("Navigation Island", CkId::Type_Navi_);
    addCategory("Navigation Component", CkId::Type_Nocm_);
    addCategory("Object Mod", CkId::Type_Omod_);
    addCategory("Object Swap Set", CkId::Type_Oswp_);
    addCategory("Object Visual", CkId::Type_Ovis_);
    addCategory("Placement Configuration", CkId::Type_Pcbn_);
    addCategory("Placement Collision Node", CkId::Type_Pccn_);
    addCategory("Placement Material", CkId::Type_Pcmt_);
    addCategory("Particle Decal", CkId::Type_Pdcl_);
    addCategory("Particle Emitter", CkId::Type_Pgre_);
    addCategory("Voice Type", CkId::Type_Vtyp_);
    addCategory("Material Type", CkId::Type_Matt_);
    addCategory("Movement Type", CkId::Type_Movt_);

    addCategory("Animated Object", CkId::Type_Anio_);
    addCategory("Color", CkId::Type_Clfm_);
    addCategory("Relationship", CkId::Type_Rela_);
    addCategory("Reverb", CkId::Type_Revb_);
    addCategory("Shout", CkId::Type_Shou_);

    addCategory("Topic", CkId::Type_Dial_);
    addCategory("Scene", CkId::Type_Scen_);
    addCategory("Message", CkId::Type_Mesg_);
    addCategory("Note", CkId::Type_Note_);
    addCategory("Terminal", CkId::Type_Term_);

    addCategory("Navmesh", CkId::Type_Navm_);
    addCategory("Climate", CkId::Type_Clmt_);

    addCategory("Effect Shader", CkId::Type_Efsh_);
    addCategory("Art Object", CkId::Type_Artv_);
    addCategory("Water Shader", CkId::Type_Wate_);
    addCategory("Weather Shader", CkId::Type_Wths_);
    addCategory("Default Object", CkId::Type_Dobj_);
    addCategory("Biome", CkId::Type_Biom_);
    addCategory("Snap Template", CkId::Type_Stmp_);
    addCategory("Apparatus", CkId::Type_Appa_);
    addCategory("Birthsign", CkId::Type_Bsgn_);
    addCategory("Clothing", CkId::Type_Clot_);
    addCategory("Explosion", CkId::Type_Expl_);
    addCategory("Eyes", CkId::Type_Eyes_);
    addCategory("Form List", CkId::Type_Flst_);
    addCategory("Hair", CkId::Type_Hair_);
    addCategory("Idle Animation", CkId::Type_Idle_);
    addCategory("Leveled Spell", CkId::Type_Lvsp_);
    addCategory("Load Screen", CkId::Type_Lscr_);
    addCategory("Projectile", CkId::Type_Proj_);
    addCategory("Region", CkId::Type_Regn_);
    addCategory("Road", CkId::Type_Road_);
    addCategory("Script", CkId::Type_Scpt_);
    addCategory("Sound Marker", CkId::Type_Smqn_);
    addCategory("Texture Set", CkId::Type_Txst_);
    addCategory("PHZD", CkId::Type_Phzd_);
    addCategory("PKIN", CkId::Type_Pkin_);
    addCategory("PMFT", CkId::Type_Pmft_);
    addCategory("PSDC", CkId::Type_Psdc_);
    addCategory("PTST", CkId::Type_Ptst_);
    addCategory("RFGP", CkId::Type_Rfgp_);
    addCategory("RSGD", CkId::Type_Rsgd_);
    addCategory("RSPJ", CkId::Type_Rspj_);
    addCategory("SDLT", CkId::Type_Sdlt_);
    addCategory("SECH", CkId::Type_Sech_);
    addCategory("SFBK", CkId::Type_Sfbk_);
    addCategory("SFPC", CkId::Type_Sfpc_);
    addCategory("SFPT", CkId::Type_Sfpt_);
    addCategory("SFTR", CkId::Type_Sftr_);
    addCategory("SMBN", CkId::Type_Smbn_);
    addCategory("SMEN", CkId::Type_Smen_);
    addCategory("SPCH", CkId::Type_Spch_);
    addCategory("STAG", CkId::Type_Stag_);
    addCategory("STBH", CkId::Type_Stbh_);
    addCategory("STDT", CkId::Type_Stdt_);
    addCategory("STMP", CkId::Type_Stmp_);
    addCategory("STND", CkId::Type_Stnd_);
    addCategory("SUNP", CkId::Type_Sunp_);
    addCategory("TMLM", CkId::Type_Tmlm_);
    addCategory("TODD", CkId::Type_Todd_);
    addCategory("TRAV", CkId::Type_Trav_);
    addCategory("TRNS", CkId::Type_Trns_);
    addCategory("VOLI", CkId::Type_Voli_);
    addCategory("VTYP", CkId::Type_Vtyp_);
    addCategory("WBAR", CkId::Type_Wbar_);
    addCategory("WKMF", CkId::Type_Wkmf_);
    addCategory("WTHS", CkId::Type_Wths_);
    addCategory("WWED", CkId::Type_Wwed_);
    addCategory("ZOOM", CkId::Type_Zoom_);

    if (isMorrowind)
    {
        addCategory("Body Part", CkId::Type_Body_);
        addCategory("Leveled Creature List", CkId::Type_Levc_);
        addCategory("Leveled Item List", CkId::Type_Levi_);
        addCategory("Lockpick", CkId::Type_Lock_);
        addCategory("Path Grid", CkId::Type_Pgrd_);
        addCategory("Probe", CkId::Type_Prob_);
        addCategory("Repair Tool", CkId::Type_Repa_);
        addCategory("Sound Generator", CkId::Type_Sndg_);
        addCategory("Skill", CkId::Type_Skil_);
    }

    auto addGroupNamed = [this](const QString& name, std::initializer_list<QString> catNames) {
        CategoryGroup group;
        group.name = name;
        for (const QString& n : catNames)
        {
            for (int i = 0; i < mCategories.size(); i++)
            {
                if (mCategories[i].name == n)
                {
                    group.categoryIndices.append(i);
                    break;
                }
            }
        }
        mGroups.append(group);
    };

    // All Forms: flat list of every category
    {
        CategoryGroup allForms;
        allForms.name = "All Forms";
        for (int i = 0; i < mCategories.size(); i++)
            allForms.categoryIndices.append(i);
        mGroups.append(allForms);
    }

    addGroupNamed("Actors", {"NPC", "Creature", "Leveled Actor", "Leveled NPC", "Actor Values", "Voice Types",
                             "Body Part", "Head Part", "Eyes", "Hair", "Idle Animation", "Leveled Creature List"});
    addGroupNamed("Items", {"Armor", "Weapon", "Book", "Misc", "Container",
                            "Ammo", "Key", "Soul Gem", "Leveled Item", "Constructible Object", "Outfit",
                            "Apparatus", "Clothing", "Form List",
                            "Lockpick", "Probe", "Repair Tool", "Leveled Item List"});
    addGroupNamed("Magic", {"Spell", "Magic Effect", "Enchantment", "Potion", "Alchemy", "Ingredient",
                            "Scroll", "Leveled Spell", "Shout"});
    addGroupNamed("World Objects", {"Static", "Activator", "Tree", "Movable Static", "Static Collection",
                                    "Door", "Furniture", "Flora", "Grass", "Debris", "Hazard", "Idle Marker",
                                    "Light", "Acoustic Space", "Image Space", "Explosion", "Projectile",
                                    "Texture Set"});
    addGroupNamed("Gameplay", {"Quest", "Package", "Global", "Game Setting", "Perk", "Class", "Faction",
                               "Race", "Combat Style", "Encounter Zone", "Location", "Location Reference Type",
                               "Keyword", "Camera Path", "Camera Shot", "Impact Data", "Lens Flare",
                               "Speech Challenge", "Birthsign", "Relationship", "Movement Type", "Skill"});
    addGroupNamed("Audio", {"Sound", "Music Type", "Music Track", "Voice Type", "Sound Marker", "Reverb", "Sound Generator"});
    addGroupNamed("Dialogue", {"Dialogue", "Info", "Topic", "Scene", "Message", "Note", "Terminal"});
    addGroupNamed("Special", {"Cell", "Worldspace", "Navmesh", "Landscape", "Reference", "Weather",
                              "Land Texture", "Climate", "Region", "Road", "Path Grid", "Water Shader", "Weather Shader"});
    addGroupNamed("Space", {"Planet", "Biome", "Biome Type", "Biome Mask"});
    addGroupNamed("Miscellaneous", {"Effect Shader", "Art Object", "Power", "Default Object", "Association Type",
                                    "Snap Template", "Material", "Material Type", "Load Screen", "Script",
                                    "Animated Object", "Color"});
}

QString ObjectWindowModel::formatFormId(quint32 formId) const
{
    return QString("0x%1").arg(formId, 8, 16, QChar('0'));
}

void ObjectWindowModel::applyFilter(const QString& text)
{
    mFilter = text;
    rebuildAllRecords();

    const QString trimmed = mFilter.trimmed();
    if (trimmed.isEmpty())
    {
        emit layoutChanged();
        return;
    }

    const bool isWildcard = trimmed.contains(QLatin1Char('*')) || trimmed.contains(QLatin1Char('?'));
    QRegularExpression wildRe;
    if (isWildcard)
    {
        wildRe = QRegularExpression(QRegularExpression::wildcardToRegularExpression(trimmed),
                                     QRegularExpression::CaseInsensitiveOption);
    }
    const QString lowerFilter = trimmed.toLower();

    for (auto& cat : mCategories)
    {
        QVector<VisibleRecord> filtered;
        for (const auto& rec : cat.visibleRecords)
        {
            bool hit = false;
            if (isWildcard && wildRe.isValid())
            {
                hit = wildRe.match(rec.editorId).hasMatch() || wildRe.match(rec.formId).hasMatch();
            }
            else
            {
                hit = rec.editorId.toLower().contains(lowerFilter) ||
                      rec.formId.toLower().contains(lowerFilter);
            }
            if (hit)
            {
                filtered.append(rec);
            }
        }
        cat.visibleRecords = filtered;
    }

    emit layoutChanged();
}

void ObjectWindowModel::applyObjectFilter(const ObjectWindowFilter& filter)
{
    mActiveObjectFilter = filter;
    rebuildAllRecords();
    for (auto& cat : mCategories)
    {
        QVector<VisibleRecord> filtered;
        for (const auto& rec : cat.visibleRecords)
        {
            QJsonObject record;
            record.insert(QStringLiteral("EditorID"), rec.editorId);
            record.insert(QStringLiteral("FormID"), rec.formId);
            record.insert(QStringLiteral("Type"), cat.name);
            if (filter.matches(record))
                filtered.append(rec);
        }
        cat.visibleRecords = filtered;
    }
    emit layoutChanged();
}

void ObjectWindowModel::rebuildAllRecords()
{
    if (!mData)
        return;

    for (auto& cat : mCategories)
    {
        cat.visibleRecords.clear();

        const BaseCollection* coll = mData->getCollectionByType(static_cast<CkId::Type>(cat.typeId));
        if (!coll)
            continue;

        const int count = coll->size();
        for (int i = 0; i < count; i++)
        {
            VisibleRecord rec;
            rec.actualIndex = i;
            rec.editorId = coll->getId(i);
            const quint32 fid = coll->getFormId(i);
            rec.formId = (fid != 0) ? formatFormId(fid) : QString();
            cat.visibleRecords.append(rec);
        }
    }
}

QModelIndex ObjectWindowModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    if (!parent.isValid())
    {
        if (row < 0 || row >= mGroups.size())
            return QModelIndex();
        return createIndex(row, column, kGroupInternalId);
    }

    quintptr parentInternal = parent.internalId();
    if (parentInternal == kGroupInternalId)
    {
        int groupRow = parent.row();
        if (groupRow < 0 || groupRow >= mGroups.size())
            return QModelIndex();
        const auto& group = mGroups[groupRow];
        if (row < 0 || row >= group.categoryIndices.size())
            return QModelIndex();
        return createIndex(row, column, static_cast<quintptr>((groupRow << 16) | row));
    }

    if (!(parentInternal & kRecordBit))
    {
        int groupRow = static_cast<int>((parentInternal >> 16) & 0xFFFF);
        int categoryRow = parent.row();
        if (groupRow < 0 || groupRow >= mGroups.size())
            return QModelIndex();
        const auto& group = mGroups[groupRow];
        if (categoryRow < 0 || categoryRow >= group.categoryIndices.size())
            return QModelIndex();
        int flatId = group.categoryIndices[categoryRow];
        if (row < 0 || row >= mCategories[flatId].visibleRecords.size())
            return QModelIndex();
        return createIndex(row, column, kRecordBit | static_cast<quintptr>((groupRow << 16) | categoryRow));
    }

    return QModelIndex();
}

QModelIndex ObjectWindowModel::parent(const QModelIndex& child) const
{
    if (!child.isValid())
        return QModelIndex();

    quintptr internal = child.internalId();
    if (internal == kGroupInternalId)
        return QModelIndex();

    if (!(internal & kRecordBit))
    {
        int groupRow = static_cast<int>((internal >> 16) & 0xFFFF);
        if (groupRow < 0 || groupRow >= mGroups.size())
            return QModelIndex();
        return createIndex(groupRow, 0, kGroupInternalId);
    }

    int groupRow = static_cast<int>(((internal & ~kRecordBit) >> 16) & 0xFFFF);
    int categoryRow = static_cast<int>((internal & ~kRecordBit) & 0xFFFF);
    if (groupRow < 0 || groupRow >= mGroups.size())
        return QModelIndex();
    return createIndex(categoryRow, 0, static_cast<quintptr>((groupRow << 16) | categoryRow));
}

int ObjectWindowModel::rowCount(const QModelIndex& parent) const
{
    if (!mData)
        return 0;

    if (!parent.isValid())
        return mGroups.size();

    quintptr internal = parent.internalId();
    if (internal == kGroupInternalId)
    {
        int groupRow = parent.row();
        if (groupRow < 0 || groupRow >= mGroups.size())
            return 0;
        return mGroups[groupRow].categoryIndices.size();
    }

    if (!(internal & kRecordBit))
    {
        int groupRow = static_cast<int>((internal >> 16) & 0xFFFF);
        int categoryRow = parent.row();
        if (groupRow < 0 || groupRow >= mGroups.size())
            return 0;
        const auto& group = mGroups[groupRow];
        if (categoryRow < 0 || categoryRow >= group.categoryIndices.size())
            return 0;
        int flatId = group.categoryIndices[categoryRow];
        return mCategories[flatId].visibleRecords.size();
    }

    return 0;
}
bool ObjectWindowModel::hasChildren(const QModelIndex& parent) const
{
    if (!mData)
        return false;

    if (!parent.isValid())
        return !mGroups.isEmpty();

    quintptr internal = parent.internalId();
    if (internal == kGroupInternalId)
    {
        int groupRow = parent.row();
        if (groupRow < 0 || groupRow >= mGroups.size())
            return false;
        return !mGroups[groupRow].categoryIndices.isEmpty();
    }

    if (internal & kRecordBit)
        return false;

    int groupRow = static_cast<int>((internal >> 16) & 0xFFFF);
    int categoryRow = parent.row();
    if (groupRow < 0 || groupRow >= mGroups.size())
        return false;
    const auto& group = mGroups[groupRow];
    if (categoryRow < 0 || categoryRow >= group.categoryIndices.size())
        return false;
    int flatId = group.categoryIndices[categoryRow];
    const auto& cat = mCategories[flatId];
    return !cat.visibleRecords.isEmpty() || cat.pendingMaterialize || cat.totalRecords > 0;
}

int ObjectWindowModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 4;
}

QVariant ObjectWindowModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractItemModel::headerData(section, orientation, role);

    switch (section)
    {
    case 0: return QStringLiteral("Editor ID");
    case 1: return QStringLiteral("Form ID");
    case 2: return QStringLiteral("Type");
    case 3: return QStringLiteral("Model");
    default: return QVariant();
    }
}

bool ObjectWindowModel::canFetchMore(const QModelIndex& parent) const
{
    if (!mData || !parent.isValid())
        return false;

    // Only category nodes fetch (deferred master types); group nodes
    // (internalId 0) and record nodes never do.
    quintptr internal = parent.internalId();
    if (internal == kGroupInternalId || (internal & kRecordBit))
        return false;

    int groupRow = static_cast<int>((internal >> 16) & 0xFFFF);
    int categoryRow = parent.row();
    if (groupRow < 0 || groupRow >= mGroups.size())
        return false;
    const auto& group = mGroups[groupRow];
    if (categoryRow < 0 || categoryRow >= group.categoryIndices.size())
        return false;
    return mCategories[group.categoryIndices[categoryRow]].pendingMaterialize;
}

void ObjectWindowModel::appendMaterializedRows(int groupRow, int categoryRow, int firstNew, const QModelIndex& parent)
{
    if (!mData || !parent.isValid())
        return;
    if (groupRow < 0 || groupRow >= mGroups.size())
        return;
    const auto& group = mGroups[groupRow];
    if (categoryRow < 0 || categoryRow >= group.categoryIndices.size())
        return;
    Category& cat = mCategories[group.categoryIndices[categoryRow]];

    BaseCollection* coll = mData->getCollectionByType(static_cast<CkId::Type>(cat.typeId));
    if (!coll)
        return;

    const int end = coll->size();
    if (firstNew >= end)
        return;

    QVector<VisibleRecord> added;
    for (int i = firstNew; i < end; ++i)
    {
        VisibleRecord rec;
        rec.actualIndex = i;
        rec.editorId = coll->getId(i);
        rec.formId = formatFormId(coll->getFormId(i));
        added.append(rec);
    }

    // Keep inserted rows consistent with any active filters.
    const QString lowerFilter = mFilter.toLower();
    if (!lowerFilter.isEmpty())
    {
        QVector<VisibleRecord> filtered;
        for (const auto& rec : added)
        {
            if (rec.editorId.toLower().contains(lowerFilter)
                || rec.formId.toLower().contains(lowerFilter))
                filtered.append(rec);
        }
        added = filtered;
    }
    if (mActiveObjectFilter.count() > 0)
    {
        QVector<VisibleRecord> filtered;
        for (const auto& rec : added)
        {
            QJsonObject record;
            record.insert(QStringLiteral("EditorID"), rec.editorId);
            record.insert(QStringLiteral("FormID"), rec.formId);
            record.insert(QStringLiteral("Type"), cat.name);
            if (mActiveObjectFilter.matches(record))
                filtered.append(rec);
        }
        added = filtered;
    }

    if (added.isEmpty())
        return;

    const int startRow = cat.visibleRecords.size();
    beginInsertRows(parent, startRow, startRow + added.size() - 1);
    cat.visibleRecords.append(added);
    endInsertRows();
}

void ObjectWindowModel::startMaterializeTimer()
{
    mMaterializeTimer.stop();
    mMaterializeTimer.start(20);
}

void ObjectWindowModel::materializeTick()
{
    if (!mData || !mJob.active)
    {
        mMaterializeTimer.stop();
        return;
    }

    const int typeId = mJob.typeId;
    const int firstNew = mJob.firstNew;
    mData->materializeNextBatch(typeId, 256);

    const QModelIndex groupNode = index(mJob.groupRow, 0, QModelIndex());
    const QModelIndex parentIdx = index(mJob.categoryRow, 0, groupNode);
    appendMaterializedRows(mJob.groupRow, mJob.categoryRow, firstNew, parentIdx);
    mJob.firstNew = mCategories[mJob.flatId].visibleRecords.size();

    if (!mData->isMaterializing())
    {
        mMaterializeTimer.stop();
        Category& cat = mCategories[mJob.flatId];
        cat.pendingMaterialize = false;
        cat.parsedCount = cat.totalRecords;
        mJob.active = false;
        if (parentIdx.isValid())
            emit dataChanged(parentIdx, parentIdx);
    }
}

void ObjectWindowModel::fetchMore(const QModelIndex& parent)
{
    if (!mData || !parent.isValid())
        return;

    quintptr internal = parent.internalId();
    if (internal == kGroupInternalId || (internal & kRecordBit))
        return;

    int groupRow = static_cast<int>((internal >> 16) & 0xFFFF);
    int categoryRow = parent.row();
    if (groupRow < 0 || groupRow >= mGroups.size())
        return;
    const auto& group = mGroups[groupRow];
    if (categoryRow < 0 || categoryRow >= group.categoryIndices.size())
        return;

    Category& cat = mCategories[group.categoryIndices[categoryRow]];
    if (!cat.pendingMaterialize)
        return;

    // Small types (or types already fully parsed) load synchronously;
    // anything with deferred master records is materialized in 20 ms
    // slices so expanding a huge category never freezes the UI. Row
    // insertion is still done with beginInsertRows/endInsertRows on the
    // UI thread (a full model reset inside fetchMore crashes QTreeView).
    if (mData->isMaterializing())
        return;
    if (!mData->beginTypeMaterialization(cat.typeId))
    {
        cat.pendingMaterialize = false;
        return;
    }

    mJob.typeId = cat.typeId;
    mJob.groupRow = groupRow;
    mJob.categoryRow = categoryRow;
    mJob.flatId = group.categoryIndices[categoryRow];
    mJob.firstNew = cat.visibleRecords.size();
    mJob.active = true;

    mData->materializeNextBatch(mJob.typeId, 256);
    appendMaterializedRows(mJob.groupRow, mJob.categoryRow, mJob.firstNew, parent);
    mJob.firstNew = mCategories[mJob.flatId].visibleRecords.size();

    if (mData->isMaterializing())
    {
        startMaterializeTimer();
    }
    else
    {
        cat.pendingMaterialize = false;
        cat.parsedCount = cat.totalRecords;
        mJob.active = false;
        if (parent.isValid())
            emit dataChanged(parent, parent);
    }
}

QVariant ObjectWindowModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    quintptr internal = index.internalId();

    if (internal == kGroupInternalId)
    {
        int groupRow = index.row();
        if (groupRow < 0 || groupRow >= mGroups.size())
            return QVariant();
        switch (index.column())
        {
        case 0:
            return mGroups[groupRow].name;
        default:
            return QString();
        }
    }

    if (!(internal & kRecordBit))
    {
        int groupRow = static_cast<int>((internal >> 16) & 0xFFFF);
        int categoryRow = index.row();
        if (groupRow < 0 || groupRow >= mGroups.size())
            return QVariant();
        const auto& group = mGroups[groupRow];
        if (categoryRow < 0 || categoryRow >= group.categoryIndices.size())
            return QVariant();
        int flatId = group.categoryIndices[categoryRow];
        const auto& cat = mCategories[flatId];
        switch (index.column())
        {
        case 0:
        {
            QString label = cat.name;
            if (cat.pendingMaterialize)
            {
                label += QStringLiteral(" (%1 deferred)")
                    .arg(cat.totalRecords - cat.parsedCount);
            }
            return label;
        }
        case 1:
            return QString();
        case 2:
        {
            CkId::Type t = static_cast<CkId::Type>(cat.typeId);
            return (t == CkId::Type_None) ? QStringLiteral("-") : CkId(t).getTypeName();
        }
        }
        return QVariant();
    }

    int groupRow = static_cast<int>(((internal & ~kRecordBit) >> 16) & 0xFFFF);
    int categoryRow = static_cast<int>((internal & ~kRecordBit) & 0xFFFF);
    if (groupRow < 0 || groupRow >= mGroups.size())
        return QVariant();
    const auto& group = mGroups[groupRow];
    if (categoryRow < 0 || categoryRow >= group.categoryIndices.size())
        return QVariant();
    int flatId = group.categoryIndices[categoryRow];
    const auto& visibleRecords = mCategories[flatId].visibleRecords;
    int localRow = index.row();
    if (localRow < 0 || localRow >= visibleRecords.size())
        return QVariant();
    const auto& rec = visibleRecords[localRow];
    switch (index.column())
    {
    case 0:
        return rec.editorId;
    case 1:
        return rec.formId;
    case 2:
    {
        CkId::Type t = static_cast<CkId::Type>(mCategories[flatId].typeId);
        return (t == CkId::Type_None) ? QStringLiteral("-") : CkId(t).getTypeName();
    }
    case 3:
        return getRecordModelPath(flatId, rec.actualIndex);
    }
    return QVariant();
}

Qt::ItemFlags ObjectWindowModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::ItemFlag::ItemIsDropEnabled;

    if (isRecordNode(index))
        return QAbstractItemModel::flags(index) | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsDragEnabled;

    return QAbstractItemModel::flags(index) | Qt::ItemFlag::ItemIsEnabled;
}

QStringList ObjectWindowModel::mimeTypes() const
{
    return { QStringLiteral("application/x-openck-record"), QStringLiteral("text/plain") };
}

QMimeData* ObjectWindowModel::mimeData(const QModelIndexList& indexes) const
{
    auto* mime = new QMimeData();
    for (const QModelIndex& idx : indexes)
    {
        if (idx.isValid() && isRecordNode(idx) && idx.column() == 0)
        {
            int catId = getCategoryIndex(idx);
            int recIdx = getRecordIndex(idx);
            if (catId >= 0 && recIdx >= 0)
            {
                const QString& formIdStr = getRecordFormId(catId, recIdx);
                const QString& editorId = getRecordEditorId(catId, recIdx);
                int typeId = getCategoryType(catId);

                QByteArray data;
                QDataStream stream(&data, QIODevice::WriteOnly);
                bool ok = false;
                quint32 formId = formIdStr.toUInt(&ok, 16);
                stream << static_cast<qint32>(typeId) << formId << editorId;

                mime->setData(QStringLiteral("application/x-openck-record"), data);
                mime->setText(editorId.isEmpty() ? formIdStr : editorId);
                return mime;
            }
        }
    }
    return mime;
}

int ObjectWindowModel::flatCategoryId(int groupRow, int categoryRow) const
{
    if (groupRow < 0 || groupRow >= mGroups.size())
        return -1;
    const auto& group = mGroups[groupRow];
    if (categoryRow < 0 || categoryRow >= group.categoryIndices.size())
        return -1;
    return group.categoryIndices[categoryRow];
}

bool ObjectWindowModel::findCategoryLocation(int flatId, int& groupRow, int& categoryRow) const
{
    for (int g = 0; g < mGroups.size(); g++)
    {
        const auto& group = mGroups[g];
        for (int c = 0; c < group.categoryIndices.size(); c++)
        {
            if (group.categoryIndices[c] == flatId)
            {
                groupRow = g;
                categoryRow = c;
                return true;
            }
        }
    }
    return false;
}

bool ObjectWindowModel::isGroupNode(const QModelIndex& index) const
{
    return index.isValid() && index.internalId() == kGroupInternalId;
}

bool ObjectWindowModel::isCategoryNode(const QModelIndex& index) const
{
    quintptr internal = index.internalId();
    return index.isValid() && internal != kGroupInternalId && !(internal & kRecordBit);
}

bool ObjectWindowModel::isRecordNode(const QModelIndex& index) const
{
    return index.isValid() && (index.internalId() & kRecordBit) != 0;
}

bool ObjectWindowModel::isRecord(const QModelIndex& index) const
{
    return isRecordNode(index);
}

int ObjectWindowModel::getCategoryIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return -1;

    quintptr internal = index.internalId();
    if (internal == kGroupInternalId)
        return -1;

    if (!(internal & kRecordBit))
    {
        int groupRow = static_cast<int>((internal >> 16) & 0xFFFF);
        int categoryRow = index.row();
        return flatCategoryId(groupRow, categoryRow);
    }

    int groupRow = static_cast<int>(((internal & ~kRecordBit) >> 16) & 0xFFFF);
    int categoryRow = static_cast<int>((internal & ~kRecordBit) & 0xFFFF);
    return flatCategoryId(groupRow, categoryRow);
}

int ObjectWindowModel::getCategoryType(int categoryId) const
{
    if (categoryId < 0 || categoryId >= mCategories.size())
        return static_cast<int>(CkId::Type_None);
    return mCategories[categoryId].typeId;
}

int ObjectWindowModel::getRecordIndex(const QModelIndex& index) const
{
    if (!isRecordNode(index))
        return -1;

    quintptr internal = index.internalId();
    int groupRow = static_cast<int>(((internal & ~kRecordBit) >> 16) & 0xFFFF);
    int categoryRow = static_cast<int>((internal & ~kRecordBit) & 0xFFFF);
    int flatId = flatCategoryId(groupRow, categoryRow);
    if (flatId < 0)
        return -1;

    int localRow = index.row();
    const auto& visibleRecords = mCategories[flatId].visibleRecords;
    if (localRow < 0 || localRow >= visibleRecords.size())
        return -1;

    return visibleRecords[localRow].actualIndex;
}

QModelIndex ObjectWindowModel::getCategoryIndexModel(int categoryId) const
{
    int groupRow = 0;
    int categoryRow = 0;
    if (!findCategoryLocation(categoryId, groupRow, categoryRow))
        return QModelIndex();
    return createIndex(categoryRow, 0, static_cast<quintptr>((groupRow << 16) | categoryRow));
}

QModelIndex ObjectWindowModel::getRecordIndexModel(int categoryId, int recordIndex) const
{
    int groupRow = 0;
    int categoryRow = 0;
    if (!findCategoryLocation(categoryId, groupRow, categoryRow))
        return QModelIndex();

    const auto& visibleRecords = mCategories[categoryId].visibleRecords;
    for (int i = 0; i < visibleRecords.size(); i++)
    {
        if (visibleRecords[i].actualIndex == recordIndex)
        {
            return createIndex(i, 0, kRecordBit | static_cast<quintptr>((groupRow << 16) | categoryRow));
        }
    }
    return QModelIndex();
}

const QString& ObjectWindowModel::getRecordEditorId(int categoryId, int recordIndex) const
{
    static const QString empty;
    if (categoryId < 0 || categoryId >= mCategories.size())
        return empty;

    const auto& visibleRecords = mCategories[categoryId].visibleRecords;
    for (const auto& rec : visibleRecords)
    {
        if (rec.actualIndex == recordIndex)
            return rec.editorId;
    }
    return visibleRecords.isEmpty() ? empty : visibleRecords[0].editorId;
}

const QString& ObjectWindowModel::getRecordFormId(int categoryId, int recordIndex) const
{
    static const QString empty;
    if (categoryId < 0 || categoryId >= mCategories.size())
        return empty;

    const auto& visibleRecords = mCategories[categoryId].visibleRecords;
    for (const auto& rec : visibleRecords)
    {
        if (rec.actualIndex == recordIndex)
            return rec.formId;
    }
    return visibleRecords.isEmpty() ? empty : visibleRecords[0].formId;
}

bool ObjectWindowModel::findRecord(const QString& idOrFormId, int& outCategoryId, int& outRecordIndex, QModelIndex& outIndex) const
{
    if (idOrFormId.isEmpty())
        return false;

    bool isHex = false;
    quint32 numericFid = 0;
    QString cleanId = idOrFormId.trimmed();
    if (cleanId.startsWith(QLatin1String("0x"), Qt::CaseInsensitive))
        numericFid = cleanId.mid(2).toUInt(&isHex, 16);
    else
        numericFid = cleanId.toUInt(&isHex, 16);

    QString formattedFid = isHex ? formatFormId(numericFid) : QString();

    for (int catIdx = 0; catIdx < mCategories.size(); ++catIdx)
    {
        const auto& visibleRecords = mCategories[catIdx].visibleRecords;
        for (const auto& rec : visibleRecords)
        {
            if (rec.editorId.compare(cleanId, Qt::CaseInsensitive) == 0 ||
                rec.formId.compare(cleanId, Qt::CaseInsensitive) == 0 ||
                (!formattedFid.isEmpty() && rec.formId.compare(formattedFid, Qt::CaseInsensitive) == 0))
            {
                outCategoryId = catIdx;
                outRecordIndex = rec.actualIndex;
                outIndex = getRecordIndexModel(catIdx, rec.actualIndex);
                return true;
            }
        }
    }
    return false;
}

QString ObjectWindowModel::getRecordModelPath(int categoryId, int recordIndex) const
{
    if (!mData || categoryId < 0 || categoryId >= mCategories.size())
        return QString();

    CkId::Type type = static_cast<CkId::Type>(mCategories[categoryId].typeId);
    switch (type)
    {
    case CkId::Type_Weap_:
        if (recordIndex >= 0 && recordIndex < mData->getWeaponCollection().size())
            return mData->getWeaponCollection().getRecord(recordIndex).get().modelPath;
        break;
    case CkId::Type_Armor_:
        if (recordIndex >= 0 && recordIndex < mData->getArmorCollection().size())
            return mData->getArmorCollection().getRecord(recordIndex).get().modelPath;
        break;
    case CkId::Type_Stat_:
        if (recordIndex >= 0 && recordIndex < mData->getStatCollection().size())
            return mData->getStatCollection().getRecord(recordIndex).get().modelPath;
        break;
    case CkId::Type_Tree_:
        if (recordIndex >= 0 && recordIndex < mData->getTreeCollection().size())
            return mData->getTreeCollection().getRecord(recordIndex).get().modelPath;
        break;
    case CkId::Type_Acti_:
        if (recordIndex >= 0 && recordIndex < mData->getActiCollection().size())
            return mData->getActiCollection().getRecord(recordIndex).get().modelPath;
        break;
    case CkId::Type_Misc_:
        if (recordIndex >= 0 && recordIndex < mData->getMiscCollection().size())
            return mData->getMiscCollection().getRecord(recordIndex).get().modelPath;
        break;
    case CkId::Type_Book_:
        if (recordIndex >= 0 && recordIndex < mData->getBookCollection().size())
            return mData->getBookCollection().getRecord(recordIndex).get().modelPath;
        break;
    case CkId::Type_Ingr_:
        if (recordIndex >= 0 && recordIndex < mData->getIngrCollection().size())
            return mData->getIngrCollection().getRecord(recordIndex).get().modelPath;
        break;
    case CkId::Type_Alch_:
        if (recordIndex >= 0 && recordIndex < mData->getAlchCollection().size())
            return mData->getAlchCollection().getRecord(recordIndex).get().modelPath;
        break;
    case CkId::Type_Cont_:
        if (recordIndex >= 0 && recordIndex < mData->getContCollection().size())
            return mData->getContCollection().getRecord(recordIndex).get().modelPath;
        break;
    case CkId::Type_Door_:
        if (recordIndex >= 0 && recordIndex < mData->getDoorCollection().size())
            return mData->getDoorCollection().getRecord(recordIndex).get().modelPath;
        break;
    case CkId::Type_Furn_:
        if (recordIndex >= 0 && recordIndex < mData->getFurnCollection().size())
            return mData->getFurnCollection().getRecord(recordIndex).get().modelPath;
        break;
    case CkId::Type_Flor_:
        if (recordIndex >= 0 && recordIndex < mData->getFlorCollection().size())
            return mData->getFlorCollection().getRecord(recordIndex).get().modelPath;
        break;
    case CkId::Type_Ligh_:
        if (recordIndex >= 0 && recordIndex < mData->getLighCollection().size())
            return mData->getLighCollection().getRecord(recordIndex).get().modelPath;
        break;
    case CkId::Type_Mstt_:
        if (recordIndex >= 0 && recordIndex < mData->getMsttCollection().size())
            return mData->getMsttCollection().getRecord(recordIndex).get().modelPath;
        break;
    case CkId::Type_Ammo_:
        if (recordIndex >= 0 && recordIndex < mData->getAmmoCollection().size())
            return mData->getAmmoCollection().getRecord(recordIndex).get().modelPath;
        break;
    case CkId::Type_Keym_:
        if (recordIndex >= 0 && recordIndex < mData->getKeymCollection().size())
            return mData->getKeymCollection().getRecord(recordIndex).get().modelPath;
        break;

    default:
        break;
    }
    return QString();
}

int ObjectWindowModel::getRecordCountForCategory(int categoryId) const
{
    if (categoryId >= 0 && categoryId < mCategories.size())
        return mCategories[categoryId].visibleRecords.size();
    return 0;
}

int ObjectWindowModel::getRecordActualIndex(int categoryId, int visibleIndex) const
{
    if (categoryId >= 0 && categoryId < mCategories.size())
    {
        const auto& recs = mCategories[categoryId].visibleRecords;
        if (visibleIndex >= 0 && visibleIndex < recs.size())
            return recs[visibleIndex].actualIndex;
    }
    return visibleIndex;
}

const QString& ObjectWindowModel::getVisibleRecordEditorId(int categoryId, int visibleIndex) const
{
    static const QString empty;
    if (categoryId < 0 || categoryId >= mCategories.size())
        return empty;
    const auto& recs = mCategories[categoryId].visibleRecords;
    if (visibleIndex >= 0 && visibleIndex < recs.size())
        return recs[visibleIndex].editorId;
    return empty;
}

const QString& ObjectWindowModel::getVisibleRecordFormId(int categoryId, int visibleIndex) const
{
    static const QString empty;
    if (categoryId < 0 || categoryId >= mCategories.size())
        return empty;
    const auto& recs = mCategories[categoryId].visibleRecords;
    if (visibleIndex >= 0 && visibleIndex < recs.size())
        return recs[visibleIndex].formId;
    return empty;
}

QString ObjectWindowModel::getVisibleRecordModelPath(int categoryId, int visibleIndex) const
{
    if (categoryId < 0 || categoryId >= mCategories.size())
        return QString();
    const auto& recs = mCategories[categoryId].visibleRecords;
    if (visibleIndex >= 0 && visibleIndex < recs.size())
        return getRecordModelPath(categoryId, recs[visibleIndex].actualIndex);
    return QString();
}

QString ObjectWindowModel::getCategoryName(int categoryId) const
{
    if (categoryId >= 0 && categoryId < mCategories.size())
        return mCategories[categoryId].name;
    return QString();
}

namespace
{
// '\' -> '/', then drop an optional "data/" and "meshes/" prefix (any case).
QString normalizeModelPath(QString path)
{
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));
    path = path.trimmed();
    for (const QLatin1String prefix : { QLatin1String("data/"), QLatin1String("meshes/") })
        if (path.startsWith(prefix, Qt::CaseInsensitive))
            path.remove(0, prefix.size());
    return path;
}

QString directoryOf(const QString& normalized)
{
    const int slash = normalized.lastIndexOf(QLatin1Char('/'));
    return slash < 0 ? QString() : normalized.left(slash);
}
}

QString ObjectWindowModel::modelFolder(const QString& modelPath)
{
    return directoryOf(normalizeModelPath(modelPath)).toLower();
}

QVector<ModelFolder> ObjectWindowModel::getModelFolders(int categoryId) const
{
    QMap<QString, ModelFolder> folders; // lowercase path order: parent always precedes child
    int noFolderCount = 0;
    const int count = getRecordCountForCategory(categoryId);
    for (int i = 0; i < count; ++i)
    {
        const QString dir = directoryOf(normalizeModelPath(getVisibleRecordModelPath(categoryId, i)));
        if (dir.isEmpty())
        {
            ++noFolderCount;
            continue;
        }
        int start = 0;
        while (start <= dir.size())
        {
            int end = dir.indexOf(QLatin1Char('/'), start);
            if (end < 0)
                end = dir.size();
            ModelFolder& f = folders[dir.left(end).toLower()];
            if (f.path.isEmpty())
            {
                f.path = dir.left(end).toLower();
                f.name = dir.mid(start, end - start);
            }
            ++f.count;
            start = end + 1;
        }
    }
    QVector<ModelFolder> result = folders.values().toVector();
    if (!result.isEmpty() && noFolderCount > 0)
    {
        ModelFolder none;
        none.path = NoFolder;
        none.name = QStringLiteral("(no folder)");
        none.count = noFolderCount;
        result.prepend(none);
    }
    return result;
}
