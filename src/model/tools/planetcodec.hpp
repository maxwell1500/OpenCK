#ifndef PLANETCODEC_HPP
#define PLANETCODEC_HPP

#include "planetdefinition.hpp"

struct PndRecord;

// PlanetDefinition <-> PndRecord binary codec (REMAINING.md §3.8). The PNDT
// on-disk layout is validated byte-for-byte against the real Starfield.esm
// (test_pndrecord); this maps the editor-side PlanetDefinition onto it.
//
// Observed real data (surveyed 2026-09-14, first 25 PNDT in Starfield.esm):
// - EDID/ANAM carry the planet and star-system editor ids verbatim
//   ("JemisonPlanetData"/"Jemison", "MarsPlanetData"/"Mars").
// - TEMP is float Celsius (Mars -5, Jemison 21, Vectera -214, Algorab I 445).
// - DENS reads like g/cm^3 (1.95-6.74), PHLA is a small int (2-348),
//   RSCS a 32-bit resource bitmask. Placeholder orbital records use
//   TEMP 0 / DENS 1 / PHLA 1 / RSCS 0 — the same defaults toRecord() uses
//   for a definition authored from scratch.
// - Everything else (keywords, FULL name, model, traits/biomes blobs) rides
//   in rawSubRecords and is preserved verbatim, never decoded.
//
// Mapping contract:
// - editorId <-> EDID and starSystem <-> ANAM transfer exactly.
// - temperature: PlanetDefinition keeps a display string; fromRecord() stores
//   the measured float as a plain numeric string ("21"), toRecord() parses a
//   leading number back (QString::toDouble semantics, so "21C" also works).
//   A non-numeric label ("Temperate") encodes as TEMP 0.0 — documented, and
//   the reason hasUndecodedData-style loss checks exist on the runner side.
// - DENS/PHLA/RSCS have no definition-side counterpart: toRecord() seeds them
//   from an optional base record (the record under edit) so an edit round-
//   trip is lossless, or from the observed shipped defaults for new planets.
// - PlanetDefinition extras with no PNDT counterpart (day length, gravity
//   label, biomes, traits, named resources) stay JSON-side by design.
namespace PlanetCodec
{

/// Builds a PndRecord from an editor-side definition. When `base` is given,
/// formId/flags/TEMP-unless-overridden/DENS/PHLA/RSCS/rawSubRecords/mOrder
/// are seeded from it and only the definition-mapped fields are applied, so
/// editing a loaded record round-trips losslessly. Without `base` the record
/// is new: formId 0 and the observed shipped defaults (TEMP from the
/// temperature string, DENS 1.0, PHLA 1.0, RSCS 0, mOrder naming the full
/// typed set (EDID/FNAM/ANAM/TEMP/DENS/PHLA/RSCS) so save() emits them).
PndRecord toRecord(const PlanetDefinition& def, const PndRecord* base = nullptr);

/// Builds an editor-side definition from a loaded record. Raw subrecords
/// are not decoded; their presence is reported via hasUndecodedData().
PlanetDefinition fromRecord(const PndRecord& rec);

/// True when the record carries subrecords the codec does not decode.
bool hasUndecodedData(const PndRecord& rec);

} // namespace PlanetCodec

#endif // PLANETCODEC_HPP
