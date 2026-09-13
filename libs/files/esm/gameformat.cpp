#include "gameformat.hpp"

#include <QSet>

namespace GameFormat
{

QString gameName(Game game)
{
    switch (game)
    {
    case Game::Morrowind:
        return QStringLiteral("Morrowind");
    case Game::Oblivion:
        return QStringLiteral("Oblivion");
    case Game::Skyrim:
        return QStringLiteral("Skyrim (LE/SE/AE)");
    case Game::Fallout4:
        return QStringLiteral("Fallout 4");
    case Game::Starfield:
        return QStringLiteral("Starfield");
    default:
        return QStringLiteral("Unknown");
    }
}

Game detectGame(const QString& selfBasename,
               const QStringList& masterBasenames,
               quint16 flags)
{
    QSet<QString> names;
    QStringList all;
    all.append(selfBasename);
    for (const QString& master : masterBasenames)
    {
        all.append(master);
    }
    for (const QString& master : all)
    {
        QString base = master;
        const int dot = base.lastIndexOf('.');
        if (dot >= 0)
        {
            base = base.left(dot);
        }
        names.insert(base.toLower());
    }

    if (names.contains(QStringLiteral("starfield")))
    {
        return Game::Starfield;
    }
    if (names.contains(QStringLiteral("fo4")) ||
        names.contains(QStringLiteral("fallout4")))
    {
        return Game::Fallout4;
    }
    if (names.contains(QStringLiteral("skyrim")))
    {
        return Game::Skyrim;
    }
    if (names.contains(QStringLiteral("oblivion")))
    {
        return Game::Oblivion;
    }
    if (names.contains(QStringLiteral("morrowind")))
    {
        return Game::Morrowind;
    }

    // LightMaster (0x200) is a Starfield-era flag.
    if ((flags & 0x200) != 0)
    {
        return Game::Starfield;
    }

    return Game::Unknown;
}

QVector<NAME> gameSpecificRecords(Game game)
{
    switch (game)
    {
    case Game::Morrowind:
    {
        // Morrowind-only (TES3) record types, verified against the record
        // types present in Morrowind.esm.
        static const QVector<NAME> k = {
            NAME('BODY'), // body
            NAME('BSGN'), // beacon
            NAME('CLOT'), // clothing
            NAME('CREA'), // creature (base)
            NAME('LEVC'), // leveled creature list
            NAME('LEVI'), // leveled item list
            NAME('LOCK'), // lock
            NAME('PGRD'), // path grid
            NAME('PROB'), // probe
            NAME('REPA'), // repair
            NAME('REGN'), // region
            NAME('SNDG'), // sound generator
            NAME('SSCR')  // scripted sound
        };
        return k;
    }
    case Game::Oblivion:
    {
        // Oblivion-only record types (beyond the shared TES4 core).
        static const QVector<NAME> k = {
            NAME('PGRD'), // path grid
            NAME('SPGD'), // spawn grid
            NAME('LSPM')  // leveled spell
        };
        return k;
    }
    case Game::Skyrim:
    {
        // Skyrim-only record types.
        static const QVector<NAME> k = {
            NAME('MATT'), // material type
            NAME('CLMT'), // climate
            NAME('LAIF'), // land image
            NAME('GRPA'), // grass patch
            NAME('GRPL'), // grass plugin
            NAME('SNIP')  // scripted navinfo
        };
        return k;
    }
    case Game::Fallout4:
    {
        // FO4-only record types.
        static const QVector<NAME> k = {
            NAME('ASRC'), // asset source
            NAME('LTEX')  // land texture
        };
        return k;
    }
    case Game::Starfield:
    {
        // Starfield-specific record types (beyond the shared TES4 core).
        static const QVector<NAME> k = {
            NAME('SHOU'), // spacecraft
            NAME('HDPT'), // HUD part
            NAME('MUST'), // music track
            NAME('RELR'), // relic
            NAME('RVLB'), // volume / relay
            NAME('XEZN'), // encounter zone
            NAME('XLCN'), // location
            NAME('LTMP'), // lighting template
            NAME('XWEM'), // water environment
            NAME('XCLR')  // region list
        };
        return k;
    }
    default:
        return {};
    }
}

bool supportsRecord(Game game, NAME type)
{
    return gameSpecificRecords(game).contains(type);
}

} // namespace GameFormat
