#include "objectwindow.hpp"

#include "../world/data.hpp"
#include "../world/collection.hpp"
#include "../world/idcollection.hpp"

ObjectWindowModel::ObjectWindowModel(QObject* parent)
    : QAbstractItemModel(parent),
      mData(nullptr)
{
}

void ObjectWindowModel::setData(Data* data)
{
    beginResetModel();

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
    auto addCategory = [this, data](const QString& name, CkId::Type typeId) {
        Category cat;
        cat.name = name;
        cat.typeId = static_cast<int>(typeId);
        cat.totalRecords = 0;

        switch (typeId)
        {
        case CkId::Type_Gmst:
            cat.totalRecords = data->getGameSettings().size();
            break;
        case CkId::Type_Npc_:
            cat.totalRecords = data->getNpcCollection().size();
            break;
        case CkId::Type_Weap_:
            cat.totalRecords = data->getWeaponCollection().size();
            break;
        case CkId::Type_Armor_:
            cat.totalRecords = data->getArmorCollection().size();
            break;
        case CkId::Type_Spel_:
            cat.totalRecords = data->getSpellCollection().size();
            break;
        case CkId::Type_Magic_:
            cat.totalRecords = data->getMagicCollection().size();
            break;
        case CkId::Type_Quest_:
            cat.totalRecords = data->getQuestCollection().size();
            break;
        case CkId::Type_Dial_:
            cat.totalRecords = data->getDialCollection().size();
            break;
        case CkId::Type_Info_:
            cat.totalRecords = data->getInfoCollection().size();
            break;
        case CkId::Type_Glob_:
            cat.totalRecords = data->getGlobCollection().size();
            break;
        case CkId::Type_Lcrt_:
            cat.totalRecords = data->getLcrtCollection().size();
            break;
        case CkId::Type_Pack_:
            cat.totalRecords = data->getPackCollection().size();
            break;
        case CkId::Type_Tree_:
            cat.totalRecords = data->getTreeCollection().size();
            break;
        case CkId::Type_Alch_:
            cat.totalRecords = data->getAlchCollection().size();
            break;
        case CkId::Type_Ingr_:
            cat.totalRecords = data->getIngrCollection().size();
            break;
        case CkId::Type_Cont_:
            cat.totalRecords = data->getContCollection().size();
            break;
        case CkId::Type_Ench_:
            cat.totalRecords = data->getEnchCollection().size();
            break;
        case CkId::Type_Book_:
            cat.totalRecords = data->getBookCollection().size();
            break;
        case CkId::Type_Misc_:
            cat.totalRecords = data->getMiscCollection().size();
            break;
        case CkId::Type_Acti_:
            cat.totalRecords = data->getActiCollection().size();
            break;
        case CkId::Type_Stat_:
            cat.totalRecords = data->getStatCollection().size();
            break;
        case CkId::Type_Race_:
            cat.totalRecords = data->getRaceCollection().size();
            break;
        case CkId::Type_Class_:
            cat.totalRecords = data->getClassCollection().size();
            break;
        case CkId::Type_Fact_:
            cat.totalRecords = data->getFactCollection().size();
            break;
        case CkId::Type_PerK_:
            cat.totalRecords = data->getPerkCollection().size();
            break;
        case CkId::Type_Soun_:
            cat.totalRecords = data->getSounCollection().size();
            break;
        case CkId::Type_Wthr_:
            cat.totalRecords = data->getWthrCollection().size();
            break;
        case CkId::Type_Ltex_:
            cat.totalRecords = data->getLtexCollection().size();
            break;
        default:
            break;
        }

        for (int i = 0; i < cat.totalRecords; i++)
        {
            VisibleRecord rec;
            rec.actualIndex = i;

            QString editorId;
            QString formId;

            switch (typeId)
            {
            case CkId::Type_Gmst:
                editorId = data->getGameSettings().getId(i);
                formId = QString();
                break;
            case CkId::Type_Npc_:
                editorId = data->getNpcCollection().getId(i);
                formId = formatFormId(data->getNpcCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Weap_:
                editorId = data->getWeaponCollection().getId(i);
                formId = formatFormId(data->getWeaponCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Armor_:
                editorId = data->getArmorCollection().getId(i);
                formId = formatFormId(data->getArmorCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Spel_:
                editorId = data->getSpellCollection().getId(i);
                formId = formatFormId(data->getSpellCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Magic_:
                editorId = data->getMagicCollection().getId(i);
                formId = formatFormId(data->getMagicCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Quest_:
                editorId = data->getQuestCollection().getId(i);
                formId = formatFormId(data->getQuestCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Dial_:
                editorId = data->getDialCollection().getId(i);
                formId = formatFormId(data->getDialCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Info_:
                editorId = data->getInfoCollection().getId(i);
                formId = formatFormId(data->getInfoCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Glob_:
                editorId = data->getGlobCollection().getId(i);
                formId = QString();
                break;
            case CkId::Type_Lcrt_:
                editorId = data->getLcrtCollection().getId(i);
                formId = QString();
                break;
            case CkId::Type_Pack_:
                editorId = data->getPackCollection().getId(i);
                formId = formatFormId(data->getPackCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Tree_:
                editorId = data->getTreeCollection().getId(i);
                formId = formatFormId(data->getTreeCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Alch_:
                editorId = data->getAlchCollection().getId(i);
                formId = formatFormId(data->getAlchCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Ingr_:
                editorId = data->getIngrCollection().getId(i);
                formId = formatFormId(data->getIngrCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Cont_:
                editorId = data->getContCollection().getId(i);
                formId = formatFormId(data->getContCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Ench_:
                editorId = data->getEnchCollection().getId(i);
                formId = formatFormId(data->getEnchCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Book_:
                editorId = data->getBookCollection().getId(i);
                formId = formatFormId(data->getBookCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Misc_:
                editorId = data->getMiscCollection().getId(i);
                formId = formatFormId(data->getMiscCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Acti_:
                editorId = data->getActiCollection().getId(i);
                formId = formatFormId(data->getActiCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Stat_:
                editorId = data->getStatCollection().getId(i);
                formId = formatFormId(data->getStatCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Race_:
                editorId = data->getRaceCollection().getId(i);
                formId = formatFormId(data->getRaceCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Class_:
                editorId = data->getClassCollection().getId(i);
                formId = formatFormId(data->getClassCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Fact_:
                editorId = data->getFactCollection().getId(i);
                formId = formatFormId(data->getFactCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_PerK_:
                editorId = data->getPerkCollection().getId(i);
                formId = formatFormId(data->getPerkCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Soun_:
                editorId = data->getSounCollection().getId(i);
                formId = formatFormId(data->getSounCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Wthr_:
                editorId = data->getWthrCollection().getId(i);
                formId = formatFormId(data->getWthrCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Ltex_:
                editorId = data->getLtexCollection().getId(i);
                formId = formatFormId(data->getLtexCollection().getRecord(i).get().formId);
                break;
            default:
                break;
            }

            rec.editorId = editorId;
            rec.formId = formId;
            cat.visibleRecords.append(rec);
        }

        mCategories.append(cat);
    };

    addCategory("NPC", CkId::Type_Npc_);

    addCategory("Armor", CkId::Type_Armor_);
    addCategory("Weapon", CkId::Type_Weap_);
    addCategory("Alchemy", CkId::Type_Alch_);
    addCategory("Ingredient", CkId::Type_Ingr_);
    addCategory("Book", CkId::Type_Book_);
    addCategory("Miscellaneous", CkId::Type_Misc_);
    addCategory("Container", CkId::Type_Cont_);
    addCategory("Enchantment", CkId::Type_Ench_);
    addCategory("Spell", CkId::Type_Spel_);
    addCategory("Magic Effect", CkId::Type_Magic_);

    addCategory("Static", CkId::Type_Stat_);
    addCategory("Activator", CkId::Type_Acti_);
    addCategory("Tree Node", CkId::Type_Tree_);

    addCategory("Quest", CkId::Type_Quest_);
    addCategory("Package", CkId::Type_Pack_);
    addCategory("Global Variable", CkId::Type_Glob_);
    addCategory("Game Settings", CkId::Type_Gmst);
    addCategory("Perk", CkId::Type_PerK_);
    addCategory("Class", CkId::Type_Class_);
    addCategory("Faction", CkId::Type_Fact_);
    addCategory("Race", CkId::Type_Race_);

    addCategory("Sound", CkId::Type_Soun_);

    addCategory("Dialogue", CkId::Type_Dial_);
    addCategory("Information", CkId::Type_Info_);

    addCategory("Weather", CkId::Type_Wthr_);
    addCategory("Land Texture", CkId::Type_Ltex_);
    addCategory("Location Reference Type", CkId::Type_Lcrt_);

    auto addGroup = [this](const QString& name, std::initializer_list<CkId::Type> types) {
        CategoryGroup group;
        group.name = name;
        for (CkId::Type t : types)
        {
            for (int i = 0; i < mCategories.size(); i++)
            {
                if (mCategories[i].typeId == static_cast<int>(t))
                {
                    group.categoryIndices.append(i);
                    break;
                }
            }
        }
        mGroups.append(group);
    };

    addGroup("Actors", {CkId::Type_Npc_});
    addGroup("Items", {CkId::Type_Armor_, CkId::Type_Weap_, CkId::Type_Alch_, CkId::Type_Ingr_,
                       CkId::Type_Book_, CkId::Type_Misc_, CkId::Type_Cont_, CkId::Type_Ench_,
                       CkId::Type_Spel_, CkId::Type_Magic_});
    addGroup("World Objects", {CkId::Type_Stat_, CkId::Type_Acti_, CkId::Type_Tree_});
    addGroup("Gameplay", {CkId::Type_Quest_, CkId::Type_Pack_, CkId::Type_Glob_, CkId::Type_Gmst,
                          CkId::Type_PerK_, CkId::Type_Class_, CkId::Type_Fact_, CkId::Type_Race_});
    addGroup("Audio", {CkId::Type_Soun_});
    addGroup("Dialogue", {CkId::Type_Dial_, CkId::Type_Info_});
    addGroup("World", {CkId::Type_Wthr_, CkId::Type_Ltex_, CkId::Type_Lcrt_});
}

QString ObjectWindowModel::formatFormId(quint32 formId) const
{
    return QString("0x%1").arg(formId, 8, 16, QChar('0'));
}

void ObjectWindowModel::applyFilter(const QString& text)
{
    mFilter = text;
    QString lowerFilter = text.toLower();

    for (auto& cat : mCategories)
    {
        if (lowerFilter.isEmpty())
        {
            cat.visibleRecords.clear();
            for (int i = 0; i < cat.totalRecords; i++)
            {
                VisibleRecord rec;
                rec.actualIndex = i;
                rec.editorId = "";
                rec.formId = "";

                switch (static_cast<CkId::Type>(cat.typeId))
                {
                case CkId::Type_Gmst:
                    rec.editorId = mData->getGameSettings().getId(i);
                    break;
                case CkId::Type_Npc_:
                    rec.editorId = mData->getNpcCollection().getId(i);
                    rec.formId = formatFormId(mData->getNpcCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Weap_:
                    rec.editorId = mData->getWeaponCollection().getId(i);
                    rec.formId = formatFormId(mData->getWeaponCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Armor_:
                    rec.editorId = mData->getArmorCollection().getId(i);
                    rec.formId = formatFormId(mData->getArmorCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Spel_:
                    rec.editorId = mData->getSpellCollection().getId(i);
                    rec.formId = formatFormId(mData->getSpellCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Magic_:
                    rec.editorId = mData->getMagicCollection().getId(i);
                    rec.formId = formatFormId(mData->getMagicCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Quest_:
                    rec.editorId = mData->getQuestCollection().getId(i);
                    rec.formId = formatFormId(mData->getQuestCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Dial_:
                    rec.editorId = mData->getDialCollection().getId(i);
                    rec.formId = formatFormId(mData->getDialCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Info_:
                    rec.editorId = mData->getInfoCollection().getId(i);
                    rec.formId = formatFormId(mData->getInfoCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Glob_:
                    rec.editorId = mData->getGlobCollection().getId(i);
                    rec.formId = QString();
                    break;
                case CkId::Type_Lcrt_:
                    rec.editorId = mData->getLcrtCollection().getId(i);
                    rec.formId = QString();
                    break;
                case CkId::Type_Pack_:
                    rec.editorId = mData->getPackCollection().getId(i);
                    rec.formId = formatFormId(mData->getPackCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Tree_:
                    rec.editorId = mData->getTreeCollection().getId(i);
                    rec.formId = formatFormId(mData->getTreeCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Alch_:
                    rec.editorId = mData->getAlchCollection().getId(i);
                    rec.formId = formatFormId(mData->getAlchCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Ingr_:
                    rec.editorId = mData->getIngrCollection().getId(i);
                    rec.formId = formatFormId(mData->getIngrCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Cont_:
                    rec.editorId = mData->getContCollection().getId(i);
                    rec.formId = formatFormId(mData->getContCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Ench_:
                    rec.editorId = mData->getEnchCollection().getId(i);
                    rec.formId = formatFormId(mData->getEnchCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Book_:
                    rec.editorId = mData->getBookCollection().getId(i);
                    rec.formId = formatFormId(mData->getBookCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Misc_:
                    rec.editorId = mData->getMiscCollection().getId(i);
                    rec.formId = formatFormId(mData->getMiscCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Acti_:
                    rec.editorId = mData->getActiCollection().getId(i);
                    rec.formId = formatFormId(mData->getActiCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Stat_:
                    rec.editorId = mData->getStatCollection().getId(i);
                    rec.formId = formatFormId(mData->getStatCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Race_:
                    rec.editorId = mData->getRaceCollection().getId(i);
                    rec.formId = formatFormId(mData->getRaceCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Class_:
                    rec.editorId = mData->getClassCollection().getId(i);
                    rec.formId = formatFormId(mData->getClassCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Fact_:
                    rec.editorId = mData->getFactCollection().getId(i);
                    rec.formId = formatFormId(mData->getFactCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_PerK_:
                    rec.editorId = mData->getPerkCollection().getId(i);
                    rec.formId = formatFormId(mData->getPerkCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Soun_:
                    rec.editorId = mData->getSounCollection().getId(i);
                    rec.formId = formatFormId(mData->getSounCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Wthr_:
                    rec.editorId = mData->getWthrCollection().getId(i);
                    rec.formId = formatFormId(mData->getWthrCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Ltex_:
                    rec.editorId = mData->getLtexCollection().getId(i);
                    rec.formId = formatFormId(mData->getLtexCollection().getRecord(i).get().formId);
                    break;
                default:
                    break;
                }

                cat.visibleRecords.append(rec);
            }
        }
        else
        {
            QVector<VisibleRecord> filtered;
            for (const auto& rec : cat.visibleRecords)
            {
                if (rec.editorId.toLower().contains(lowerFilter) ||
                    rec.formId.toLower().contains(lowerFilter))
                {
                    filtered.append(rec);
                }
            }
            cat.visibleRecords = filtered;
        }
    }

    emit layoutChanged();
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

int ObjectWindowModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 3;
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
            return cat.name;
        case 1:
            return QString();
        case 2:
            return CkId(static_cast<CkId::Type>(cat.typeId)).getTypeName();
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
        return CkId(static_cast<CkId::Type>(mCategories[flatId].typeId)).getTypeName();
    }
    return QVariant();
}

Qt::ItemFlags ObjectWindowModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::ItemFlag::ItemIsDropEnabled;

    if (isRecordNode(index))
        return QAbstractItemModel::flags(index) | Qt::ItemFlag::ItemIsSelectable;

    return QAbstractItemModel::flags(index) | Qt::ItemFlag::ItemIsEnabled;
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
