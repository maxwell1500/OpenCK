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
    { CkId::Type_Plnt_, "Planet" },
    { CkId::Type_Refr_, "Refr" },
    { CkId::Type_Material_, "Material" },
    { CkId::Type_Land_, "Landscape" },
    { CkId::Type_Soun_, "Sound" },
    { CkId::Type_Wthr_, "Weather" },
    { CkId::Type_Ltex_, "Land Texture" },
    { CkId::Type_Ammo_, "Ammo" },
    { CkId::Type_Appa_, "Apparatus" },
    { CkId::Type_Avif_, "Actor Value" },
    { CkId::Type_Bsgn_, "Birthsign" },
    { CkId::Type_Clmt_, "Climate" },
    { CkId::Type_Clot_, "Clothing" },
    { CkId::Type_Cobj_, "Constructible Object" },
    { CkId::Type_Crea_, "Creature" },
    { CkId::Type_Csty_, "Combat Style" },
    { CkId::Type_Door_, "Door" },
    { CkId::Type_Efsh_, "Effect Shader" },
    { CkId::Type_Expl_, "Explosion" },
    { CkId::Type_Eyes_, "Eyes" },
    { CkId::Type_Flor_, "Flora" },
    { CkId::Type_Flst_, "Form List" },
    { CkId::Type_Furn_, "Furniture" },
    { CkId::Type_Grass_, "Grass" },
    { CkId::Type_Hair_, "Hair" },
    { CkId::Type_Idle_, "Idle Animation" },
    { CkId::Type_Idlm_, "Idle Marker" },
    { CkId::Type_Imgs_, "Image Space" },
    { CkId::Type_Keym_, "Key" },
    { CkId::Type_Kywd_, "Keyword" },
    { CkId::Type_Ligh_, "Light" },
    { CkId::Type_Lscr_, "Load Screen" },
    { CkId::Type_Lvlc_, "Leveled Creature" },
    { CkId::Type_Lvli_, "Leveled Item" },
    { CkId::Type_Lvsp_, "Leveled Spell" },
    { CkId::Type_Mesg_, "Message" },
    { CkId::Type_Mstt_, "Movable Static" },
    { CkId::Type_Navm_, "Navmesh" },
    { CkId::Type_Note_, "Note" },
    { CkId::Type_Otft_, "Outfit" },
    { CkId::Type_Proj_, "Projectile" },
    { CkId::Type_Regn_, "Region" },
    { CkId::Type_Road_, "Road" },
    { CkId::Type_Scpt_, "Script" },
    { CkId::Type_Scrl_, "Scroll" },
    { CkId::Type_Slgm_, "Soul Gem" },
    { CkId::Type_Smqn_, "Sound Marker" },
    { CkId::Type_Spgd_, "Shader Particle Geometry" },
    { CkId::Type_Scol_, "Static Collection" },
    { CkId::Type_Scen_, "Scene" },
    { CkId::Type_Txst_, "Texture Set" },
    { CkId::Type_Wate_, "Water" },
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
    { CkId::Type_Plnt_, "Planets" },
    { CkId::Type_Refr_, "Refs" },
    { CkId::Type_Material_, "Materials" },
    { CkId::Type_Land_, "Landscapes" },
    { CkId::Type_Soun_, "Sounds" },
    { CkId::Type_Wthr_, "Weather" },
    { CkId::Type_Ltex_, "Land Textures" },
    { CkId::Type_Ammo_, "Ammo" },
    { CkId::Type_Appa_, "Apparatus" },
    { CkId::Type_Avif_, "Actor Values" },
    { CkId::Type_Bsgn_, "Birthsigns" },
    { CkId::Type_Clmt_, "Climates" },
    { CkId::Type_Clot_, "Clothing" },
    { CkId::Type_Cobj_, "Constructible Objects" },
    { CkId::Type_Crea_, "Creatures" },
    { CkId::Type_Csty_, "Combat Styles" },
    { CkId::Type_Door_, "Doors" },
    { CkId::Type_Efsh_, "Effect Shaders" },
    { CkId::Type_Expl_, "Explosions" },
    { CkId::Type_Eyes_, "Eyes" },
    { CkId::Type_Flor_, "Flora" },
    { CkId::Type_Flst_, "Form Lists" },
    { CkId::Type_Furn_, "Furniture" },
    { CkId::Type_Grass_, "Grass" },
    { CkId::Type_Hair_, "Hair" },
    { CkId::Type_Idle_, "Idle Animations" },
    { CkId::Type_Idlm_, "Idle Markers" },
    { CkId::Type_Imgs_, "Image Spaces" },
    { CkId::Type_Keym_, "Keys" },
    { CkId::Type_Kywd_, "Keywords" },
    { CkId::Type_Ligh_, "Lights" },
    { CkId::Type_Lscr_, "Load Screens" },
    { CkId::Type_Lvlc_, "Leveled Creatures" },
    { CkId::Type_Lvli_, "Leveled Items" },
    { CkId::Type_Lvsp_, "Leveled Spells" },
    { CkId::Type_Mesg_, "Messages" },
    { CkId::Type_Mstt_, "Movable Statics" },
    { CkId::Type_Navm_, "Navmeshes" },
    { CkId::Type_Note_, "Notes" },
    { CkId::Type_Otft_, "Outfits" },
    { CkId::Type_Proj_, "Projectiles" },
    { CkId::Type_Regn_, "Regions" },
    { CkId::Type_Road_, "Roads" },
    { CkId::Type_Scpt_, "Scripts" },
    { CkId::Type_Scrl_, "Scrolls" },
    { CkId::Type_Slgm_, "Soul Gems" },
    { CkId::Type_Smqn_, "Sound Markers" },
    { CkId::Type_Spgd_, "Shader Particle Geometries" },
    { CkId::Type_Scol_, "Static Collections" },
    { CkId::Type_Scen_, "Scenes" },
    { CkId::Type_Txst_, "Texture Sets" },
    { CkId::Type_Wate_, "Water" },
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
    // Fall back to the on-disk record name (e.g. "NPC_", "WEAP_", "MGEF").
    // These are not in the friendly-name tables but correspond 1:1 to the
    // Type_ enum constant suffix.
    const QString diskRaw = typeName.trimmed().toUpper();
    QString disk = diskRaw;
    // Some callers pass a trailing underscore ("WEAP_"); strip it for the
    // 4CC comparison.
    if (disk.endsWith('_'))
        disk.chop(1);
    if (!disk.isEmpty())
    {
        static const struct { CkId::Type type; const char* disk; } diskAliases[] = {
            { Type_Gmst, "GMST" }, { Type_Npc_, "NPC" }, { Type_Weap_, "WEAP" },
            { Type_Armor_, "ARMO" }, { Type_Spel_, "SPEL" }, { Type_Magic_, "MGEF" },
            { Type_Quest_, "QUST" }, { Type_Dial_, "DIAL" }, { Type_Info_, "INFO" },
            { Type_Glob_, "GLOB" }, { Type_Lcrt_, "LCRT" }, { Type_Pack_, "PACK" },
            { Type_Tree_, "TREE" }, { Type_Alch_, "ALCH" }, { Type_Ingr_, "INGR" },
            { Type_Cont_, "CONT" }, { Type_Ench_, "ENCH" }, { Type_Book_, "BOOK" },
            { Type_Misc_, "MISC" }, { Type_Acti_, "ACTI" }, { Type_Stat_, "STAT" },
            { Type_Race_, "RACE" }, { Type_Class_, "CLAS" }, { Type_Fact_, "FACT" },
            { Type_PerK_, "PERK" }, { Type_Cel_, "CELL" }, { Type_WRLD_, "WRLD" },
            { Type_LOCT_, "LCTN" }, { Type_Plnt_, "PNDT" }, { Type_Refr_, "REFR" }, { Type_Material_, "MATL" },
            { Type_Land_, "LAND" }, { Type_Soun_, "SOUN" }, { Type_Wthr_, "WTHR" },
            { Type_Ltex_, "LTEX" }, { Type_Ammo_, "AMMO" }, { Type_Appa_, "APPA" },
            { Type_Avif_, "AVIF" }, { Type_Bsgn_, "BSGN" }, { Type_Clmt_, "CLMT" },
            { Type_Clot_, "CLOT" }, { Type_Cobj_, "COBJ" }, { Type_Crea_, "CREA" },
            { Type_Csty_, "CSTY" }, { Type_Door_, "DOOR" }, { Type_Efsh_, "EFSH" },
            { Type_Expl_, "EXPL" }, { Type_Eyes_, "EYES" }, { Type_Flor_, "FLOR" },
            { Type_Flst_, "FLST" }, { Type_Furn_, "FURN" }, { Type_Grass_, "GRAS" },
            { Type_Hair_, "HAIR" }, { Type_Idle_, "IDLE" }, { Type_Idlm_, "IDLM" },
        };
        for (const auto& alias : diskAliases)
        {
            if (disk == QLatin1String(alias.disk))
            {
                return alias.type;
            }
        }
    }
    return Type_None;
}