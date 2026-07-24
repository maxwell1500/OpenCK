# Cross-Reference: tes4codes.hpp Translation Table vs Starfield Ground Truth

## Summary

The `tes4codes.hpp` translation layer maps on-disk Bethesda subrecord codes to
OpenCK internal codes. **The ground truth confirms that ALL on-disk codes we can
verify are correct Bethesda codes.** The internal codes are the ones that are
invented. Every translation is ultimately unnecessary — we should use on-disk
codes everywhere and remove the translation layer.

The only legitimate purpose for a translation table would be to resolve name
collisions (same 4CC meaning different things in different record types). No
such collisions exist in the ground truth data.

## `fromTes4()` — Read Direction

| On-Disk (correct) | Internal (invented) | Found In Starfield? | Record Types |
|---|---|---|---|
| `SPLO` (spells list) | `INPC` | YES | NPC_, RACE |
| `CNTO` (container items) | `IACL` | YES | CONT, GBFM, NPC_ |
| `MODL` (model path) | `ODIT` | YES | 29 record types (ACTI, ALCH, AMMO, etc.) |
| `DNAM` (weapon type) | `ITM0` | YES | 19 record types (AACT, AFFE, ALCH, etc.) |
| `ICON` (icon path) | `ITM2` | **NOT FOUND** | Not present in any Starfield record |
| `MDOB` (school of magic) | `SChM` | **NOT FOUND** | MGEF in Starfield has no MDOB subrecord |

## `toTes4()` — Write Direction (additional mappings)

| Internal (invented) | On-Disk (correct) | Purpose | On-Disk Code Exists In Starfield? |
|---|---|---|---|
| `CLAS` (class ref) | `CNAM` | NPC_ class FormID | YES — CNAM appears in NPC_, CONT, etc. |
| `RACE` (race ref) | `RNAM` | NPC_ race FormID | YES — RNAM appears in NPC_, RACE, etc. |
| `FLAG` (flags) | `FNAM` | Flags/Full Name | YES — FNAM appears in 17 record types. FLAG itself appears as raw code in KEYM, MISC |

## Key Findings

### 1. All translations are unnecessary
No name collisions exist in the data. Each on-disk code is unambiguous within
its record type context. We can:
- Remove `Tes4Codes::fromTes4()` entirely
- Make `readNSubHeader()` behave like `readRawNSubHeader()` (no translation)
- Remove `Tes4Codes::toTes4()` entirely
- Use on-disk codes directly in all `load()` / `save()` methods

### 2. `ICON` and `MDOB` are not used in Starfield
Starfield does not use `ICON` (icon paths) or `MDOB` (magic school) subrecords
at all. These may still be relevant for Skyrim SE / Fallout 4 but are absent
in Starfield.

### 3. `FLAG` appears on disk in Starfield KEYM/MISC
Starfield uses `FLAG` (not `FNAM`) for flags in KEYM and MISC records. This is
a game-specific difference from Skyrim SE where FNAM is used. The `toTes4()`
mapping of FLAG → FNAM is correct for writing to Skyrim/FO4 format but would
be incorrect for Starfield. This may need a game-version-aware path.

### 4. GBFM records contain binary subrecord data
The GBFM (Geometry B-Face Mesh) record type has 200+ unique subrecord codes
that are binary data, not ASCII text. This is expected — some subrecord types
encode numeric/offset data in their 4-byte name field.

## Action Items

1. **Modify `readNSubHeader()`** to stop calling `Tes4Codes::fromTes4()`.
   Remove the Tes4Codes namespace entirely once all record parsers use correct
   on-disk codes.
2. **Update record parsers** (Phase 1.1):
   - Replace `'ODIT'` with `'MODL'` in all 29 record types
   - Replace `'ITM0'` with `'DNAM'` in all 19 record types
   - Replace `'IACL'` with `'CNTO'` in CONT, GBFM, NPC_
   - Replace `'INPC'` with `'SPLO'` in NPC_, RACE
   - Replace `'ITM2'` with `'ICON'` (if any record uses it)
   - Replace `'SChM'` with `'MDOB'` (if any record uses it)
   - Replace `'CLAS'` with `'CNAM'` in NPC_
   - Replace `'FLAG'` with the correct per-record-type code (FNAM for most, FLAG for Starfield KEYM/MISC)
3. **Handle game-version-specific codes**: Starfield uses `FLAG` for KEYM/MISC
   flags while Skyrim/FO4 use `FNAM`. The parser/writer may need to be
   game-version-aware.

## Verification

Run `test_groundtruth.exe` against each supported game's ESM to build
game-specific ground truth tables before applying fixes.
