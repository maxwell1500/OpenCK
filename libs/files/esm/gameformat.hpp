#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

#include "common.hpp" // NAME

// Game-family detection and per-game record dispatch. This is the entry
// point for making OpenCK work across Morrowind / Oblivion / Skyrim /
// FO4 / Starfield with one code path: callers detect the game a plugin
// belongs to, then use the per-game record registry to decide which
// record types (and their editors) are valid.
namespace GameFormat
{

enum class Game
{
    Unknown,
    Morrowind,
    Oblivion,
    Skyrim, // covers LE / SE / AE (same record layout)
    Fallout4,
    Starfield
};

QString gameName(Game game);

// Detect the game family a plugin or master belongs to. The file's own
// basename is the strongest signal for base masters (e.g. "Starfield.esm",
// "Fallout4.esm", "Morrowind.esm"); for mods the master list is used
// instead. Both are case-insensitive and extension-agnostic; the HEDR file
// flags are a final tiebreaker (LightMaster 0x200 = Starfield-era).
Game detectGame(const QString& selfBasename,
               const QStringList& masterBasenames,
               quint16 flags);
inline Game detectGame(const QStringList& masterBasenames, quint16 flags)
{
    return detectGame(QString(), masterBasenames, flags);
}

// 4-char record codes that are specific to a given game (as opposed to the
// shared TES4 core set). Record dispatch uses this to enable/disable the
// per-game editors.
QVector<NAME> gameSpecificRecords(Game game);

bool supportsRecord(Game game, NAME type);

} // namespace GameFormat
