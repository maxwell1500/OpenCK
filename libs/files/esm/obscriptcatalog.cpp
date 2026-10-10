#include "obscriptcatalog.hpp"

namespace ObScript
{

namespace
{

using FP = NativeFunction;

// A signature list: name, parameter types, return type, property flag.
struct NativeEntry
{
    const char* name;
    ValueType ret;
    std::initializer_list<ValueType> params;
    bool property = false;
    /// Number of leading parameters that may be omitted by a caller. Papyrus
    /// gives many native functions default argument values; without this a
    /// `wait(1.0)` call would be reported as an arity error.
    int optionalParams = 0;
};

// ---------------------------------------------------------------------------
// Skyrim SE / Anniversary Edition
// ---------------------------------------------------------------------------
// Curated from the public Papyrus reference for Skyrim SE. Signatures are
// given as value types; parameter names are omitted because they are not used
// for validation.
constexpr NativeEntry kSkyrimSE[] = {
    // --- core values / math ---
    {"print", ValueType::Nil, {ValueType::String}},
    {"messagebox", ValueType::Nil, {ValueType::String}},
    {"abs", ValueType::Float, {ValueType::Float}},
    {"rnd", ValueType::Int, {ValueType::Int}},
    {"round", ValueType::Int, {ValueType::Float}},
    {"ceil", ValueType::Int, {ValueType::Float}},
    {"floor", ValueType::Int, {ValueType::Float}},
    {"sqrt", ValueType::Float, {ValueType::Float}},
    {"pow", ValueType::Float, {ValueType::Float, ValueType::Float}},
    {"square", ValueType::Float, {ValueType::Float}},
    {"log", ValueType::Float, {ValueType::Float}},
    {"sin", ValueType::Float, {ValueType::Float}},
    {"cos", ValueType::Float, {ValueType::Float}},
    {"tan", ValueType::Float, {ValueType::Float}},
    {"asin", ValueType::Float, {ValueType::Float}},
    {"acos", ValueType::Float, {ValueType::Float}},
    {"atan", ValueType::Float, {ValueType::Float}},
    {"randomint", ValueType::Int, {ValueType::Int, ValueType::Int}},
    {"randomfloat", ValueType::Float, {ValueType::Float, ValueType::Float}},
    {"size", ValueType::Int, {ValueType::Form}},
    {"getnthmember", ValueType::Any, {ValueType::Any, ValueType::Int}},

    // --- runtime / game ---
    {"getgamerealtime", ValueType::Float, {}},
    {"getplaytime", ValueType::Float, {}},
    {"getsecondsplayed", ValueType::Int, {}},
    {"quitgame", ValueType::Bool, {ValueType::Bool}},
    {"wait", ValueType::Nil, {ValueType::Float, ValueType::Float}, false, 1},
    {"getcamerern", ValueType::Any, {ValueType::Int}},
    {"player", ValueType::Object, {}, true},
    {"self", ValueType::Object, {}, true},

    // --- actor / actorbase ---
    {"getactorvalue", ValueType::Float, {ValueType::String}},
    {"getbaseactorvalue", ValueType::Float, {ValueType::String}},
    {"setactorvalue", ValueType::Nil, {ValueType::String, ValueType::Float}},
    {"damageactorvalue", ValueType::Nil, {ValueType::String, ValueType::Float}},
    {"restoreactorvalue", ValueType::Nil, {ValueType::String, ValueType::Float}},
    {"getav", ValueType::Float, {ValueType::String}},
    {"getbaseav", ValueType::Float, {ValueType::String}},
    {"forceactorvalue", ValueType::Nil, {ValueType::String, ValueType::Float}},
    {"getlevel", ValueType::Int, {}},
    {"getdead", ValueType::Bool, {}},
    {"isdead", ValueType::Bool, {ValueType::Any}},
    {"isalive", ValueType::Bool, {ValueType::Any}},
    {"ischildactor", ValueType::Bool, {}},
    {"issneaking", ValueType::Bool, {}},
    {"isrunning", ValueType::Bool, {}},
    {"issprinting", ValueType::Bool, {}},
    {"playidle", ValueType::Bool, {ValueType::Any}},
    {"isessential", ValueType::Bool, {ValueType::Any}},

    // --- inventory ---
    {"additem", ValueType::Nil, {ValueType::Any, ValueType::Int, ValueType::Bool}},
    {"removeitem", ValueType::Nil, {ValueType::Any, ValueType::Int, ValueType::Bool}},
    {"removeallitems", ValueType::Nil, {ValueType::Object}},
    {"getitemcount", ValueType::Int, {ValueType::Any}},
    {"getnumitems", ValueType::Int, {}},
    {"getequippeditemtype", ValueType::Int, {}},
    {"equipitem", ValueType::Nil, {ValueType::Any, ValueType::Bool}},
    {"unequipitem", ValueType::Nil, {ValueType::Any, ValueType::Bool, ValueType::Bool}},
    {"equipspell", ValueType::Nil, {ValueType::Any}},
    {"unequipspell", ValueType::Nil, {ValueType::Any}},
    {"equipshout", ValueType::Nil, {ValueType::Any}},
    {"unequipshout", ValueType::Nil, {ValueType::Any}},
    {"getequippedspell", ValueType::Any, {ValueType::Int}},
    {"getequippedshout", ValueType::Any, {}},
    {"isweapondrawn", ValueType::Bool, {}},
    {"getweight", ValueType::Float, {}},

    // --- object / reference ---
    {"getpositionx", ValueType::Float, {}},
    {"getpositiony", ValueType::Float, {}},
    {"getpositionz", ValueType::Float, {}},
    {"getanglex", ValueType::Float, {}},
    {"getangley", ValueType::Float, {}},
    {"getanglez", ValueType::Float, {}},
    {"getscale", ValueType::Float, {}},
    {"setposition", ValueType::Bool,
        {ValueType::Float, ValueType::Float, ValueType::Float}},
    {"setangle", ValueType::Bool,
        {ValueType::Float, ValueType::Float, ValueType::Float}},
    {"setactorangle", ValueType::Bool,
        {ValueType::Float, ValueType::Float, ValueType::Float}},
    {"setpos", ValueType::Bool, {ValueType::Float, ValueType::Float, ValueType::Float}},
    {"moveto", ValueType::Bool, {ValueType::Any, ValueType::Float, ValueType::Float, ValueType::Float}},
    {"translate", ValueType::Bool, {ValueType::Float, ValueType::Float, ValueType::Float}},
    {"rotate", ValueType::Bool, {ValueType::Float, ValueType::Float, ValueType::Float}},
    {"getdistance", ValueType::Float, {ValueType::Any}},
    {"getdistancefromplayer", ValueType::Float, {}},
    {"playanimation", ValueType::Nil, {ValueType::String}},
    {"playimpacteffect", ValueType::Nil, {ValueType::String, ValueType::String, ValueType::Float}},
    {"playgamebryananimation", ValueType::Nil, {ValueType::String}},
    {"playgroups", ValueType::Nil, {ValueType::String, ValueType::Int}},
    {"playgroup", ValueType::Nil, {ValueType::String, ValueType::Int}},
    {"enable", ValueType::Bool, {ValueType::Bool}},
    {"disable", ValueType::Bool, {ValueType::Bool}},
    {"delete", ValueType::Nil, {}},
    {"lock", ValueType::Bool, {ValueType::Bool, ValueType::Bool}},
    {"islocked", ValueType::Bool, {}},
    {"activate", ValueType::Nil,
        {ValueType::Any, ValueType::Bool, ValueType::Bool, ValueType::Int, ValueType::Bool}},
    {"geteditorlocation", ValueType::String, {}},
    {"getcurrentdestination", ValueType::Any, {}},
    {"waituntilactoratposition", ValueType::Bool,
        {ValueType::Any, ValueType::Float, ValueType::Float, ValueType::Float}},

    // --- cells / worldspace ---
    {"getparentcell", ValueType::Any, {}},
    {"isinaninterior", ValueType::Bool, {}},
    {"getworldspace", ValueType::Any, {}},
    {"ismarker", ValueType::Bool, {}},
    {"ispcteleporter", ValueType::Bool, {}},

    // --- quest / aliases ---
    {"getquest", ValueType::Any, {ValueType::String}},
    {"startquest", ValueType::Nil, {ValueType::Any}},
    {"stopquest", ValueType::Nil, {ValueType::Any}},
    {"setstage", ValueType::Nil, {ValueType::Int}},
    {"setcurrentstageid", ValueType::Nil, {ValueType::Int}},
    {"getstage", ValueType::Int, {ValueType::Any}},
    {"getcurrentstageid", ValueType::Int, {ValueType::Any}},
    {"isqueststage", ValueType::Bool, {ValueType::Any, ValueType::Int}},
    {"isobjectivecompleted", ValueType::Bool, {ValueType::Int}},
    {"setobjectivecompleted", ValueType::Bool, {ValueType::Int, ValueType::Bool}},
    {"completeallobjectives", ValueType::Nil},

    // --- UI ---
    {"showmenu", ValueType::Nil, {ValueType::String}},
    {"sendmodEvent", ValueType::Nil, {ValueType::String}},

    // --- magic effects ---
    {"cast", ValueType::Bool, {ValueType::Any}},
    {"dispell", ValueType::Nil, {ValueType::Any, ValueType::Int}},
    {"getmagiceffect", ValueType::Any, {}, true},
    {"getmagnitude", ValueType::Float, {}, true},
    {"getduration", ValueType::Float, {}, true},
    {"getarea", ValueType::Float, {}, true},
};

void addSignature(NativeCatalog& catalog, const NativeEntry& e)
{
    NativeFunction fn;
    fn.name = QString::fromUtf8(e.name);
    if (e.property)
    {
        fn.isProperty = true;
    }
    else
    {
        int index = 0;
        for (ValueType t : e.params)
        {
            NativeParam p;
            p.type = t;
            p.name = fn.name;
            // Only the trailing parameters may be optional; a function with
            // optionalParams == 0 has none, and comparing `index >= 0` would
            // mark every parameter optional.
            p.optional = e.optionalParams > 0 && index >= e.optionalParams;
            fn.params.append(p);
            ++index;
        }
    }
    fn.returnType = e.ret;
    catalog.add(fn);
}

} // namespace

QString gameFlavorName(GameFlavor flavor)
{
    switch (flavor)
    {
    case GameFlavor::Morrowind:   return QStringLiteral("Morrowind");
    case GameFlavor::Oblivion:    return QStringLiteral("Oblivion");
    case GameFlavor::Skyrim:      return QStringLiteral("Skyrim");
    case GameFlavor::FalloutNV:   return QStringLiteral("Fallout: New Vegas");
    case GameFlavor::SkyrimSE:    return QStringLiteral("Skyrim Special Edition");
    case GameFlavor::Fallout4:    return QStringLiteral("Fallout 4");
    case GameFlavor::Fallout76:   return QStringLiteral("Fallout 76");
    case GameFlavor::Starfield:   return QStringLiteral("Starfield");
    case GameFlavor::Unknown:     break;
    }
    return QStringLiteral("Unknown");
}

GameFlavor gameFlavorFromName(const QString& name)
{
    const QString n = name.toLower();
    if (n.contains(QStringLiteral("morrowind")))  return GameFlavor::Morrowind;
    if (n.contains(QStringLiteral("oblivion")))    return GameFlavor::Oblivion;
    if (n.contains(QStringLiteral("special edition"))
        || n.contains(QStringLiteral("skyrim se"))
        || n.contains(QStringLiteral("skyrimse"))
        || n.contains(QStringLiteral("anniversary")))
        return GameFlavor::SkyrimSE;
    if (n.contains(QStringLiteral("skyrim")))       return GameFlavor::Skyrim;
    if (n.contains(QStringLiteral("fallout 4"))
        || n.contains(QStringLiteral("fallout4"))
        || n.contains(QStringLiteral("fallout iv")))
        return GameFlavor::Fallout4;
    if (n.contains(QStringLiteral("new vegas")))     return GameFlavor::FalloutNV;
    if (n.contains(QStringLiteral("fallout 76")))   return GameFlavor::Fallout76;
    if (n.contains(QStringLiteral("starfield")))    return GameFlavor::Starfield;
    return GameFlavor::Unknown;
}

bool hasKnownSurface(GameFlavor flavor)
{
    // Only Skyrim SE has a curated specific surface so far; every other
    // flavor validates against the cross-game builtins and reports unknown
    // natives as warnings rather than errors.
    return flavor == GameFlavor::SkyrimSE;
}

NativeCatalog nativeCatalogFor(GameFlavor flavor)
{
    NativeCatalog catalog = builtinCatalog();
    if (flavor == GameFlavor::SkyrimSE)
    {
        for (const NativeEntry& e : kSkyrimSE)
        {
            addSignature(catalog, e);
        }
    }
    // Every other flavor keeps the cross-game builtins only: attributing the
    // Skyrim surface to Morrowind/Oblivion/Fallout would silently validate
    // natives those compilers do not ship.
    return catalog;
}

QStringList nativeNamesFor(GameFlavor flavor)
{
    const NativeCatalog catalog = nativeCatalogFor(flavor);
    QStringList names;
    names.reserve(int(catalog.all().size()));
    for (auto it = catalog.all().cbegin(); it != catalog.all().cend(); ++it)
    {
        names.append(it.key());
    }
    names.sort();
    return names;
}

int nativeCountFor(GameFlavor flavor)
{
    if (flavor == GameFlavor::Unknown)
    {
        return builtinCatalog().all().size();
    }
    return nativeCatalogFor(flavor).all().size();
}

} // namespace ObScript
