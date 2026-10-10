#pragma once

#include <QHash>
#include <QString>
#include <QVector>

#include "obscripttypechecker.hpp"

// Game-flavored native catalog for OBScript/Papyrus validation. The function
// sets differ meaningfully between Creation Engine generations, so a script
// that compiles in Skyrim SE can reference natives absent in Morrowind and
// vice versa. The entries below are hand-curated from public Bethesda script
// documentation (the Creation Kit wiki function lists, the Papyrus reference
// and the game SDK headers shipped with the Creation Kit installs).
namespace ObScript
{

/// The Creation Engine games whose script natives are known here.
enum class GameFlavor
{
    Unknown,    ///< not identified; only the cross-game builtins apply
    Morrowind,
    Oblivion,
    Skyrim,
    FalloutNV,
    SkyrimSE,   ///< Skyrim SE and Anniversary Edition share this surface
    Fallout4,
    Fallout76,
    Starfield
};

QString gameFlavorName(GameFlavor flavor);

/// Maps a game name/path to the closest flavor. Unknown names resolve to
/// GameFlavor::Unknown so callers can warn instead of failing.
GameFlavor gameFlavorFromName(const QString& name);

/// The native catalog for a flavor: the cross-game builtins plus (for known
/// flavors) the game-specific functions/properties.
NativeCatalog nativeCatalogFor(GameFlavor flavor);

/// Sorted native names for a flavor — used for autocomplete word lists.
QStringList nativeNamesFor(GameFlavor flavor);

/// True when the flavor has a curated specific surface beyond builtins.
bool hasKnownSurface(GameFlavor flavor);

/// The number of native entries available to a script, or -1 when unknown.
int nativeCountFor(GameFlavor flavor);

} // namespace ObScript
