#include "ckid.hpp"

#include <sstream>
#include <stdexcept>

struct TypeData
{
    CkId::Type type;
    const char* name;
};

static const TypeData typesIdArg[] =
{
    { CkId::Type_Gmst, "Game Setting"},
    { CkId::Type_Npc_, "Actor" },
    { CkId::Type_Weap_, "Weapon" },
    { CkId::Type_Armor_, "Armor" },
    { CkId::Type_Spel_, "Spell" },
    { CkId::Type_Magic_, "Magic Effect" },
    { CkId::Type_Quest_, "Quest" },
    { CkId::Type_Dial_, "Dialogue" },
    { CkId::Type_Info_, "Info" },
    { CkId::Type_Glob_, "Global" },
    { CkId::Type_Lcrt_, "Leveled Creature" },
    { CkId::Type_Pack_, "Package" },
    { CkId::Type_Tree_, "Leveled Creature Group" },
    { CkId::Type_Alch_, "Alchemy" },
    { CkId::Type_Ingr_, "Ingredient" },
    { CkId::Type_Cont_, "Container" },
    { CkId::Type_Ench_, "Enchantment" },
    { CkId::Type_Book_, "Book" },
    { CkId::Type_Misc_, "Misc" },
    { CkId::Type_Acti_, "Activator" },
    { CkId::Type_Stat_, "Static" },
    { CkId::Type_Race_, "Race" },
    { CkId::Type_Class_, "Class" },
    { CkId::Type_Fact_, "Faction" },
    { CkId::Type_PerK_, "Perk" },
    { CkId::Type_Cel_, "Cell" },
    { CkId::Type_WRLD_, "Worldspace" },
    { CkId::Type_LOCT_, "Location" },
    { CkId::Type_Refr_, "Refr" },
    { CkId::Type_Material_, "Material" },
    { CkId::Type_Land_, "Landscape" },
    { CkId::Type_Soun_, "Sound" },
    { CkId::Type_Wthr_, "Weather" },
    { CkId::Type_Ltex_, "Land Texture" },
    { CkId::Type_None, 0 }
};

static const TypeData typesNoArg[] =
{
    { CkId::Type_Gmst, "Game Settings" },
    { CkId::Type_Npc_, "Actors" },
    { CkId::Type_Weap_, "Weapons" },
    { CkId::Type_Armor_, "Armors" },
    { CkId::Type_Spel_, "Spells" },
    { CkId::Type_Magic_, "Magic Effects" },
    { CkId::Type_Quest_, "Quests" },
    { CkId::Type_Dial_, "Dialogues" },
    { CkId::Type_Info_, "Infos" },
    { CkId::Type_Glob_, "Globals" },
    { CkId::Type_Lcrt_, "Leveled Creatures" },
    { CkId::Type_Pack_, "Packages" },
    { CkId::Type_Tree_, "Leveled Creature Groups" },
    { CkId::Type_Alch_, "Alchemy" },
    { CkId::Type_Ingr_, "Ingredients" },
    { CkId::Type_Cont_, "Containers" },
    { CkId::Type_Ench_, "Enchantments" },
    { CkId::Type_Book_, "Books" },
    { CkId::Type_Misc_, "Misc" },
    { CkId::Type_Acti_, "Activators" },
    { CkId::Type_Stat_, "Statics" },
    { CkId::Type_Race_, "Races" },
    { CkId::Type_Class_, "Classes" },
    { CkId::Type_Fact_, "Factions" },
    { CkId::Type_PerK_, "Perks" },
    { CkId::Type_Cel_, "Cells" },
    { CkId::Type_WRLD_, "Worldspaces" },
    { CkId::Type_LOCT_, "Locations" },
    { CkId::Type_Refr_, "Refs" },
    { CkId::Type_Material_, "Materials" },
    { CkId::Type_Land_, "Landscapes" },
    { CkId::Type_Soun_, "Sounds" },
    { CkId::Type_Wthr_, "Weather" },
    { CkId::Type_Ltex_, "Land Textures" },
    { CkId::Type_None, 0 }
};

static const TypeData typesIndexArg[] =
{
    { CkId::Type_LoadingLog, "Loading Error Log" },
    { CkId::Type_None, 0 }
};

CkId::CkId(const QString& ckid) :
    index(0), argumentType(ArgumentType_None), type(Type_None)
{
    QString::size_type sepPos = ckid.indexOf(':');

    if (sepPos != -1)
    {
        QString type_ = ckid.mid(0, sepPos);

        for (int i = 0; typesIdArg[i].name; ++i)
        {
            if (type_ == typesIdArg[i].name)
            {
                argumentType = ArgumentType_Id;
                type = typesIdArg[i].type;
                id = ckid.mid(sepPos + 1, ckid.size());
                return;
            }
        }

        for (int i = 0; typesIndexArg[i].name; ++i)
        {
            if (type_ == typesIndexArg[i].name)
            {
                argumentType = ArgumentType_Index;
                type = typesIndexArg[i].type;
                id = ckid.mid(sepPos + 1, ckid.size());
                return;
            }
        }
    }
    else
    {
        for (int i = 0; typesNoArg[i].name; ++i)
        {
            if (ckid == typesNoArg[i].name)
            {
                argumentType = ArgumentType_None;
                type = typesNoArg[i].type;
                id = ckid.mid(sepPos + 1, ckid.size());
                return;
            }
        }
    }
}

CkId::CkId(Type type) :
    index(0), argumentType(ArgumentType_None), type(type)
{
    for (int i = 0; typesIdArg[i].name; ++i)
    {
        if (type == typesIdArg[i].type)
        {
            argumentType = ArgumentType_Id;
            return;
        }
    }

    for (int i = 0; typesIndexArg[i].name; ++i)
    {
        if (type == typesIndexArg[i].type)
        {
            argumentType = ArgumentType_Index;
            return;
        }
    }
}

CkId::CkId(Type type, const QString& id) :
    index(0), argumentType(ArgumentType_Id), type(type), id(id)
{
}

CkId::CkId(Type type, int index) :
    index(index), argumentType(ArgumentType_Index), type(type)
{
}

CkId::ArgumentType CkId::getArgumentType() const
{
    return argumentType;
}

CkId::Type CkId::getType() const
{
    return type;
}

const QString& CkId::getId() const
{
    if (argumentType != ArgumentType_Id)
    {
        throw std::runtime_error("Invalid access of ID from CKID with no ID");
    }

    return id;
}

int CkId::getIndex() const
{
    if (argumentType != ArgumentType_Index)
    {
        throw std::runtime_error("Invalid access of index from CKID with no index");
    }

    return index;
}

bool CkId::equalTo(const CkId& ckid) const
{
    if (argumentType != ckid.argumentType || type != ckid.type)
    {
        return false;
    }

    switch (argumentType)
    {
    case (ArgumentType_Id):
    {
        return id == ckid.id;
    }
    case (ArgumentType_Index):
    {
        return index == ckid.index;
    }
    default:
    {
        return true;
    }
    }
}

bool CkId::lessThan(const CkId& ckid) const
{
    if (type < ckid.type)
    {
        return true;
    }
    if (type > ckid.type)
    {
        return false;
    }

    switch (argumentType)
    {
    case (ArgumentType_Id):
    {
        return id < ckid.id;
    }
    case (ArgumentType_Index):
    {
        return index < ckid.index;
    }
    default:
    {
        return false;
    }
    }
}

QString CkId::getTypeName() const
{
    const TypeData* typeData = typesNoArg;

    if (argumentType == ArgumentType_Id)
    {
        typeData = typesIdArg;
    }
    else if (argumentType == ArgumentType_Index)
    {
        typeData = typesIndexArg;
    }

    for (int i = 0; typeData[i].name; ++i)
    {
        if (typeData[i].type == type)
        {
            return typeData[i].name;
        }
    }

    throw std::runtime_error("Failed to get CKID type name");
}

QString CkId::toString() const
{
    std::ostringstream str;
    str << getTypeName().toStdString();

    switch (argumentType)
    {
    case ArgumentType_None:        break;
    case ArgumentType_Id:        str << ": " << id.toStdString(); break;
    case ArgumentType_Index:    str << ": " << index; break;
    }

    return QString(str.str().c_str());
}

bool operator== (const CkId& left, const CkId& right)
{
    return left.equalTo(right);
}

bool operator!= (const CkId& left, const CkId& right)
{
    return !left.equalTo(right);
}

bool operator< (const CkId& left, const CkId& right)
{
    return left.lessThan(right);
}

CkId::Type CkId::stringToType(const QString& typeName)
{
    for (int i = 0; typesNoArg[i].name; ++i)
    {
        if (typeName == typesNoArg[i].name)
        {
            return typesNoArg[i].type;
        }
    }
    return Type_None;
}