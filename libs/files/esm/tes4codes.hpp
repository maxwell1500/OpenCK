#ifndef TES4CODES_HPP
#define TES4CODES_HPP

#include <QtGlobal>

namespace Tes4Codes {

// Translate real TES4 on-disk subrecord codes to OpenCK internal codes (for reading).
// When reading a real TES4 ESM/ESP file, the on-disk codes are translated to
// the internal codes used by OpenCK's record loaders.
//
// Codes that are the same in both formats pass through unchanged (identity).
// Only codes that differ between TES4 and OpenCK internal format are listed here.
inline quint32 fromTes4(quint32 tes4Code)
{
    switch (tes4Code)
    {
        // NPC_ record: TES4 'SPLO' (spells list) -> OpenCK 'INPC'
        case 'SPLO': return 'INPC';

        // NPC_ record: TES4 'CNTO' (container/inventory items) -> OpenCK 'IACL'
        case 'CNTO': return 'IACL';

        // Common across many records: TES4 'ICON' (icon path) -> OpenCK 'ITM2'
        // Used in: ACTI_, ALCH_, ARMOR_, BOOK_, CONT_, INGR_, MISC_, STAT_, TREE_, WEAP_, CLASS_, FACT_, MAGIC_, PERK_, MATL_
        case 'ICON': return 'ITM2';

        // Common across many records: TES4 'MODL' (model path) -> OpenCK 'ODIT'
        // Used in: ACTI_, ALCH_, ARMOR_, BOOK_, CONT_, INGR_, MISC_, STAT_, TREE_, WEAP_, MAGIC_, MATL_
        case 'MODL': return 'ODIT';

        // MGEF record: TES4 'MDOB' (school of magic) -> OpenCK 'SChM'
        case 'MDOB': return 'SChM';

        // WEAP record: TES4 'DNAM' (weapon type) -> OpenCK 'ITM0'
        case 'DNAM': return 'ITM0';

        // NOTE: 'SCRI' passes through as identity for REFR (script FormID array).

        // NPC_ record: TES4 'ACBS' (config/stats packed struct)
        // ACBS is handled directly in NpcRecord::load() as a packed struct.
        // No translation needed (identity passthrough).

        // NOTE: FNAM collision
        // OpenCK uses 'FNAM' for flags (uint32) in 26 record types, but in TES4,
        // 'FNAM' is the Full Name (zstring) for some records (NPC_, WEAP, etc.).
        // Since FNAM appears in different record types than where TES4 uses it,
        // the collision is rare in practice. Future work should rename OpenCK's
        // internal flags code (e.g., to 'FLAG') and add per-record-type mappings.

        // NOTE: DATA is per-record-type
        // TES4 'DATA' has different meanings depending on the record type.
        // OpenCK's DATA mapping passes through as identity, which works because
        // each record's load/save handles DATA according to its own type.

        // Default: no translation (pass through)
        default: return tes4Code;
    }
}

// Translate OpenCK internal subrecord codes to real TES4 on-disk codes (for writing).
// When writing an ESM/ESP file that the real CK can read, the internal codes
// are translated to the real TES4 subrecord codes.
//
// Codes that are the same in both formats pass through unchanged (identity).
// Only codes that differ between OpenCK internal format and TES4 are listed here.
inline quint32 toTes4(quint32 internalCode)
{
    switch (internalCode)
    {
        // NPC_ record: spells list
        // OpenCK 'INPC' -> TES4 'SPLO'
        case 'INPC': return 'SPLO';

        // NPC_ record: inventory/container items
        // OpenCK 'IACL' -> TES4 'CNTO'
        case 'IACL': return 'CNTO';

        // NPC_ record: class reference (FormID)
        // OpenCK 'CLAS' -> TES4 'CNAM'
        // NOTE: 'CNAM' is used for different purposes in other records (ENCH_ costLimit,
        // ARMOR_ partsWorn, INFO_ conditions, PERK_ conditions, LCRT_ color, TES4 header author).
        // This mapping is safe because 'CLAS' is only used as an internal code in NPC_.
        case 'CLAS': return 'CNAM';

        // NPC_ record: race reference (FormID)
        // OpenCK 'RACE' -> TES4 'RNAM'
        // NOTE: 'RNAM' is used as an internal code in FACT_ (rank names) and
        // FACT_ does not use 'RACE', so this mapping is safe.
        // MATL_ uses 'MRCE' internally (was 'RACE', renamed to avoid collision).
        case 'RACE': return 'RNAM';

        // Common across many records: icon path (zstring)
        // OpenCK 'ITM2' -> TES4 'ICON'
        // Used in: ACTI_, ALCH_, ARMOR_, BOOK_, CONT_, INGR_, MISC_, STAT_, TREE_, WEAP_, CLASS_, FACT_, MAGIC_, PERK_, MATL_
        case 'ITM2': return 'ICON';

        // Common across many records: model path (zstring)
        // OpenCK 'ODIT' -> TES4 'MODL'
        // Used in: ACTI_, ALCH_, ARMOR_, BOOK_, CONT_, INGR_, MISC_, STAT_, TREE_, WEAP_, MAGIC_, MATL_
        case 'ODIT': return 'MODL';

        // MGEF record: school of magic
        // OpenCK 'SChM' -> TES4 'MDOB'
        case 'SChM': return 'MDOB';

        // WEAP record: weapon type
        // OpenCK 'ITM0' -> TES4 'DNAM'
        case 'ITM0': return 'DNAM';

        // NOTE: 'SCRI' passes through as identity for REFR (script FormID array).

        // OpenCK 'FLAG' -> TES4 'FNAM' for records where TES4 uses FNAM for flags.
        // NOTE: TES4 uses FNAM for different purposes depending on record type:
        // - ARMOR, CONT, FACT, WEAP: FNAM = flags (uint32) — same as OpenCK
        // - ACTI: FNAM = full name (zstring) — different from OpenCK
        // - Other records: no FNAM in TES4
        // This global mapping works because ACTI's FLAG won't be written (ACTI has no flags field).
        // Records that don't have flags (ALCH, CLASS, ENCH, INGR, MAGIC, MISC, SPEL, STAT, TREE)
        // won't write FLAG, so this mapping won't be triggered for them.
        case 'FLAG': return 'FNAM';

        // Default: no translation (pass through)
        default: return internalCode;
    }
}

} // namespace Tes4Codes

#endif // TES4CODES_HPP
