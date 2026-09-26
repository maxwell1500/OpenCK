# REMAINING.md — Everything left to make OpenCK truly functional

> **Single source of truth.** All previous planning/status/debt/roadmap/phase
> documents were reconciled against the codebase and deleted (2026-08-16).
> Anything not listed here is **done** or **intentionally closed** (see §6).
> Don't re-add work that's already shipped.
>
> Baseline on this machine: Release build clean, 134/134 test executables
> green (excluding the standalone `test_subrecord_diff` CLI), including the
> real-data gates that load Starfield.esm + Magnus.esm + Vvardenfell.esp;
> GUI + CLI smoke tests pass.

### 2026-09-24 — Skyrim SE BSA writer conformance

`BsaArchive::create` now emits the v0x69 folder-record layout with 32-bit
folder offsets and padding, NUL-inclusive folder-name lengths, sorted hashes,
absolute payload offsets, explicit little-endian fields, checked size limits,
and optional LZ4 frame compression with per-file stored fallbacks.
`BsaArchive::open` now seeks each recorded folder block, derives the file-name
table from the maximum folder-block end, and validates table and payload
extents. `test_bsawrite` independently parses the generated tables and
verifies distinct folder offsets, name tables, non-overlapping payload extents,
compressed-frame headers, compression-bit semantics, and stored fallbacks; the
real Skyrim SE Voices archive also round-trips through the corrected reader.
The product archive action exposes both BA2 and BSA creation, with
format-specific file filters, compression selection, and recursive collection
of supported files from the selected directory tree.

### 2026-09-08 — UndoStack::push() now calls execute()

`UndoStack::push()` previously only stored the command without calling
`execute()`, meaning `EditRecordCommand` changes were never applied to the
collection. All 40+ editors that create an `EditRecordCommand` and push it
were silently discarding user edits. Fixed by adding `command->execute()`
to `push()`. Added `test_editor_writeback` smoke test (7 cases covering
STAT/GLOB/CELL/WRLD/ACHR/PACK types) verifying the push→undo→redo cycle.
Updated `test_undostack` assertions to match the new push-executes behavior.
All 109 tests pass.

### 2026-09-06 — ESMWriter group-size stack (Phase 1.2 hardening)

`ESMWriter::startGrup`/`endGrup` used a single `grupSizePos` field, so
nested groups (a top-level CELL group containing a cell-children group)
had the outer group's size left at 0. Fixed with a `QVector<qint64>`
stack: `testGrupSizeConsistent` builds a nested file, verifies both group
size fields via a raw byte scan. All 108 tests pass. Also removed the
inert HKLM IFEO `test_loader.exe` key via elevated cleanup.

---

## 1. Data-integrity gaps (highest priority — these are correctness bugs)

   1. **Residual reader warnings → zero.** Loading Starfield.esm still produces
   ~8 "unconsumed bytes" / compression-misalignment warnings (INFO `01047741`
   over-read of 4 bytes; compressed NPC/AI-package records such as
   `Traits_OctopedeA_BlisterCrab_Large`, `EncShip_TradeAuthority_A_Atlas02_AutopilotAI`,
   `EncShip_TradeAuthority_C_Highlander02_AutopilotAI`, `LC017_LvlStarborn_01_Flames`,
   `BE_KT02_Partygoer04`, `BE_KT02_PartyGoer08`). Track them against
   `docs/record_formats.md` and drive every row to zero with the W1 diagnostics.

    **Status 2026-09-03 (full-matrix materialization, Release):**
    `testMaterializationMatrixZeroWarnings` passes end-to-end over
    Starfield.esm + Magnus.esm + Vvardenfell.esp: **3,829,768 records
    materialized, zero reader warnings, exit 0** (incl. the 3,291,891-record
    REFR pass). The pre-fix tree crashed deterministically at the REFR
    transition (0xC0000374 in Debug and Release); the fix was guarding the
    `BGSRefData_Component` fixed-width reads (`NAME`/`DATA`/`XSCL`/`XOWN`/
    `DNAM`/`XESP`/`SCRI` now consume only declared bytes, LE) plus a drain
    break in `RefrRecord::load`. Note: the Debug configuration cannot run
    the full matrix inside QTest's 5-minute function timeout (3.8M
    collection inserts); the timeout kill followed by loader-thread
    teardown is what crashed Debug runs, not a parser bug. Verify Debug
    with `OPENCK_TEST_PER_TYPE_LIMIT` caps, Release for the full gate.

    **Status 2026-08-25 (full-matrix materialization, all deferred types):**
    the desync class of warnings is eliminated — zero negative-recLeft
    over-reads remain. Fixed this round (each was silently desyncing every
   following record in its GRUP):
   - `MsttRecord` (disk `MSTT`): Starfield writes 1-byte `FNAM`/`DATA`;
     the fixed 32-bit read over-read by 3. Now reads/writes at the observed
     width (`fnamWidth`/`dataWidth` preserved for round-trip).
   - `OutfitRecord`: 0-byte `DATA` marker; fixed 4-byte read over-read by 4.
   - `LocationRecord` (`LCTN`) + `LocationRefType` (`LCRT`): variable-width
     flag/id subrecords now read little-endian at declared size; LCRT drains
     post-CNAM subrecords into `rawSubRecords` (~3000 warnings).
   - `GameSetting` (`TSMG`): 14 trailing bytes after the value subrecord are
     drained losslessly into `rawSubRecords`.
   - `AmmoRecord` (disk `AMMO`): Starfield writes 8-byte `DATA` variants;
     all fields are guarded so exactly the declared bytes are consumed.
   - `NpcRecord`: compressed NPCs ending in a non-subrecord binary blob
     (four NUL bytes where a name would sit) are captured verbatim
     (`name=0` raw) instead of abandoning hundreds of tail bytes.
     Warning floor is now 0: `testMaterializationMatrixZeroWarnings` passes
    with 3,829,768 records / 180 types / zero warnings. The 40×
    compressed-record misalignments and ~30 unknown/garbage-name artifacts
    that previously remained were eliminated by the drain/guard fixes
    above. Fixed 2026-09-07: `Variant::load(Format_GMST)` now drains
     the DATA payload losslessly for unknown EDID prefixes instead of
     throwing; zero errors remain.

 2. **Untouched-plugin round-trip must be payload-identical.** Build a
    subrecord-diff tool (per-record list of subrecord name+payload) and run it
    for every record type over the full corpus (Starfield.esm, Magnus.esm,
    SeydaNeen.esp). Load → save untouched → diff; drive every differing
    subrecord to zero. Today only SeydaNeen is covered.

    **Status 2026-09-03:** `testSaveRoundTripSubrecordIdentical` passes and
    `test_subrecord_diff Vvardenfell.esp <saved>` reports **1374/1374
    records, 0 mismatches**. Vvardenfell.esp holds STAT+CELL+REFR+WRLD+LCTN.
    Fixed (all were save-side: unconditional default writes + reordered
    subrecords; the snapshot gate compares positionally):
    - `StatRecord`: spurious `FNAM` when flags==0; save now replays the
      recorded load order (`loadOrder` + `hasFlags`/`flagsSpelling`) and only
      appends values absent at load. Added `smallIconPath` mirror (ICO2 was
      silently kept only via `saveAll`).
    - `CellRecord`: spurious `DATA`/`XCLC`/`XOWN`/`XLOC` (members were
      uninitialized — Debug filled 0xCC — and save wrote them
      unconditionally); `DATA` width preserved (Starfield: 4 bytes) with
      `dataExtra` tail, 12-byte exterior `XCLC` tail kept in `xclcExtra`,
      empty-`XCLW` shape preserved; same load-order replay.
    - `RefrRecord`: spurious 1-byte empty `EDID` on placed refs without one.
    - `LocationRecord`: spurious `FNAM`/`DATA`; same load-order replay with
      presence flags; members default-initialized.
    - **Save order:** `Data` now records the plugin's flat load order
      (`m_pluginOrder`) and `Document::save` replays it (switching top-level
      type GRUPs as needed, CELLs with their children groups inline) instead
      of regrouping by type — Vvardenfell.esp is interleaved (e.g. WRLD
      between CELLs), which regrouping scrambled. New/overridden records
      absent from the order fall through to the type-grouped pass, which
      skips replayed records via `saveModifiedRecordsExcept`. New
      `IRecordCollection` hooks: `isRecordSaveable`,
      `saveRecordAt` (returns whether anything was written),
      `saveModifiedRecordsExcept`.
    **Status 2026-09-08:** `testSyntheticMultiTypeRoundTrip` added — writes a
    plugin with NPC_/GLOB/STAT/WRLD records, loads, saves untouched, and
    asserts subrecord-identical output. Always runs (no real-data dependency).
    Fixed: `GlobalVariable` and `LocationRefType` lacked a `formId` field,
    causing them to be displaced to the fallback group with formId=0 on save
    (breaking ordered replay). Both now carry `formId` set from the reader.
    The full Starfield.esm-scale round-trip (3.8M records) is now covered by
    the nightly/CI gate described below, rather than by the regular unit-test
    suite.

     **Status 2026-09-09:** The ~36 `Variant::load` GMST/GLOB LOG_ERRORs are
     gone. Verified by re-running `testMaterializationMatrixZeroWarnings`
     (full Starfield.esm + Magnus.esm + Vvardenfell.esp, 3,829,768 records
     materialized across 180 types) and capturing stderr: **zero `[ERROR]`,
     zero `[WARNING]`, zero "Error loading", zero "Invalid format"** lines.
     The fixes were the generic-walk `GlobalVariable::load` (no throw on
     unexpected subrecords) and `Variant::load(Format_GMST)` draining unknown
     EDID prefixes to `rawData` instead of throwing. Load-side is clean at
     full scale; only the save+diff at 3.8M scale is left to CI/nightly.

     **Status 2026-09-15 (nightly gate built + save-side fixes):** the
     deferred CI/nightly job now exists: `tests/test_fullscale_roundtrip.cpp`
     (standalone CLI — load → save untouched → snapshot-diff, plus
     `--compact` mode that compacts a copy and verifies reload; deliberately
     NOT in ctest) driven by `tools/nightly-roundtrip.ps1` (round-trips
     Starfield.esm, compacts up to 5 real `.esp` copies). Validated:
     Vvardenfell.esp 1374/1374 identical, Magnus.esm 522/522 identical,
     `--compact` on a real plugin (73/73 remapped, reload clean). The gate
     paid for itself immediately — SFBGS00D.esm exposed a save crash and
     seven round-trip bugs, all fixed:
     - Save fast-fail (`0xC0000409`) on localised GMSTs: `Variant::write`
       threw on `Var_LString` (and `Var_None` with empty `rawData`); the
       uncaught throw aborted the save. `write` now emits the string-table
       index for LStrings and re-emits drained payloads verbatim.
     - `RefrRecord` did not replay load order (component block always first):
       now records `loadOrder` + `hasEdid` and replays positionally via a new
       `BGSRefData_Component::saveSubrecord` single-sub writer.
     - `LocationRecord` DATA always wrote 12 bytes (shipped 8-byte variants
       exist): now preserves `dataFieldCount` + `dataExtra` tail.
     - `PndRecord` treated every FNAM as the flags u32: only the first is
       (later 24-byte FNAM structs were truncated and `flags` clobbered by
       the last occurrence); subsequent FNAMs stay raw with per-name replay.
     - `StatRecord` appended a 1-byte NUL EDID for EDID-less records.
     - `AlchRecord` over-read 4-byte DATA as 8 (weight+value), desyncing its
       stored raws so the save emitted garbage mid-record (this is what
       killed the snapshot walker at record 2621): DATA is now width-guarded
       (Starfield ALCH DATA is weight-only, 353/353 surveyed), FNAM/DATA
       emission gated on presence.
     - `GlobalVariable` invented FNAM for FNAM-less GLOBs (`hasType` gate);
       `KeywordRecord`/`FactRecord` invented EDID/FNAM/FULL
       (`hasEdid`/`hasFlags`/`hasFull` gates); `GameSetting` consumed a
       non-DATA subrecord (e.g. XALG) as its value and always emitted DATA
       (`isNextName` check + `hasData` gate).
     - Rule of thumb established: never emit a subrecord the source lacked
       unless it carries a user edit.
     **Status 2026-09-16 (SFBGS00D order + payload):** the flat record
     stream is now exactly reproduced — 434,990/434,990 records, and
     `test_subrecord_diff` reports `positional-shift 0`. Two structural
     gaps closed:
     - Records with no typed loader (GPOF/GPOG/GWED and any future type)
       are preserved verbatim as `Data::OpaqueRecord` (header + ordered
       raw subrecords, compressed bit cleared on re-emit) and written back
       in load order. `Data::opaqueRecords()` exposes them.
     - Cell children are now tracked generically. While loading, a GRUP
       stack (`Data::inCellChildrenGroup`, group type 6) attributes every
       placeable record to its owning CELL via `m_recordParentCell`;
       `Data::childrenOfCell()` returns them in file order and
       `Document::writeCellChildrenGroups` re-emits the interleaved
       sequence (REFR/ACHR/PGRE/PHZD/... + opaque) instead of regrouping.
     `testSyntheticCellChildrenAndOpaque` covers both (typed + unmodelled
     child interleaving, no cross-cell attribution).
     Payload diffs fell 38,838 → ~1,850 of 434,990 (95%). Fixed classes so
     far: invented EDID on records whose source lacked it (RFGP, DIAL,
     NAVM, PGRE, ACHR, PHZD); invented CNAM/TLOI on INFO with an ordered
     CTDA replay; Starfield wide XOWN/XESP payloads (12/8-byte) now keep
     their trailing words (component fields must also be copied in
     `clone()`/`copyFrom()` or they silently vanish on the baseRecord →
     modifiedRecord copy); CELL no longer invents DATA.
     **Status 2026-09-16 (later):** the fixed-preamble class is largely
     converted. Records now replay the source subrecord order via a shared
     `SubrecordReplay` helper (`libs/files/esm/subrecordreplay.hpp`) plus
     `Component::writeSubrecord` (so component-owned names such as
     MODL/ICON/FULL/YNAM emit at their original position instead of a
     fixed one): ACTI, MISC, BOOK, MSTT, FURN, ENCH, KEYM, LIGH, SPEL,
     SCEN, FLST, OTFT, LVLI, PACK. Sub-4-byte and wider-than-4-byte scalar
     subrecords keep their width via `ESMReader::readSubU32(&width)` /
     stored raw trailing bytes (`TESFlags_Component`, XESP, SPEL SPIT,
     ENCH ENIT, LVLI LVLD/LVLF/LVLO, PACK PKDT/PLDT/PTDT). Non-NPC payload
     **Status 2026-09-16 (final — §1 complete):** SFBGS00D.esm round-trips
     434,990/434,990 records with zero payload differences,
     BlueprintShips-Starfield.esm round-trips 1,503,332/1,503,332 with
     zero differences, and the full Starfield.esm master round-trips
     3,829,246/3,829,246 with zero differences (`positional-shift 0` in
     all three; ~5.77M records verified total). The remaining
     per-type gaps were closed by a generic verbatim mechanism rather than
     per-type parsing fixes:
     - `ESMReader` snapshots each record's exact on-disk payload span at
       `readHeader()` time (`lastRecordBody()`/`lastRecordSize()`), covering
       compressed payloads (4-byte size word + zlib stream) as well.
     - Opt-in record structs keep `verbatimBody` + `verbatimFlags` + a
       `verbatimSnapshot` of the parsed state at load (captured centrally
       in `IdCollection::loadRecord`, guarded by form-id match).
     - `tryWriteVerbatimRecord()` (`src/model/world/verbatimrecord.hpp`)
       re-emits the original bytes with the original header flags when the
       struct still equals its snapshot (components compared too); any edit
       trips the comparison and falls back to structured save, so editing
       is unaffected. Wired into `saveRecordAt`, `saveModifiedRecords[*]`,
       and `writeRecordState`, covering replay, grouped, CELL, and orphan
       paths.
     - Opted-in residual types: NPC_ (compressed), QUST, WRLD, PACK, ARMO,
       EFSH, INFO, HAZD, DOOR, REFR, WEAP, CELL, RACE, NAVI, ALCH, DEBR,
       plus the Starfield.esm-only set: IPCT, LTEX, FLOR, DIAL, IPDS, MUST,
       MRPH, AMMO, LCTN, ANIO, FURN, WATR, SMEN, CLAS, PDCL, GLOB, WTHR,
       SFBK, LCRT, ASPC, PKIN, SMBN, FACT, EFSQ, REGN, IDLE, EXPL, STMP.
       This also made the earlier per-type order-replay work a pure
       fast-path for edited records; untouched records no longer depend on
       parser completeness. The NPC_ corruption root cause (fixed-shape
       ACBS/AIDT reads vs. variable source + invented component defaults)
       is documented but no longer load-bearing for round-trip.
      Suite 130/130, lint clean. Debug support kept (env-gated, off by
      default): `OPENCK_SAVE_PROGRESS`, `OPENCK_SNAPSHOT_TRACE`.
      `test_subrecord_diff` prints a per-type mismatch histogram (its name
      filter must include underscores, e.g. `NPC_`).
      The nightly gate should be re-run after each per-type conversion.
      **Status 2026-09-20 (structured fidelity zero — §1 polish):** a new
      `test_structuredfidelity` gate loads every MISC/QUST/ARMO/EFSH/WRLD/
      INFO from Starfield.esm, re-saves through the STRUCTURED path
      (`record.save()`, bypassing the verbatim fast path that masks all of
      this in file round-trips), and byte-compares each emitted body with
      the source: **131,236 compared, mismatches driven to zero in every
      type** (MISC 1319, QUST 2077, ARMO 1017, EFSH 47, WRLD 429, INFO
      126,347; 4 compressed WRLDs skip). Fixed classes:
      - QUST segment-ordered replay (per-position action codes freezing the
        load-time routing, so interleaved stage/objective/alias/top runs
        round-trip exactly), presence-gated QSDT/CNAM, per-occurrence
        QSDT/CNAM values (duplicate QSDTs collapsed last-wins before),
        questDesc/dialogueView positional re-emit (were DROPPED), and raw
        preservation for binary/LString-index CNAM/desc/view payloads
        (UTF-8 decoding corrupted `00e8f302` into U+FFFD text).
      - ARMO full conversion (loadOrder replay, FLAG/DNAM/DATA presence +
        width preservation, EDID gating, missing drain-break): TESBipedModel
        gained width-aware INDX/BMDT reads (the fixed u32 read consumed the
        next subrecord — silent, `recLeft` never went negative so no
        warning), raw snapshots for male/female paths, `hasFemale`, and
        `writeSubrecord`; TESEnchantableForm gained `hasEnchant` +
        `writeSubrecord`; pickup sounds emit at position spelling; ARMO
        ANAM/ENAM route by size (47-byte ANAM paths stay raw, 4-byte form
        IDs go to the component); INDT stays record-level raw; duplicate
        MODL/MNAM/INDX/BMDT replay verbatim except the last (edits land
        there) via new occurrence lists on TESModel/TESBipedModel.
      - INFO empty-FNAM preservation (TESFlags `==0?1` width clamp invented
        a byte; now widens to u32 only when flags were actually set).
      - EFSH DATA width rule (empty stays empty, grows only to fit edited
        fields) + loadOrder replay (ENAM/DATA order).
      - WRLD first-occurrence-typed replay (duplicate NAM2/NAM3/ZNAM
        collapsed last-wins before) via a generic typedOrderPos set.
      - `operator==` now includes `components` on ARMO + MISC/CONT/MAGIC/
        PERK/FACT (component edits previously never tripped verbatim, so
        the stale bytes were emitted and edits silently lost).
      - `testSyntheticStructuredSaves` (always runs, no fixture):
        hand-crafted tricky bodies (duplicate QSDT, binary CNAM, differing
        MODL/INDX duplicates, empty FNAM/DATA, reordered EFSH, duplicate
        NAM2/NAM3) round-trip byte-exact, plus positional edit checks
        (QUST flag edit lands on the last duplicate only).
      Full Starfield.esm round-trip still 3,829,246/3,829,246 zero-diff;
      suite 132/132, zero-warning build, lint clean.

      **Status 2026-09-20 (structured fidelity — every type zero):** the
      fidelity gate now covers ACTI/BOOK/MSTT/FURN/ENCH/KEYM/LIGH/SPEL/
      SCEN/FLST/OTFT/LVLI/CONT/LCTN/MGEF/PERK/FACT/HAZD/DOOR/WEAP/RACE/
      NAVI/ALCH/DEBR/PACK/CELL/REFR alongside MISC/QUST/ARMO/EFSH/WRLD/
      INFO: **3,465,828 records compared, 0 mismatches in every type**
      (REFR alone 3,291,860; CELL 6,547 + 24,170 compressed skipped).
      Fixed classes:
      - Fixed-preamble saves converged to positional replay (WEAP, ALCH,
        DOOR, HAZD, RACE, FACT): EDID/FNAM/FLAG present but never invented,
        typed values emitted at their load position, component-owned names
        via `writeSubrecord`, leftovers appended.
      - Duplicate-subrecord collapse (the last-wins component stored one
        raw for N occurrences): occurrence lists added to TESModel
        (modlRaws/mnamRaws), TESFlags (flagsRaws), BGSPickupPutdownSounds
        (ynamRaws/znamRaws), TESBipedModel (indxRaws/bmdtRaws); the record
        replays occurrences 0..N-2 verbatim and the last via the component,
        so edits land on the live value. Fixed PACK (5 FNAMs, 2 PKDT/PLDT),
        ALCH (MNAM/ZNAM), FURN (MNAM), RACE (FNAM), DEBR (12 DATAs).
      - Width preservation where the reader over-read into the next
        subrecord: WEAP EAMT (2-byte), DOOR FNAM (1-byte)/SNAM.
      - LCTN dropped every linked-ref group with refTypeId 0 (a valid
        value); now emitted.
      - DEBR DATA is not a u32-count/256-byte-model struct (12 repeats);
        preserved raw + positional (assembled records still encode).
      - REFR invented NAME/DATA: `BGSRefData_Component::saveSubrecord`
        now gates both on load-presence (`hasName`/`hasData`) and re-emits
        the exact DATA float count (6 or legacy 7). This was the largest
        single fix (3.29M records).
      - `TESBodyParts_Component` gained `writeSubrecord`; `TESModel`
        gained occurrence lists.
      - Replay-state (`loadOrder`/`loadIsRaw`/presence flags) stays out of
        `operator==` so an assembled record equals its reloaded form (the
        `test_missingrecords` contract); `components` is compared where the
        original did. GRUP sizes re-checked (include-24 unchanged).
      Full Starfield.esm round-trip still 3,829,246/3,829,246 zero-diff;
      suite 132/132 (test_missingrecords 123/123), zero-warning build,
      lint clean.
     Debug support kept (env-gated, off by default): `OPENCK_SAVE_PROGRESS`
     in `Document::save`, `OPENCK_SNAPSHOT_TRACE` in the snapshot walker.
     `test_subrecord_diff` now prints a per-type mismatch histogram plus
     the first two examples per type (its name filter must include
     underscores, e.g. `NPC_`).

    **Status 2026-09-04:** `LocationRecord::locationName` is persisted now.
    `FULL` was consumed as an opaque raw (the shared
    `TESFullName_Component` never handles it — `tesfullname.cpp` holding
    real `canHandle`/`save` implementations is dead code, not compiled; the
    header-only version is a no-op), so the name mirror was always empty and
    editor edits were silently dropped while the raw preserved the
    round-trip. `FULL` is now parsed into `locationName` first-occurrence-
    wins (extras stay raw) and re-emitted at its load-ordered position;
    `test_locationrecord` asserts the name survives a save/load cycle.

3. **FormIdCompactor leaves opaque FormID references stale.** `XPRM` and other
    opaque Starfield raw payloads are not rewritten on compaction
    (`formidcompactor.cpp::rewriteRawSubRecords` only handles the known set).
    Either decode-and-rewrite or fail loudly when such a payload would be
    compacted.

    **Status 2026-09-08:** Resolved. The compactor now uses a generic
    fallback pass (`genericRawFormIdFix`) that rewrites any u32 in an
    opaque raw subrecord matching the old→new FormID map, in addition to
    the type-specific rewrites. The -2 refusal path is removed; the
    false-positive risk is negligible (only values matching the plugin's
    own records' FormIDs are affected). ~20 additional record types were
    added to the rewrite loop. The `testDiscoverFormIdSubrecordLayouts`
    diagnostic loads Starfield.esm and outputs all (type, subrecord,
    offset) FormID reference layouts for future explicit-rule additions.
    Verification is covered by the nightly gate; the generic fallback remains
    the compatibility path and explicit rules are an optimization.
     **Status 2026-09-19:** Verified. `tools/nightly-roundtrip.ps1` passes
     end to end: Starfield.esm untouched round-trip 3,829,246/3,829,246
     payload-identical, plus FormIdCompactor `--compact` reload-clean on
     SeydaNeen.esp, SeydaNeen2.esp, SeydaNeen_Minimal.esp,
     SeydaNeen_project_2026-07-04.esp (1367 owned/remapped), and
     test_100.esp (84 owned/remapped). **Resolved.**

  8. **GRUP size convention (found 2026-09-20, resolved).** Bethesda GRUP
     sizes include the full 24-byte group header; our reader
     (`skipGrupHeader`: end = pos-after-size + size) and writer
     (`endGrup`: size = content-after-size-field) both used exclude-8,
     so every group end was 8 bytes late and every size we wrote was 8
     short of Bethesda's. Proven by exact tiling: consecutive groups
     start exactly where the previous one's declared size ends them
     (10 boundaries: nested type-4/5 block tiling plus top-level GMST/
     KYWD spans), and our own round-trip file now carries Bethesda's
     exact first-group size (157113). Invisible to all previous gates
     (payload diffs ignore group headers; `testGrupSizeConsistent`
     enshrined our own convention). A sequential walker using the old
     ends desynced into garbage sizes and AV'd — that crash is what
     exposed it. Fixed both sides; `testGrupSizeConsistent` asserts the
     include-24 sizes (52/104) plus a fixture-gated tiling check over
     the first 3 shipped top-level groups. TES3 untouched (no shipped
     groups to ground a change).

4. **DIAL/INFO relationship walking.** INFO records nested under DIAL are
    parsed but not walked into a DIAL→INFO tree for the dialogue editor.

    **Status 2026-09-08:** `DialRecord::load()` now parses INAM into
    `responseIds` (was falling through to `rawSubRecords`). `save()`
    re-emits INAM from `responseIds` when `hasInam` is set. Round-trip
    verified by `testSyntheticMultiTypeRoundTrip`. `DialogueTreeEditor`
    and `DialogueEditorWidget::populateTree()` both iterate
     `dial.responseIds` to show INFO children; `addInfo` updates
     `responseIds` and `m_infoParentDial`. **Resolved.**

     Perf follow-up (2026-09-15): `Data::infosUnderDial` scans the whole
     INFO collection per topic — O(dials × infos), which timed out
     `testDialInfoParentWalking` at QTest's 5-minute limit on full masters
     (68k × 126k; responses are sparse early, so even a stop-after-25 cap
     still timed out). The test now gates on a linear
     `Data::infosWithParentDialCount()` single pass plus a 5-topic
     `infosUnderDial` spot-check. The product callers
     (`DialogueEditorWidget::populateTree`, `DialogueTreeEditor`) have the
     same complexity and will hang on full-master dialogue trees; they need
     a reverse parent→children index maintained alongside
     `m_infoParentDial`.

     **Status 2026-09-19:** Resolved. `Data` now maintains a
     `m_dialInfoChildren` reverse index (parent DIAL → INFO form id +
     collection index pairs) alongside `m_infoParentDial`: populated at
     load with the known collection index, updated in `setInfoParentDial`
     (reparent-safe, duplicate-free, dial 0 not indexed), cleared in
     `preload`, and self-healing on read (stale/unknown indices are
     validated against the collection and repaired with a scan, deleted
     INFOs filtered). `infosUnderDial()` is O(responses); the master test
     now walks all 68k topics (126,347 parented infos, ~6s) and asserts the
     walked total equals the linear count, plus a synthetic
     `testSyntheticDialInfoReverseIndex` covering attribution, reparenting,
     and duplicates. `populateTree()` and `DialogueTreeEditor::
     loadDialogueTree()` build a per-call formId→index hash instead of
     rescanning the INFO collection per response (first occurrence wins,
     same as the old scan).

 5. **Master-record state machine on save.** Verify that a materialized
    (deferred) master record saved without edits is not emitted as an override,
    and that an edit promotes base → modified correctly (State_Base /
    State_Modified / State_ModifiedOnly) across the corpus save path.

    **Status 2026-09-08:** Verified. `testMasterRecordSaveStateMachine` passes:
    master record materializes as `State_Base`, is NOT emitted on untouched
    save, promotes to `State_Modified` on edit, and IS emitted as an override.

 6. **ObjectPalette save/load asymmetry.** "Save Placement" never writes a file
    (it only appends in-memory) while "Load Placement" reads a binary file.
    Make save write the same format Load reads, and rename the extension away
    from `.json` since it is binary (`QDataStream`, little-endian).

    **Status 2026-09-09:** Resolved. `ObjectPalette::onSavePlacementClicked`
    now writes a binary little-endian `QDataStream` file (count, then per
    placement: name, x, y, z, rotX, rotY, rotZ, scale, active) under a
    `.placement` extension — exactly the layout `onLoadPlacementClicked`
    reads back (which re-resolves the base formId from the name via
    `resolveFormIdFromName`).

7. **Field range validation on editors.** Editors still accept out-of-range
   values that can corrupt ESM files (the long-running X-02 item). `ColumnValidator`
   exists — deploy it to the remaining editors and enforce ranges on every
   numeric field.

   **Status 2026-09-07:** `ColumnValidator` deployed to all 10 editors with
   save flows that lacked it: `globeditor`, `watereditor` (GLOB path),
   `landscapeeditor` (cell water), `aipackageeditor`, `navmesheditor`,
   `celltransitionseditor`, `dialogueeditorwidget`, `dialoguetreeeditor`
   (DIAL+INFO). `queststageeditor`/`questaliaseditor` skipped (no `Data*`).
   40 editors now have validation. Build + 108 tests pass.

---

## 2. UI write-back wiring (every edit must be undoable)

1. **Editor write-back audit.** Build a table of every editor/dialog in
   `src/view/window/`: does it read from `Data` and commit through
   `EditRecordCommand`/UndoStack? Fix the stragglers.

   **Status 2026-09-07:** `LandscapeEditor::saveHeightmap` and
   `saveWaterToCell` now route through `EditRecordCommand` with undo-stack
   push instead of raw `record.setModified()`. All other editors already
   used the canonical pattern. Build + 108 tests pass.

2. **Weather/light and water editors.** GMST add/edit/delete in
   `weatherlighteditor.cpp` / `watereditor.cpp` still discard edits in places;
   route them through the UndoStack like the Object Window's Game Setting add
   (done).

    **Status 2026-09-07:** `watereditor.cpp` already uses `EditRecordCommand`
    for GLOB/GMST edits and `removeRecordWithUndo` for deletes. No remaining
    raw mutations.

     **Status 2026-09-09:** `weatherlighteditor.cpp` "Add Setting" was a
     disabled no-op stub. It now builds a new `GameSetting` via
     `createNewRecord(CkId::Type_Gmst, id)` + `initializeSettingValue` (infers
     bool/int/float/string from the entered value), wraps it in
     `Record<GameSetting>(State_ModifiedOnly, nullptr, &gs)`, and pushes an
     `AddRecordCommand` (with a `coll.appendRecord` fallback). The button is
     re-enabled. `openck` links clean (0 errors); `test_editor_writeback`
     9/9 pass.
 3. **Editor write-back smoke tests.** Automated test per editor: open a fixture
     record, perform a canonical edit, assert the UndoStack gained a command and
     the record changed.

     **Status 2026-09-08:** `test_editor_writeback` added — 7 test cases covering
     StatRecord, GlobalVariable, CellRecord, WorldspaceRecord, NpcRecord,
     PackageRecord, and the no-change case. Each case: add fixture → push
     `EditRecordCommand` → verify record changed → undo → verify reverted →
     redo → verify re-applied. All pass.

     **Status 2026-09-15 (re-audit):** a fresh grep for raw `setModified`
     across `src/view/window/` found two stragglers the 09-07 audit missed:
     the Object Window script-text edit and `DialogueEditorWidget::onAddInfo`
     (DIAL response-id append) both mutated records without an undo command.
     Both now snapshot original → edit a copy → push `EditRecordCommand`
     (with a `setModified` fallback only when no UndoStack exists). The two
     `landscapeeditor.cpp` hits are the canonical pattern (snapshot + push).
     `test_editor_writeback` extended with `testScriptEditorUndoable` and
     `testDialAddInfoUndoable`: 13/13 pass. No raw mutations remain outside
     command application.

---

## 3. Editor / feature gaps

1. **Dialogue editor.** Conditional response editing, voice-file
    association (`.wav` links), and quest-graph stage editing
    (flags/indices/objectives).

    **Done 2026-09-09:** INFO records parse CTDA into
    `QVector<CtdaCondition>` (was opaque raw) and re-emit on save; VMAP
    voice files parse/save. The condition function dropdown is now
    data-driven: `CtdaCondition::functionName(id)` /
    `functionIdForName(name)` give a stable ID↔name mapping — known
    indices render by name, unknown indices render as `Function <hex>`
    and round-trip exactly — so a condition's function index no longer
    collapses to 0 on save. `InfoDataWidget` stores/parses the function
    id via that mapping (raw hex is also accepted). Quest stages now edit
    the standard per-stage `QSDT` flags (`QuestRecord::stageFlags`, one
    byte per stage) instead of a non-standard `SFLG` blob;
    `QuestStageEditor` reads/writes `stageFlags` and no longer emits
    `SFLG`. `test_conditionrecord` 9/9 pass (incl.
    `testFunctionNameRoundTrip`); `openck` builds clean.
2. **Animation timeline.** `NifKeyframeData`/`NifTransformData` block
    parsing (`nifrecord.cpp`), the `TimelineWidget`, keyframe undo
    commands (move/add/remove via `CommandUndoAdapter`), atomic NIF
    write-back (`NifAnimationWriter::writeKeyframesToNif`), and the SCEN
    phase timeline (`SceneTimelineWidget`) are all built;
    `AnimationEditor` wires the timeline to undo and Play/Stop.

    **Closed headlessly:** the animation playback and NIF import/write-back
    pipelines are implemented and covered by automated tests. **Remaining
    verification:** display-dependent Play confirmation on a skinned animated
    mesh and visual confirmation of the GPU shader.

    **Status 2026-09-13:** Automated tests added.
    `test_nifanimation` (6/6) covers JSON and XML export→import round-trips
    for clips/channels/keyframes (translation, rotation, scale) and markers,
    plus the null-export and missing-file import error paths. The
    in-viewport 3D playback is scoped in §8 (it is further along than the
    old note below suggested).
3. **Particle FX.** The NIF particle block parser
    (`NifParticleSystem`/`NifPSysEmitter`, parse + write in
    `nifrecord.cpp`, dispatched in `nifparser.cpp`) is built, and particle
    editors exist (`ParticleEffectsEditor`, `ParticleRenderer`,
    `ParticleSystem`, `ParticleBundle`, LOD presets, projectile
    bindings).

    **Remaining verification:** display-dependent visual confirmation of the
    live in-viewport particle preview.

    **Status 2026-09-13:** In-viewport preview is wired and the simulation
    core is now headlessly tested.
    - `ParticleSystem` (the Qt/GL wrapper in `src/view/window/`) owns a
      `QTimer` and is driven by `NifViewportWidget`'s particle toolbar
      (play/pause/stop); `loadNif` calls `initParticleSystems()`, which parses
      the NIF's particle effects and shows the toolbar, and `renderMesh()`
      draws the live particles through `ParticleRenderer`.
    - The simulation math was extracted into a GUI-free
      `ParticleSimulation` (`src/model/tools/particlesimulation.*`): emission
      rate, lifetime jitter, spread/velocity bias, gravity, colour/size curves
      and a max-particle cap, using a reproducible LCG so results are
      deterministic. `ParticleSystem` now delegates to it and mirrors the
      particles for the renderer.
    - `test_particlesimulation` (9/9) covers emission, the max-particle cap,
      lifetime expiry, gravity, colour/size-over-lifetime, velocity bias,
      seed determinism, and reset.
4. **NavMesh reachability.** NavMesh generation works but uses centroid-based
    cell assignment; a reachability flood-fill pass is a documented residual.

    **Status 2026-09-09:** Added `NavMeshTools::largestReachableComponent` — a
    4-connected flood-fill over the row-major walkable cell grid that returns
    the largest connected component and (optionally) the total component
    count. `NavMeshGenerator::voxelFilter` now builds the walkable grid, runs
    the flood-fill, and prunes triangles/cells whose centroid cell is not in
    the largest reachable component, so disconnected floating islands are
    dropped. `NavMesh.componentCount` exposes the pre-prune component count.
    3 new tests added (`testLargestReachableComponent`,
    `testLargestReachableComponentSingle`, `testVoxelFilterDropsDisconnectedIsland`);
    `test_navmeshtoolkit` 21/21 pass.
5. **In-viewport object manipulation** (move/rotate/scale placed references
    in the render window). `NifViewportWidget` implements this: an
    `EditMode` (Select/Move/Rotate/Scale) with gizmos (`m_translateGizmoVBO` /
    `m_rotateGizmoVBO` / `m_scaleGizmoVBO`), grid/angle snapping,
    `refTransformPreview`/`refTransformCommitted` signals, and
    `setCellReferences`/`setSelectedRefIndex` for placed references.

    **Done:** `MainWindow` connects `refTransformCommitted` to an undoable
    write-back that snapshots the `RefrRecord`, applies the transform via the
    shared `RefrRecord::applyTransform`, and pushes an
    `EditRecordCommand<RefrRecord>` onto `Data::getUndoStack()`. Reference
    transform undo/redo is covered by `test_editor_writeback`
    (`testRefrTransformUndoable`) alongside the other record editors.
    **Remaining verification:** in-render-window 3D playback is wired for
    rigid animation, skinned animation, and particles; the remaining checks
    need a display and are scoped in §8.
6. **Mod-manager integration** (Mod Organizer 2 / Vortex).
    `ModManagerDetection` (detect/detectMO2/detectVortex/profiles/
    `getInstalledMods`) and `ModManagerDialog` are built and wired into
    `MainWindow::on_actionModManager_triggered`. The INI/JSON parsing is
    factored into testable `parseMo2Ini` / `parseVortexField` helpers;
    `test_modmanager` covers profile, gamePath, modsDirectory, selectedProfile
    and Vortex field extraction (6/6 pass).
7. **OBScript editor** — lexer + parser core done: `ObScript::Lexer`
   (`libs/files/esm/obscriptlexer.hpp/.cpp`) tokenizes reserved words,
   identifiers, int/float literals, strings, operators, newlines and `;`
   comments; `ObScript::Parser` (`libs/files/esm/obscriptparser.hpp/.cpp`)
   builds an AST via recursive descent: expressions with full operator
   precedence (`|| && == != < > <= >= + - * / %`, unary `- ! ~`, calls,
   field access, indexing, parenthesised grouping, literals) and the common
   statements (`function`/`endfunction`, `if`/`elseif`/`else`/`endif`,
   `while`/`endwhile`, `for … to …`/`endfor`, `return`, `let`/`set` and bare
   assignment, expression statements, and the optional `begin … end`/
   `endscript` wrapper). The parser reports the first syntax error with its
   line. `test_obscript` (8/8) covers the lexer; `test_obscriptparser` (15/15)
   covers functions, if/elseif/else, loops, precedence, calls/postfix,
   assignment/`let`, the `begin` wrapper, unary/logical operators and four
   error cases. The AST stores children in `std::vector` (Qt's `QVector`
   copies on reallocation and so cannot hold `unique_ptr`).
   The editor UI is in place: `ObScriptHighlighter`
   (`src/view/window/obscripthighlighter.hpp/.cpp`) classifies each source
   line into spans (control-flow / type / keyword / string / comment / number /
   operator) via the pure, testable `classifyObScriptLine`, and
   `ScriptEditorDialog` (`src/view/window/scripteditordialog.hpp/.cpp`) edits a
   script's SCTX source with that highlighter plus a live syntax-check status
   line (from `ObScript::parse`). `ObjectWindowDialog::editSelected` now has a
   `CkId::Type_Scpt_` case that opens it and writes the edited text back to
   `ScriptRecord::scriptText` (marking the record modified).
   `test_scripteditor` (11/11) covers span classification (incl. comments
   inside strings and escaped quotes) and the dialog's valid/invalid syntax
   status.
    The semantic binder is in place: `ObScript::bindProgram`
    (`libs/files/esm/obscriptbinder.hpp/.cpp`) builds a symbol table
    (functions, parameters, locals, globals), reports duplicate declarations
    as errors (duplicate function, parameter, or local), and collects
    references that do not resolve locally into `referencedExternals`
    (typically the game's native functions/properties); an optional
    `knownExternals` set suppresses entries a caller already knows.
    `ObScript::completionEntries` returns a sorted word list of keywords,
    declared symbols, and unresolved references. `test_obscriptbinder`
    (11/11) covers the binder and the completion word list.
    Autocomplete is wired in: `ScriptEditorDialog` attaches a `QCompleter`
    (via a `QStringListModel`) to the editor through an event filter and
    triggers it on Ctrl+Space; the word list refreshes with each syntax
    check.
    The compiler and game-specific native type catalog are implemented; the
    remaining unsupported runtime behavior is the game's actual execution of
    the emitted bytecode, which is outside the editor scope.

    **Status 2026-09-13:** Both remaining pieces are done.
    - Compile-to-bytecode: `ObScript::BytecodeProgram` +
      `obscriptbytecode.hpp` define a stack-based opcode set (control flow,
      arithmetic, comparison, bitwise, logical, stack, variables, calls) and
      `obscriptcompiler.hpp/.cpp` emits it from the AST with jump patching,
      a symbol table for variables, and expression compilation.
    - Type checking: `obscripttypechecker.hpp/.cpp` checks a program against
      a `NativeCatalog` of game functions/properties (`builtinCatalog()`
      provides the common Bethesda natives). It reports wrong argument
      count/type, calling a property as a function (errors), and unknown
      functions (warning).
    - `ScriptEditorDialog` now shows the first type error (red) or warning
      (amber) after a successful parse, or "Syntax and types OK" (green).
    - Tests: `test_obscriptcompiler` (8/8) and `test_obscripttypechecker`
      (8/8).
8. **Starfield-specific feature slots** (model/UI baseline plus validated
   integrations):
   spaceship editor, galaxy view, worldspace/planet-generation editors
   (PNDT planets, OPAL placement), reflection probes, crowd-region authoring,
   morph/face-gen editor, RoboVoicer (TTS pipeline), Houdini integration.

   **Status 2026-09-13 (worldspace/planet-generation editors):** the
   PNDT-planet and OPAL-placement editors are done at the model + UI level.
   - `PlanetDefinition` (already modelled + JSON round-trip) now has a
     `PlanetEditorDialog` exposing editor id, star system, day length,
     gravity/temperature, a biome table (name/colour/coverage), a checkable
     trait list (from `commonTraits()` plus customs) and a resource table,
     with Load/Save JSON.
   - `OpalList` gained `toCsv()` / `saveFile()` (CSV-quoted round-trip) and a
     `OpalPlacementDialog` that displays the header/rows in an editable table
     with add/remove row and Load/Save `.opl`.
   - Both dialogs are wired into the Tools menu
     ("Planet Editor...", "OPAL Placement Editor...").
   - `test_opallist` extended with a CSV round-trip case (8/8). The planet
     model is covered by `test_planetdefinition`.
   - `StarfieldToolsDialog` (all remaining slots, wired to the Tools menu):
     Spaceship editor (identity, reactor/grav/shield/engine, cargo/crew,
     mass/hull, module table), Galaxy map with a painted star-system view
     (`GalaxyViewWidget`: fit-to-view circles, click-to-select) plus system
     list and planet table, Reflection Probe editor (position, radius,
     resolution, projection, brightness, shape), Crowd Region editor
     (behavior, volume, density, member weights), Morph/Face editor (race,
     clamped value sliders as a channel table seeded from commonChannels),
     RoboVoicer (voice-line plan table + run that reports what a real
     `IVoiceSynthesizer` backend would synthesize), and a Houdini bridge tab
     that previews the generated interactive/batch command.
   - `test_starfieldtools` (11/11): JSON round-trips for all five models,
     the voice runner (success / null-engine / unavailable-engine), and the
     Houdini command builder.
    The previously deferred items are closed where the repository can validate
    them: PNDT/MRPH/ship/OPAL/galaxy/crowd/reflection encoders, SAPI voice
    playback, and native Houdini-side command scripts are implemented above.
    Undocumented reflection payloads and FaceFX runtime playback remain
    intentionally out of scope.

    **Status 2026-09-14 (leftovers closed where validatable):**
    - Binary encoders:
      - **PNDT (Planets):** `PlanetCodec::toRecord/fromRecord`
        (`src/model/tools/planetcodec.*`) maps PlanetDefinition onto the real
        PNDT layout — EDID/ANAM exactly, TEMP via the numeric temperature
        string, DENS/PHLA/RSCS seeded from the record under edit or shipped
        defaults. `test_planetcodec` 6/6, incl. 10 real PNDT losslessly.
      - **MRPH (Morphable Objects):** `MrhpRecord` now parses TCMP (morph
        path), MOBC (flags), TMPP (template path) as typed fields alongside
        EDID, with raw subrecords preserved. Surveyed 998 records (998 MOBC,
        976 TCMP, 191 TMPP). `test_morphrecord` 3/3 — 20 real records round-
        tripped byte-exact (17 with TCMP, 7 with TMPP, 0 failures).
      - **Ships:** composite COBJ→FLST→GBFM chain (no single SHIP record).
        Done this round:
        - `GbfmRecord` now parses the BFCB component architecture into a
          derived view (`GbfmComponent`: type name + its subrecords) while
          preserving `rawSubRecords` byte-exactly. Typed accessors extract
          `TESFullName_Component::FULL`, `BGSKeywordForm_Component::KWDA`
          (`u32List` flattens both the one-subrecord/many-value KWDA shape and
          the one-value-per-occurrence FLKW/FLFM shape), and
          `BGSFormLinkData_Component` ITMC/FLFM/FLKW.
        - `ShipPartCodec` (`src/model/tools/shippartcodec.*`) maps a GBFM to a
          `ShipPartDefinition` and writes it back; every opaque component
          (Blueprint_Component's BUO4, the NVNM navmesh blob, …) rides along
          untouched, so an untouched apply is byte-exact.
        - `ShipCompositeResolver` walks COBJ → CNAM → FLST → LNAM → GBFM and
          produces `ShipComposite` (recipe, form list, variant form ids +
          resolved editor ids), with a case-insensitive id lookup helper (the
          collections' own `searchId` is case-sensitive while the game is not).
        - `test_shipcomposite` 8/8 against real Starfield.esm: component
          parsing, 15 GBFM byte-exact round-trips, 15 no-op applies, a real
          chain (`co_SMS_FuelTank_Dogstar_M50_Ulysses` →
          `SMSSet_FuelTank_Dogstar_M50` → 2 variant GBFMs), the ship
          blueprint decode (25 records / 1,267 items / 0 stride failures) and
          the crowd component decode.
        - **Component layouts** are now taken from xEdit's published
          `wbDefinitionsSF1.pas` (MPL) rather than guessed, which unblocked
          two components the earlier note called opaque:
          - `Blueprint_Component::BUO4` — the module placements (Base Item
            GBFM, Construction Object COBJ, Vec3PosRot 3+3 floats, Part ID),
            a fixed 36-byte stride. This is the actual ship composition.
          - `BGSCrowdComponent_Component` — CDND density, CDNS population
            count, and per-population STRV name + FLTV scale.
        Genuinely opaque: `ParticleSystem_Component` (PTCL) and
        `HoudiniData_Component` (PCCC) are `wbReflection` data streams —
        preserved byte-exactly but not semantically decoded (xEdit blocks
        override-copying them too). (`ReflectionProbes_Component` was in this
        list until 2026-09-14; it has zero shipped instances and the real
        probe data is `Volumes_Component::VLMS`, now decoded — see below.)
        NVNM (navmesh) is defined but large; it rides along as a raw
        subrecord.
      - **OPAL placement lists:** the real binary `.opl` format is now
        decoded and implemented (the earlier CSV/header version was a guess).
        Found via the ten shipped lists under `Content/OPAL/` — all 3,311
        entries parse with zero trailing bytes. Layout: `uint32 version` (=3),
        `uint32 count`, then per entry `uint32 nameLen`, name + NUL, `uint32
        payloadLen` (0 or 24), payload (24 = 6 floats: pos xyz + rot xyz),
        `uint64 trailer` (high 32 always 0; low 32 = FormID). `OpalList` was
        rewritten to this layout (payload kept as raw bytes for exact
        round-trip), the dialog now shows name/transform/FormID, and
        `test_opallist` 7/7 — including a byte-exact round-trip of all ten
        shipped files.
      - **Galaxy — DONE 2026-09-14.** The galaxy map *is* an ESM record after
        all: `STDT` (Star), 123 in Starfield.esm. The earlier note missed it.
        `StdtRecord` (`libs/files/esm/stdtrecord.*`) stores subrecords raw and
        round-trips byte-exactly, with typed accessors for the galaxy fields:
        `ANAM` name, `BNAM` system parsec location (3 floats — the map
        position), `DNAM` system id, `ENAM` colour, `SNAM`/`PNAM` links. The
        star catalogue data (catalogue id, spectral class, magnitude, mass,
        habitable zones, HIP, radius, temperature) lives in the
        `BGSStarDataComponent_Component` base-form component, parsed from its
        `DATA` layout per xEdit. `test_starrecord` 5/5: 30 stars with all
        fields, 123 scanned / 122 distinct system ids, byte-exact round-trip,
        and REFL schema decode.
      - **Crowd — DONE (component):** `BGSCrowdComponent_Component` (density,
        population count, per-population name/scale) is decoded on GBFM.
      - **ReflectionProbes — RESOLVED 2026-09-14 (was looking in the wrong
        place).** Searched *every* installed master (Starfield.esm,
        BlueprintShips 290 MB, SFBGS00D 97 MB, all DLC/mod masters) and the
        Creation Kit itself:
        - `ReflectionProbes_Component` has **zero shipped instances** — xEdit
          defines it, nothing emits it. It was never the right target.
        - The CK's real reflection-probe system is cell/volume based: its
          binary references `ProbeGridVolume`, `ReflectionProbeCellComponent`
          and `ReflectionProbeInstanceData`, and a "Reflection Probes" toolbar
          action, and `E:\BuildAgent\...\Genesis\BSMain\BSReflectionProbe.cpp`.
        - The shipped representation is **`Volumes_Component::VLMS`** (present
          on STAT/REFR/GBFM in every master) plus `XVOI` — "Volume Reflection
          Probe Offset Intensity" — on references.
        - `parseVolumePayload` (`libs/files/esm/baseformcomponents.*`) decodes
          VLMS: `uint32 count`, then per entry `uint32 type` (1/3/5),
          row-major `float[16]` matrix, 3 floats, and a type-specific tail
          (1→1, 3→2, 5→3 floats) per xEdit's `wbVLMSTypeDecider`. Validated
          against the whole master: **11,765 VLMS subrecords, every one
          consuming exactly its own size, 0 failures** (types 1:114, 3:549,
          5:18223). `test_shipcomposite::testVolumeComponent` pins a sample.
        - `parseReflectionStream` (`libs/files/esm/reflectstream.*`) also now
          reads the `BETH`-framed REFL schema (root type + field names), so
          that stream is no longer an opaque blob. Field *values* still need
          the per-type layouts nobody has.
        So the probe geometry/data is decodable from shipped files; the
        remaining gap is only the *semantics* of the volume `type` codes,
        which neither xEdit nor the CK expose.
    - Voice/Houdini: done (see above).
    - Voice playback: `SapiVoiceSynthesizer` renders lines to WAV through
      the in-box Windows speech engine (System.Speech over SAPI in a helper
      process — no ATL/SDK linkage; ~1 s per call), `runVoicePlan` uses it
      whenever voices are installed (David/Zira/Haruka verified here), and
      the dialog offers playback of the first completed line through the
      existing `VoicePreview::playVoiceAudio` path. `test_starfieldtools`
      13/13 (2 new SAPI slots, skipping cleanly on voiceless machines).
    - Houdini scripts: `tools/houdini/export_ship.py` (ship geometry to FBX
      via a filmboxfbx ROP, `--node`/`--hip`) and `import_opal.py` (OPAL CSV
      rows to null locators with spare parameters, x/y/z honored when
      present) — the batch counterparts to the bridge's command builder.
      `py_compile` clean; both exit 2 with a clear message outside Houdini
      (no Houdini installed here to run them under).
 9. **Multi-game record dispatch** — foundation verified against real
    non-Starfield masters:
    `GameFormat` (`libs/files/esm/gameformat.hpp/.cpp`) detects the game
    family from master basenames + the LightMaster flag (`detectGame`), now
    also from the opened file's own basename (real files are named after the
    game, not "Skyrim.esm"/"Starfield.esm"), and exposes a per-game record
    registry (`gameSpecificRecords` / `supportsRecord`), now filled for
    Oblivion (PGRD/SPGD/LSPM), Skyrim (MATT/CLMT/LAIF/GRPA/GRPL/SNIP),
    FO4 (ASRC/LTEX) and Starfield. `test_gameformat` covers detection (all
    5 games) and every per-game registry (11/11). `test_multigame` parses real masters end-to-end through the
    existing `ESMReader`/`Header`: **FO4 `Fallout4.esm` (1.74M records) and
    Starfield `Starfield-Core.esm` (3.83M records) each read 400 records with
    0 errors and correct `detectGame`, and the base `.esm` masters correctly
    report no MAST entries** — proving the unified TES4 reader generalizes
    beyond Skyrim.
     **Morrowind (TES3) is a genuine separate format:** its first record is
     `'TES3'` and stores `HEDR` *inline* (version/numRecords/nextObjectID as
     raw fields, not a subrecord) followed by per-month `GMST`/`NAME`/`STRV`
     calendar subrecords — the TES4 reader's `header.load()` desynced on the
     inline bytes and yielded 0 records.

     **Status 2026-09-12 (TES3 reader core — Phase 1):** the reader now
     accepts `'TES3'` masters end-to-end at the structural level:
     - `ESMReader::open` recognizes the `'TES3'` magic (`m_tes3` flag);
       TES3 record header is 16 bytes (name+size+unknown+flags, no formId),
       subrecord header is 8 bytes (name+uint32 size, no XXXX/compression).
       `readHeader`/`readNSubHeader`/`readSubHeader`/`skipGrupHeader`/
       `buildRecordIndex` all branch on `m_tes3`.
     - `Header::loadTes3` parses the inline `HEDR` subrecord (version float,
       file-type uint32, fixed 32-byte author, fixed 256-byte description,
       record count) plus `MAST`/`DATA`/`GMDT`/`SCRD`/`SCRS`. New
       `ESMReader::readFixedString(int)` reads a fixed-width string field
       inside a subrecord (TES3's HEDR author/description are flat fields,
       *not* subrecords — `readZString` over-consumed them).
     - `Data::continueLoading` takes a TES3 branch: read header, dispatch
       through the existing switch, drain remaining subrecords losslessly —
       no desync, no per-type loader required for a clean walk yet.
     - Verified against the real `Morrowind.esm` (79,837,557 bytes):
       **48,295 records walked end-to-end with zero desync** (`test_tes3`
       6/6: HEDR v1.2/type-1/48,295; full walk; record index first entry
       GMST@324; GMST NAME+STRV fully drained). `test_multigame::testMorrowind`
       is upgraded from detection-only to a full 400-record smoke walk.
     - TES3 GMST `STRV` holds a *variable-length string* for string globals
       (e.g. `sMonthMorningstar` → "Morning Star"), a 4-byte float for
       numeric ones — the size field disambiguates.
     - Morrowind record registry added to `gameformat` (BODY/BSGN/CLOT/
       CREA/LEVC/LEVI/LOCK/PGRD/PROB/REPA/REGN/SNDG/SSCR — the types
       actually present in Morrowind.esm, per the walk histogram);
       `test_gameformat` 12/12.
    `detectGame` is now wired into the loader: `Data::preload` detects the
    game from the file's own basename, the MAST list and the HEDR flags and
    stores it (`Data::currentGame()`); `Data::isGameSpecificRecord(NAME)`
    answers whether a record code belongs to the detected game's registry,
    and the unknown-record warning names the detected game.
    `test_editor_writeback::testCurrentGameDetection` preloads the real
    `Starfield.esm` through `Data` (indexing 3.8M records in ~1 s) and
    asserts the detection (11/11 overall).
    TES3 Phase 2 scope (superseded by the statuses below): per-type record
    loaders for the Morrowind record types in the walk histogram (INFO/DIAL/
    CELL/STAT/NPC_ bodies etc. — ~40 types, mirroring the OpenMW
    `esm3/load*.cpp` layouts), a TES3 save/round-trip path with subrecord-
    diff gating, and the game-specific editors (§3.8).

     **Status 2026-09-12 (TES3 generic record + save — Phase 2a):** the
     entire Morrowind format now loads and saves through one generic record,
     no per-type loader required:
     - `Tes3Record` (`libs/files/esm/Tes3record.hpp/.cpp`) preserves the
       16-byte header (type/size/unknown/flags), the first `NAME` subrecord
       (raw payload kept verbatim; editorId decoded as Latin-1 so non-UTF-8
       bytes like `0x92` survive), and every other subrecord positionally in
       load order. `save()` replays them in order, adding a `NAME` only when
       one exists.
     - `ESMWriter` gained a TES3 mode (`setTes3`): 16-byte record headers,
       8-byte subrecord headers with uint32 sizes, NUL-less zstrings, no XXXX,
       and a 320-byte HEDR record-count patch offset. `Header::save` writes
       the inline TES3 `HEDR` (version/type/fixed 32-byte author/fixed
       256-byte description/count) plus `MAST`/`GMDT`/`SCRD`/`SCRS`.
     - `Data` stores Morrowind records in `QHash<NAME, IdCollection<Tes3Record>*>`
       keyed by record code (`tes3CollectionFor`), assigns synthetic form ids
       (unique even for `NAME`-less LAND/PGRD), registers Qt models lazily,
       and `allCollectionsWithTypes`/`getCollectionByType` branch on
       `GameFormat::Game::Morrowind`.
     - `Data::saveTes3Records` replays `pluginOrder` flat (no GRUPs) and
       writes each record directly so the record header flags survive;
       `Document::save` takes a TES3 branch.
     - 9 new `CkId::Type`s: BODY/LEVC/LEVI/LOCK/PGRD/PROB/REPA/SNDG/SKIL.
     - `test_tes3roundtrip` (new, gated on the real Morrowind.esm): full
       48,295-record load with **exact per-type counts** (GMST 1449, NPC_ 2675,
       STAT 2788, DIAL 2358, CELL 2538, LAND 1390, PGRD 1194, BODY 1125, …)
       and a **byte-identical** save of the whole 79,837,557-byte master.
    TES3 Phase 2b scope (superseded by the statuses below): component-backed
    editors for the Morrowind record types (the generic record edits
    losslessly but exposes raw bytes only), and the game-specific editors
    (§3.8).

     **Status 2026-09-13 (TES3 component-backed editing — Phase 2b):**
     - `Tes3Record` gains `parseComponents()`, called after `load()`, which
       extracts display/edit components from the raw subrecords: `TESFullName`
       (FULL), `TESModel` (MODL/MNAM), `TESTexture` (ICON/ICO2), and the new
       `Tes3Data_Component` (`tes3_components.hpp`) which captures the DATA
       subrecord bytes and exposes them as a hex-edit property. The save path
       still replays the original raw subrecords, so untouched records remain
       **byte-identical** (the 79,837,557-byte round-trip test still passes).
     - `Tes3Record` added to the `FOR_EACH_COMPONENT_RECORD_TYPE` resolver
       macro, so the Object Window's generic `editSelected` path opens
       Morrowind records through `QtFormDialog` like any other record type.
    TES3 Phase 2c initial scope (superseded below): type-specific DATA parsing
    (the current DATA editor is generic hex), edit-through-component write-back
    wired into the UndoStack, and specialised editors for Morrowind
    record families.

     **Status 2026-09-20 (component write-back — Phase 2c core):** done.
     - `Tes3Record::operator==` now includes `components` (was raw-only), so
       `EditRecordCommand::hasChanged()` detects form-dialog edits; the
       component `clone()`/`copyFrom()`/`isEqualTo()` set was already
       complete, so push→undo→redo works through the standard UndoStack.
     - `Tes3Record::save()` emits component values for FULL/MODL/MNAM/ICON/
       ICO2/DATA at their load-ordered positions when they differ from the
       parse of the last raw occurrence, and appends component values for
       subrecords the source lacked (fixed FULL…DATA order). Untouched
       records replay raws verbatim and stay byte-identical. The
       last-occurrence rule matters: parse is last-wins for duplicates and
       lossy for non-UTF8 bytes, so byte comparison flagged untouched
       records as edited (+27 KB drift on the full master, caught by the
       byte-identical test). NAME edits now go through `writeSubZString`
       (NUL-less in TES3 mode) instead of appending a stray NUL.
     - `testSyntheticComponentWriteBack` (always runs, no fixture): synthetic
       Morrowind.esp with two CLOTs — untouched save byte-identical;
       FULL+DATA edit survives save/reload at original positions with MODL
       intact; undo→save→reload restores the original bytes exactly;
       redo re-applies; MODL added to a MODL-less record is appended.
     Full Morrowind.esm round-trip still byte-identical (48,295 records);
     suite 130/130, zero-warning build (also fixed a pre-existing C4477
     `%d`→`%lld` in `subrecordsnapshot.hpp` that kept the gate red),
     lint clean.
    TES3 Phase 2c type-specific DATA scope (completed below): the generic
    hex editor stays only for unvalidated layouts.

     **Status 2026-09-20 (type-specific DATA parsing):** done for every
     DATA group with a survey-proven layout.
     - `libs/files/esm/tes3datalayout.*` (openck_esm): data-driven
       (code, size) → field table + LE decode/encode (floats via memcpy):
       DIAL/1 dialogType; INFO/12 dialogType + disposition + flags + rank
       + gender + pcRank; CELL/12 flags + gridX/Y; CELL/24 placed-ref
       pos/rot floats; LAND/LEVC/LEVI u32 flags; SNDG type; SOUN
       volume/minRange/maxRange; LTEX NUL-terminated texture path.
     - Layouts grounded in `test_tes3data` diagnostics against the real
       master: size histogram, per-lane ranges, INFO byte-0 == parent DIAL
       type 23,693/23,693, CELL FRMR↔24-byte-DATA pairing 2,538/2,538,
       INFO value sets (disposition ≤100 + journal indices, gender
       {0,1,255}, ranks 0-9/255). PGRD/12 deliberately undecoded (lane 3
       is not a point count; counts shadowing siblings are unsafe to
       expose) — pinned by test to stay on hex.
     - `Tes3Data_Component` decodes the mirrored occurrence into typed
       fields with per-field editor properties (plus a committing hex
       view); field edits re-encode into the raw bytes, so save/undo/hex
       follow. The component is occurrence-explicit (`recordCode` +
       `dataOccurrence`): multi-DATA CELLs expose the header occurrence,
       transforms stay raw. `Tes3Record::save` substitutes at that
       occurrence.
     - Tests: `testLayoutConformance` decodes + re-encodes all 347,143
       covered DATAs byte-exact (0 failures; only PGRD uncovered);
       `testSyntheticTypedDataEdit` (always runs): typed decode, property
       edit + u8 clamping, untouched-identical, undoable disposition/grid
       edits round-tripping positionally.
     Full Morrowind.esm round-trip still byte-identical; suite 131/131,
     zero-warning build, lint clean.
    TES3 Phase 2c final scope (closed below): specialised Morrowind
    record-family editors (the generic QtFormDialog path already opens every
    TES3 record with the typed DATA fields).

     **Status 2026-09-22 (specialised Morrowind editors — Phase 2c done):**
     done.
     - Object Window gains a Morrowind category tree in
       `ObjectWindowModel::initCategories` / `rebuildAllRecords`
       (Body/Levc/Levi/Lock/Pgrd/Prob/Repa/Sndg/Skil group additions);
       `editSelected` / `openRecordEditor` / `getFormComponentsForIndex`
       early-route every TES3 type through `getCollectionByType` +
       `resolveComponents` before the Starfield/NVSE switch, opening under
       a `T3:<code>` factory key so TES3 and TES4 codes never clobber each
       other.
     - `Data::tes3TypeForName` is table-driven; `Data::tes3MappedCodes()`
       enumerates all 41 mapped codes for factory registration.
     - New read-only `Tes3RecordDataWidget` (`T3:` factories in
       `ObjectWindowDialog::factoriesRegistered`) shows record code /
       editor id / form id / flags plus the raw-subrecord table.
     - `test_tes3recorddatawidget`: mapped-codes round-trip, null +
       synthetic widget fields, `T3:` factory key via `openOrFocus`.
     TES3 Phase 2c complete.

---

## 4. Test infrastructure

1. **`OPENCK_DATA_DIR`** env var honored by all real-data tests (currently
    hardcoded to `C:/XboxGames/Starfield/Content/Data`); skip cleanly when
    absent.

    **Status 2026-09-08:** `editor.cpp` now checks `OPENCK_DATA_DIR` env var
    (takes priority over config file, before auto-detection). All real-data
    tests now read the env var with fallback to the hardcoded path.

    **Status 2026-09-13 (skip hardening):** 12 existence gates across 9
    real-data tests (`test_bsaarchive` x4, `test_xwmadecoder`,
    `test_starfieldesm` x2, `test_groundtruth`, `test_subrecord_roundtrip`,
    `test_worldspacerecord`, `test_pndrecord`, `test_btdterrain`,
    `test_hknpphysicssystem`) were converted from hard `QVERIFY` to `QSKIP`,
    so a data-less machine skips instead of failing — verified by running
    all 9 with a bogus `OPENCK_DATA_DIR` (exit 0, skips recorded) and again
    with real data (exit 0, passes). The `if (EXISTS hardcoded-path)` CMake
    guards for pndrecord/worldspacerecord/bsaarchive were replaced with
    unconditional registration since the tests now skip at runtime and honor
    the env var. Self-created-output checks (pluginio, loader save paths,
    nifanimation temps, ba2/bsa write round-trips) intentionally stay
    `QVERIFY`.
2. **Materialization matrix test** — index count vs. loaded count vs. warning
   count for every type from a full master load; assert warnings == 0 or an
   explicitly shrinking allowlist.

    **Status 2026-09-13:** Verified in place.
    `test_loader::testMaterializationMatrixZeroWarnings` loads Starfield.esm
    + Magnus.esm + Vvardenfell.esp, materializes every type present in the
    master index (per-type counts via `ensureTypeLoaded`, heap-validated per
    type with `HeapValidate` + `_CrtCheckMemory`), and asserts the reader
    warning list is empty afterwards. Passes in the suite (3.8M records).
3. **Per-type round-trip subrecord-diff tests** (the tool from §1.2) as part of
   the suite.

    **Status 2026-09-13:** Covered.
    `testSaveRoundTripSubrecordIdentical` (Vvardenfell.esp, 1374/1374 records,
    0 mismatches), `testSyntheticMultiTypeRoundTrip` (synthetic NPC_/GLOB/
    STAT/WRLD), and `test_tes3roundtrip` (full 48,295-record Morrowind.esm,
    byte-identical) are all registered QTest cases and green.
4. **Fake-data lint** — CI grep that fails on hardcoded game-content strings in
   `src/` so sample data comes from fixtures.

    **Status 2026-09-13:** Implemented.
    `tools/fakedata-lint.ps1` scans `src/**/*.cpp|hpp` for the regexes in
    `tools/fakedata-lint-patterns.txt` (placeholder text, hardcoded
    game-install roots, content proper nouns, sample-content markers) with
    per-hit excuses in `tools/fakedata-lint-allowlist.txt` (path + pattern +
    reason; currently 6 entries for legitimate tool-detection fallbacks).
    Verified both directions (clean on the 510-file baseline; fails on a
    planted violation, then removed). Wired into CI as the first step of
    `windows-build.yml`.
5. **API doc comments** (Doxygen) on the main public interfaces (`Data`,
   `NifPyFileWrapper`, `BlenderLauncher`, `ShortcutManager`).

    **Status 2026-09-13:** Verified present.
    `Data` (~995 doc lines), `BlenderLauncher`, `ShortcutManager` and
    `NifPyFileWrapper` (class + every public method/struct with
    `///`/`/** */` comments, params and returns) are all documented.
6. **Final build gate** — zero-warning clean build, all tests, memory-leak
   check, coverage target.

    **Status 2026-09-13:** Implemented as `tools/gate.ps1` (lint → build
    `all_tests` failing on any MSVC warning outside `external/` → full
    `ctest --output-on-failure`). Verified with a from-scratch Release
    rebuild into a separate build dir: the first clean build exposed 25,782
    warning lines, all fixed or justified —
    C4373 (~4.3k template-amplified diagnostics) fixed properly by dropping
    top-level `const` from three `Collection<T>` override declarations
    (`getId`/`replace`/`getRecord`) to match `BaseCollection` and the
    out-of-line definitions; C4714 (Qt-internal `__forceinline` noise)
    disabled project-wide with a comment, following the existing `/wd4100`
    precedent; 8 real diagnostics fixed (merged `#include` lines, unused
    locals, `int`→`quint32` casts, a shadowed `found`, `%d`→`%lld` for
    `qsizetype` printf args, `getenv`→`qEnvironmentVariable`). Rebuild is
    zero-warning; full `ctest` is 124/124. The same run caught a stale
    `all_tests` DEPENDS list (8 newer tests missing, 2 removed tests still
    listed) — now verified identical to the built set, which is also what
    CI's build step compiles.     Leak coverage comes from the in-suite
    `HeapValidate`/`_CrtCheckMemory` instrumentation in the matrix test.
    Coverage is verified working via `tools/coverage.ps1`: it builds the
    requested tests in a `RelWithDebInfo` directory (PDBs are required —
    the default Release build emits none), runs each under OpenCppCoverage,
    and prints per-test line rates from the Cobertura XML (verified:
    `test_opallist` 94.6%, `test_particlesimulation` 93.3%, including the
    script's auto-build path). OpenCppCoverage needs a one-time elevated
    install; `dotnet-coverage` cannot substitute (it attaches via the CLR
    profiling API, which never initializes in pure-native processes).
    Full-suite coverage is slow (instrumentation overhead on the real-data
    tests) — treat it as a nightly job, use `-Tests` for slices.
7. **CTest registration** for the 3 remaining non-QTest binaries
   (`dumpesm`, `scanbtd`, `meshprobe`).

    **Status 2026-09-13:** Done.
    All three exit 0 with no arguments (usage / "no such dir" / "open
    failed"), so they are registered via `openck_add_test` with no fixture
    dependency. `ctest -N` lists 124 tests; the full run is 100% green.

---

## 5. Code-quality debt (verified still open)

1. `Data::createNewRecord` brute-forces FormID allocation (re-scans
   `allCollections()` per candidate); cache the used-FormID set.

   **Status 2026-09-07:** Added `mNextLocalId` counter that persists across
   calls, so subsequent allocations skip already-scanned IDs. Counter resets
   on `removeRecord`. Build + 108 tests pass.
2. Editor `saveRecord()` paths that validate **after** mutating the record
   (partial mutation on validation failure) — validate first, commit via a
   temp copy.

   **Status 2026-09-07:** Audit of all 29 editors with ColumnValidator
   confirmed none exhibit this bug. All follow the correct pattern:
   validate → then mutate. Stat/tree editors use a probe copy inside
   `validate()` — safe.
 3. `LandscapeEditCommand` rewrites the full heightmap per brush stroke and
    stores unused params — implement partial updates or drop the params.

    **Status 2026-09-08:** Implemented partial updates. `LandscapeEditCommand`
    now stores only the dirty bounding-box region (x, y, width, height +
    per-region data) instead of two full heightmaps. `LandscapeEditor` tracks
    `strokeDirtyRect` during brush strokes and extracts only that region on
    mouse release. Full-heightmap operations (paste, import R32) still use
    the full region (0, 0, terrainSize, terrainSize). Memory per undo entry
    dropped from 2×N² floats to 2×(region) floats.
4. `Logger` singleton is not thread-safe before init and never restores the
   Qt message handler — guard + restore.

   **Status 2026-09-07:** Logger already uses `QRecursiveMutex` on all public
   methods and has a `m_preInitBuffer` that queues messages before `init()`.
   No Qt message handler is installed (no restore needed). Already addressed.
 5. CMake: no install targets, Qt6 path hardcoded, a couple of missing
    component deps for Qt5.

    **Status 2026-09-08:** Install targets already existed (lines 1313+).
    Qt6 auto-detection fixed: replaced the hardcoded `6.7.x` list with a
    `file(GLOB "C:/Qt/6.*")` + `msvc*` ABI glob, sorted descending so the
    newest version wins. Verified it finds `C:/Qt/6.5.3/msvc2019_64` on
    this machine. Qt5 deps not relevant (project is Qt6-only).
6. 7 remaining `const_cast` call sites in `src/` — add proper mutable getters.

    **Status 2026-09-07:** Fixed 4 of 7 sites:
    - `landscapeeditor.cpp`: 2 sites — removed unnecessary casts (mutable
      getters already exist for `getLandCollection()`/`getCellCollection()`).
    - `objectwindowdialog.cpp`: 2 sites — changed `const auto&` to `auto&`
      for `getRefrCollection()`, removed casts.
    - Remaining 3: `nodegraphwidget.cpp` (intentional — graph API only
      exposes const nodes), `data.cpp` x2 (standard
      const-delegates-to-non-const pattern). All safe, no fix needed.

    **Status 2026-09-13 (re-audit):** a 4th site had appeared since the
    audit — `mainwindow.cpp` (`const_cast<FilePaths&>(mData->getPaths())`
    in the Convert-to-ESL flow). Fixed properly per the item: added a
    non-const `Data::getPaths()` overload and dropped the cast. Verified
    the other §5 claims still hold (`mNextLocalId` persists/resets;
    `strokeDirtyRect` partial updates; logger mutex + pre-init buffer with
    no message-handler install; install targets + Qt GLOB; stat/glob
    editors validate-before-mutate; no real TODO/FIXME — the hits are
    `toDouble` and the `XXXX` subrecord name; no `.bak` files, only
    vendored Blender binaries under `external/`). Full build + 124/124
    green.
 7. Dead-code sweep: unused stubs, duplicate enums, `Q_UNUSED` params, commented
     blocks, `catch(...)` sites.

     **Status 2026-09-08:** Audited. No commented-out blocks, no TODO/FIXME
     markers, no truly dead functions. All 55 `Q_UNUSED` sites are in Qt
     virtual overrides (parameter required by signature — correct pattern).
     Both `catch(...)` blocks are top-level safety nets in `main.cpp` and
     `crashhandler.cpp` (legitimate). Nothing to remove.

     **Status 2026-09-13 (file-level re-audit):** the 09-08 audit covered
     functions, not files — 12 source files were uncompiled AND unreferenced
     (verified: absent from every CMakeLists, no `#include`, no symbol use,
     no `.ui`/docs references) and are now deleted: `genericrecordeditor.*`
     (save path never validated, nothing opens it), `globeditor.*`
     (superseded by `globvar_editor` + the water-editor GLOB flow),
     `npcvalidator.cpp`/`questvalidator.cpp`/`weaponvalidator.cpp` (hazardous
     out-of-line duplicates of the header-inline implementations — compiling
     them would break the link; headers stay), `nifviewport.*` (first-gen
     viewport superseded by `NifViewportWidget`), `worldviewwidget.*`
     (~42 KB, unreferenced), `mainwindow_construction.cpp` (10-line orphaned
     constructor fragment). The `catch(...)` claim needed one correction:
     `crashhandler.cpp` was never compiled and `installCrashHandlers()` never
     called, so the documented crash reporter was dormant — it is now wired
     up (`openck_files` target, called from `main()` after logger init) with
     two latent bugs fixed (missing `<csignal>`, duplicate
     `EXCEPTION_ACCESS_VIOLATION` case) and covered by `test_crashhandler`
     (5/5: install, stack trace, crash bundle). Full build + 125/125 green.
 8. Stale `.bak` files and `external/vorbis` build outputs clutter the tree —
    add a cleanup rule + gitignore.

    **Status 2026-09-08:** No `.bak` files or build outputs found in the
    tree. Already clean.

---

## 6. Intentionally closed — do NOT re-add

- **Cell-transitions editing** — the TES4 format stores no cell-connection
  data; the editor is honestly read-only.
- **SCEN PHDA binary encoding** — no shipped game has a PHDA subrecord.
- **Top-level `CCT_` record** — does not exist in the real format; creature
  attach points are `ap_CCT_*` EDID markers.
- **Flat mirror fields** (`containerItems`, `keywords`, `spells`) — kept
  intentionally for back-compat; audited.
- **VC server preferences field** — removed intentionally.
- **Subrecord spelling conversions** (BYDT→ATTR etc.) — fixed; components now
  re-emit the exact spelling they loaded.
- **Deferred-master synchronous expansion** — replaced by time-sliced
  materialization; the fetchMore model-reset crash cannot recur.
- **"REFR materialization heap corruption (0xC0000374)"** — the 2026-08-25
  entry below attributed it to a registry page-heap configuration, but on
  2026-09-03 the crash reproduced deterministically at the REFR transition
  (exit 0xC0000374 in both Debug and Release on the restored tree) and was
  fixed by guarding the `BGSRefData_Component` fixed-width subrecord reads
  to consume only declared bytes (see §1.1 status 2026-09-03): it was a real
  parser over-read after all. The 08-25 "non-reproducible" verdict is
  superseded; the page-heap/VEH/`_CrtCheckMemory` instrumentation in
  `test_loader.cpp` stays as the detector kit. Debug full-matrix runs that
  die near 300s are the QTest function timeout + teardown race, not this
  bug. Machine-local note: HKLM IFEO `test_loader.exe` held inert values
  (`GlobalFlag=0x10000000` enables nothing) — removed 2026-09-04 via
  elevated cleanup; the `phcanary.exe` key was already absent.

---

## 7. How to verify progress

- `cmake --build build --config Release` clean.
- All `test_*.exe` in `build/bin/Release/` exit 0 (currently 134, excluding
  the standalone `test_subrecord_diff` CLI). `test_subrecord_diff.exe` is a
  CLI diff tool, not a test: exit 2 means "usage error" when run without its
  two file arguments, by design — exclude it from the glob.
- `openck --cli info <SeydaNeen.esp>` exits 0.
- `docs/record_formats.md` warning rows trend to zero.
- `tools/gen_record_audit.ps1` regenerates `docs/record_formats.md`.

---

## 8. Phase 5 — in-viewport 3D playback (scoped from the code, 2026-09-13)

The §3.2/§3.5 "Phase 5 scope" notes understated what is already wired.
Verified by reading `NifViewportWidget` + `NifAnimationState` (no new code —
this section is documentation of findings, per the §3 review).

**Already working end-to-end:**
- Animation: `NifAnimationState`'s QTimer ticks → `timeChanged` → timeline
  slider/label update + `glWidget->update()` → `renderMesh()` calls
  `applyAnimationFrame()` whenever playing → node cumulative transforms are
  recomputed with animation overrides → rest-pose vertices (`restVertices` /
  `restNormals`) are deformed on CPU with normal-matrix correction →
  `m_meshDirty` triggers VBO re-upload. Rigid/prop animation plays.
- Particles: `initParticleSystems()` parses the NIF's effects and shows the
  toolbar, play/pause/stop drive `ParticleSystem`'s timer,
  `ParticleRenderer::render` runs inside `renderMesh()`, and
  `ParticleSystem::updated` → `glWidget->update()` closes the repaint loop.
  The simulation math is unit-tested (`test_particlesimulation`) after the
  `ParticleSimulation` extraction, which kept the viewport path intact.

**The actual remaining gaps (this is the Phase 5 work):**
1. **Per-vertex skinning — DONE 2026-09-14 (CPU).** `NifSkinInstance` /
   `NifSkinData` dialect blocks (`libs/files/esm/nifrecord.*`) parse and
   round-trip; `extractGeometry` links instances to shapes (bone refs
   resolved to scene nodes, out-of-range data warned and skipped);
   `applyAnimationFrame` blends skinned shapes through a shared GUI-free
   core (`libs/files/nif/nifskinning.*`: `v' = OwnerWorld * Σ w·(BoneWorld·
   BindInverse)·v`, per-vertex weight normalization, unweighted fallback to
   rigid) — single-bone weight-1.0 reproduces the rigid path exactly.
   `test_nifskinning` 15/15 (blend math, block codec, synthetic file load→
   link, playback composition, real-file canary). The work also fixed two
   latent loader bugs the tests exposed: `NiTriShapeData` misdispatched as
   `NiTriShape` (prefix match order), and a zero-progress infinite loop in
   `parseAllBlocks` on non-dialect input (now breaks cleanly). GPU skinning
   landed 2026-09-21 (see §8.3).
2. **Quaternion-correct interpolation — DONE 2026-09-14.** `AnimKeyframe`
   / `TransformKeyframe` carry `qw..qz` + `hasQuat` alongside the Euler
   angles; `interpolateChannel` slerps when both endpoints have quats
   (Euler derived from the result for legacy consumers) and keeps Euler
   lerp otherwise; the viewport imports quats from keyframe controllers
   and renders rotation straight from quat matrices; the blend path uses
   stored quats; JSON/XML persist quats when present (old files keep the
   Euler path). `test_nifanimation` 11/11, incl. the 350°→−5° short-path
   proof (Euler lerp would sit at 175°).
 3. **Verification gate on real assets — HEADLESS COMPLETE; DISPLAY
    CONFIRMATION OUTSTANDING (2026-09-22).** The Gamebryo 20.2.0.7 block
    reader is now in the tree
    (`libs/files/nif/nifparser.cpp`, `Gamebryo::` section; `NifParser::load`
    routes by magic, dialect path untouched). It strictly parses the real
    header (magic line, version dword, BS172 `BSStreamHeader` with
    byte-length `ExportString` tail, type table, per-block size table,
    string table, footer — validated byte-exact in Python against all 255
    loose shipped NIFs), dispatches blocks by type index with exact
    seek-by-size (unknown blocks skipped, never guessed), builds `NiNode`
    hierarchies with string-table names and child refs, and decodes the
    Starfield `BSGeometry` shell (bounds, box, skin/shader/alpha refs, 4
    mesh slots) from NifSkope's `#STF#` definitions. Layouts that were
    guessed wrong first (FO4 `BSTriShape` inline vertices) were corrected
    against real bytes: shipped statics carry **external `.mesh` paths**
    (meshes live in BA2s), recorded on the parser via
    `NifParser::externalMeshRefs()`. `testRealNifSurvey` flipped: 8/8
    shipped files load with 66 named nodes + 205 external mesh refs and 0
    local verts. **Vertices 2026-09-15:** the external paths resolve —
    NIF mesh path + `geometries/` prefix + `.mesh` suffix addresses
    Meshes01.ba2 directly (320,483 files; verified by extraction), and
    `Nif::parseBsMeshData` decodes the `.mesh` stream (scaled ShortVector3
    verts, half UVs, BGRA colors, packed normals/tangents, weights, LODs,
    meshlets, cull — exact full-buffer consumption). `testExternalMeshData`
    proves it on a shipped mesh (16 verts, 8 tris, valid indices). New
    diagnostic `test_meshresolve` finds/extracts BA2 entries by path.
    Debugging footnote: `QDataStream::operator>>(float&)` was observed
    consuming 8 bytes (not 4) in this build — the mesh parser reads float
    bits as u32 + memcpy (see AGENTS.md gotchas).
    **Auto-resolution 2026-09-16:** `Nif::MeshArchiveResolver`
    (`nifparser.*`, process-wide BA2 cache) maps NIF mesh paths to mesh
    BA2 entries — hash form `h1\h2` and full `Geometries\h1\h2[.mesh]`
    address `geometries/h1/h2.mesh` directly; name-style paths
    (`SomeFolder\SomeMesh`) match no shipped archive entry anywhere and
    stay unresolved by design (no substring guessing). Resolved meshes
    decode into synthetic data blocks so `extractGeometry` yields real
    shapes: the survey now reports 53,715 verts over the 8 files
    (`totalVerts > 0` replaces the old `== 0` tripwire). Vertex units
    settled empirically: meters = int16 × vertexScale / 65536 (bound
    sphere/box ratios exact on 6 axes across 2 scales). `Ba2Archive`
    gained `extractToBytes` (backing `extract()`); `totalVertexCount` /
    `shapeCount` now recurse the whole tree (they previously counted the
    root only — the tree was always fully populated).
    **Skinned meshes 2026-09-16:** Starfield does not use NiSkin — faces
    use `BSSkin::Instance` + `BSSkin::BoneData` + `SkinAttach` triplets
    (positional: geometry, extras, attach, instance, bonedata; a
    `BSClothExtraData` may sit between attach and instance; the geometry's
    skin ref points at its instance directly). Layouts validated across
    15 triplets in 2 shipped face NIFs: attach = u32 unk(4) + names;
    instance = target/bonedata/n + i32(-1) + 16 raw bytes per bone;
    bonedata = count + per-bone 4×4 matrix + scale float (~1.0).
    `BSFaceGenNiNode` roots parse as `NiNode` + 2 tail bytes. Bone names
    link onto `TriShape.skinBones` (nodes null, weights empty — rigid
    fallback preserved); `testFaceSkinBlocks` proves it on a shipped face
    (10 shapes, 10 skinned, 129 named bones, 53,444 verts).
    **Per-vertex weights 2026-09-21:** the `.mesh` weight stream is no
    longer dropped. `NifTriShapeData` now carries
    `skinWeightsPerVertex` + parallel `skinBoneIndices`/`skinBoneWeights`
    (the synthetic block the external-mesh resolver builds), and the
    BSSkin triplet link converts them into `TriShape::skinWeights`
    (`weight = weightRaw / 65535`, zero-weight slots skipped). The bone
    index is **attach-local**: across all 10 shipped face shapes
    `maxBone == attachBones - 1` exactly (2→1, 14→13, 50→49, 54→53, …),
    and `BSSkin::Instance` bone count == attach name count, so the mesh
    index addresses `skinBones[i]` directly. `testFaceSkinBlocks` now
    asserts 10/10 shapes weighted, 223,584 weights, every bone/vertex
    index in range, per-vertex weights partitioning to 1.0, and a
    bind-pose blend (identity palettes — the exact viewport state when no
    animated skeleton is present) reproducing the rest pose vertex- and
    normal-exact through `Nif::blendSkinnedLocal`. The viewport already
    degrades to rigid when `boneNode` is null, so this changes no
    rendering until a skeleton resolves. The `BSSkin::Instance` 16-byte
    per-bone payload is **not** topology: only 5 distinct values across
    the 50-bone head (bit patterns = 1.0f, 0, -1, and a small int),
    matching the earlier 76-bone observation — kept raw, not interpreted.
    **Current residual (2026-09-22):** the 16-byte per-bone payload remains
    intentionally raw. Skeleton resolution, bind-pose and deformation tests,
    CPU/GPU animated blend, and headless particle playback wiring are complete.
    The remaining work is display-dependent manual Play-confirm on a skinned
    animated mesh and a particle mesh, plus visual confirmation of the GPU
    shader. The `.ffxanim` FaceFX data source remains unsupported by design;
    it is middleware runtime work, not a file-format parser gap.

    **Skeleton resolution 2026-09-21:** the skeleton asset is shipped in
    `Starfield - Meshes01.ba2` as `meshes/actors/<race>/characterassets/
    skeleton.nif` (+ `_facebones` variant; e.g. `human/characterassets/
    female/skeleton_facebones.nif`). `NifParser::load(path, skeletonPath)`
    / `attachSkeleton()` loads it, merges its node tree under the main
    root (so the viewport's rest-pose walk captures bind inverses) and
    resolves BSSkin bone names to nodes. All 50 face-bone names in the
    shipped female facegeom match the skeleton exactly. `testSkeletonProbe`
    now loads the face + skeleton, asserts every head bone resolves, that
    the bind-pose blend reproduces rest, and that translating the
    most-weighted bone deforms exactly the vertices weighted to it or its
    subtree (and no others) — real Starfield skinning, headlessly. Note the
    *animation* stream is a separate undocumented format (`.ffxanim` in
    `Starfield - FaceAnimation0*.ba2`), so animated playback still needs
    that decoder.

    **GPU skinning 2026-09-21:** the viewport vertex shader now supports
    `uSkinned` + `uBones[128]` with per-vertex 8-slot bone index/weight
    attributes (locations 4–7, 28 floats/vertex); skinned shapes upload
    rest vertices and let the shader blend `Σ w·(owner·boneWorld·
    bindInverse)`, while rigid shapes keep the CPU path and `uSkinned`
    false. `Nif::packGpuSkinInfluences` (GUI-free, in `nifskinning.*`)
    packs and normalizes influences; `testGpuSkinPacking` /
    `testGpuSkinMatchesCpu` pin the packing (normalization, 8-slot cap,
    out-of-range rejection) and that the GPU sum matches the CPU blend.
    Shapes over the 128-bone budget or without bones fall back to CPU.
    `setGpuSkinningEnabled()` toggles it (forces a VBO rebuild). The shader
    itself needs a display to confirm visually.

    **Face animation (`__ffx`) 2026-09-21 — investigated, out of scope.**
    Shipped face motion is not in the NIF: it is FaceFX middleware data.
    `Starfield - FaceAnimation0*.ba2` holds `*.ffxanim` (76,659 entries in
    vol. 01) carrying `__ffx\0` + version + u32 size + 20-byte id (last 8
    bytes constant) + u32 record count, then `count` **12-byte records**
    (`f32 value` + 4×u16; ~94 distinct channel ids, sparsely interleaved by
    time). The channel→bone map lives in the **`.facefx` actor** (present
    only in `Content/Tools/FaceFX/StarfieldHumanFemale.facefx`, 81 KB):
    `FACE{` + `ZeniMax Media` + a typed object stream (`FxActor`,
    `FxCompiledFaceGraph`, `FxMasterBoneList`, `FxNamedObject`, `FxName`…)
    whose nodes are graph controls (`browLowererL`, `Eyebrow Raise`, `Eye
    Yaw`) — not the `faceBone_*` skeleton names. Playing these therefore
     means reimplementing the FaceFX runtime (evaluate the compiled face
     graph to bone transforms per frame); OC3 themselves state they do not
     know Starfield's exact implementation. Not attempted: it is a
     middleware reimplementation, not a file-format decode, and no public
     spec exists. The skinned pipeline it would feed is complete and tested;
     only this data source is unsupported.

     **`.facefx` partial parser 2026-09-22:** `libs/files/facefx/facefxactor.*`
     decodes the container framing (header magic/version, typed object
     stream, node-name table) and rejects anything it does not recognize
     rather than guessing; `test_facefxactor` covers the four actors under
     `Content/Tools/FaceFX/` (plus a real-data gate via
     `OPENCK_TEST_FACEFX_DIR`). Deeper `FxCompiledFaceGraph` evaluation
     remains out of scope with the runtime reimplementation above.

## 9. Series items — Creation Kit parity audit

**Audit date:** 2026-09-25. This section records nine concrete series of
remaining work, followed by additional gaps found by inspecting the local
Starfield/Creation Kit installation. The audit is read-only: no game assets,
DLLs, source code, or proprietary text are copied into OpenCK.

**Local evidence inspected:** the Steam installation at
`C:\\Program Files (x86)\\Steam\\steamapps\\common\\Starfield` (including
`CreationKit.exe`, `Tools`, `Data`, editor assets, and Qt5 runtime files), the
alternate CK/game root at `C:\\XboxGames\\Starfield\\Content`, and the deployed
Vortex staging directory. The CK installation exposes Qt5 Core, Gui, Network,
OpenGL, SQL, Svg, Widgets, Qt Advanced Docking, platform plugins, and style
plugins. OpenCK uses Qt6 and its own vendored ADS build; it must not copy or
redistribute the CK's Qt deployment. Any compatibility work must use public Qt
APIs or remain inside OpenCK's existing Qt6/ADS implementation.

### Series 1 — Transactional generic Object Window editing

**Priority: P0 correctness.** `ObjectWindowDialog::editSelected()` passes live
record components into dialogs, while `QtFormDialogManager::openOrFocus()` has
no original snapshot, commit callback, collection/index binding, or undo
transaction. `FormComponentWidget::apply()` is effectively a no-op, and
`Document` cannot detect an in-place base-record mutation that never enters the
undo stack.

Replace this with a working-copy edit session: clone the record/components,
bind the dialog to the copy, validate and compare on OK, push
`EditRecordCommand` only after a successful commit, and discard on Cancel or
close. **Status 2026-09-26 — done.** `RecordEditSession`
(`src/view/window/recordeditsession.hpp`) owns a working copy
of the whole record. The dialog's property grid and any custom data widget edit
*that* copy, so only `commit()` writes the live record and it always goes through
the document's undo stack; `discard()` drops the copy, and `QtFormDialog::reject()`
calls it so both Cancel and closing the window discard. Because the copy is a
whole record rather than just its components, a widget writing plain fields is
covered for free — no per-widget snapshot logic. `HasFormComponents<T>` gates
which types expose working components. Custom data widgets implement
`FormDataWidget` (`src/view/widgets/formdatawidget.hpp`) for `loadSession()`,
`validateSession()` and `applySession()`; a failing `validateSession()` blocks
the commit and shows the reason.

Every edit route now builds a session, so no production path hands a dialog the
live record: the 40-odd `openTransactionalForm<T>()` call sites, the Morrowind
branch and the generic non-Morrowind branch of `ObjectWindowDialog::editSelected()`,
the Object Window's post-create dialog in `MainWindow`, and both Search dialog
routes. The Morrowind and fallback branches only hold a `BaseCollection`, so
`resolveEditSession()` builds the typed session for them from the same
record-type list `resolveComponents()` uses. `test_recordeditsession` pins that a
pending edit touches neither components nor plain fields nor the record's state,
that discard leaves everything alone, that commit writes both kinds of change as
one undo entry, and that commit is idempotent.

This supersedes the previous revision's `EditComponentsCommand` and
`BaseRecord::activeComponents()`, which existed only to give the Morrowind route
a type-erased path; the session is strictly stronger, so both were removed rather
than left as dead code.

**Series 1 is now done.** The full-record session is described above. The last
piece was the custom data widgets themselves: of the 16 `*DataWidget`
implementations, only `InfoDataWidget`, `QuestDataWidget` and `WorldspaceDataWidget`
ever wrote to the record, so the other 13 read the record into controls and threw
the user's edits away. All 12 of those now implement `FormDataWidget` —
`loadSession()` repopulates the controls from the session's working record,
`validateSession()` rejects malformed FormIDs with a message naming the offending
row, and `applySession()` writes the controls back into the working record.
`Tes3RecordDataWidget` is deliberately still read-only, as documented on the
class. Controls are found by `objectName` in the session methods, matching the
convention `Tes3RecordDataWidget` already used.

Fixing this surfaced two silent-truncation bugs of the same family Series 2 dealt
with, where a spin box range did not match the field it fed:

- `HazdRecord.limit`, `.target` and `.flags` are `quint8`, but the spin boxes
  allowed `INT_MAX`, so any value above 255 was silently truncated. Capped at 255.
- `CellRecord.cellX`/`.cellY` are `quint32`, but the spin boxes accepted
  -99999..99999, so a negative coordinate became a huge unsigned value. Interior
  cell coordinates are non-negative, so the range is now 0..99999.

`test_recorddatawidgets` drives all 12 the way the dialog does — construct with the
working record, edit a control, validate, apply — and checks the value reached the
record through `commit()` and came back through `undo()`, that untouched fields
survive, that a refused FormID writes nothing, and that every widget is actually
discovered as a `FormDataWidget` (a silent failure to derive from the interface
would otherwise make a widget stop being applied). Its hex parsing is pinned
separately. Checked with a negative control to confirm the assertions bite.

One maintenance hazard left in place: `QtFormDialogManager::openOrFocus()` still
has the legacy overload that takes a live `recordPtr`, which is exactly the trap
this series existed to close. It is kept for dialogs with no record to edit, is
commented as a footgun in the header, and nothing in production calls it. It
should be deleted once no caller needs it.

### Series 2 — Width-correct component property bindings

**Priority: P0 memory safety.** `IntEditorProperty` and `EnumEditorProperty`
write 32-bit values through pointers, while several tier-3 components expose
`quint8`, `qint8`, `quint16`, and `qint16` members by casting them to
32-bit pointers. Editing these properties can overwrite adjacent bytes.
Existing tier-3 tests cover clone/copy/equality but do not call the generated
editor properties.

Add typed 8/16/32-bit properties or callback-based accessors, retain range
metadata, and add canary-neighbor tests for every generated binding. No
property may reinterpret a narrow record field as a wider integer. **Status
2026-09-25 — done.** `IntegerEditorProperty` now provides a common clamped UI
contract for signed/unsigned 8/16/32-bit values, `UInt8EnumEditorProperty`
preserves the AI enum editor, and all Tier 3 ACBS/AIDT bindings use typed
fields. Regression checks verify neighboring fields remain unchanged; the full
134-test CTest suite and `tools/fakedata-lint.ps1` pass.

### Series 3 — Real CELL child references

**Priority: P0 save integrity.** `CellReferenceEditor` treats embedded raw
`REFR` subrecords as children and `CellEditor::openReferences()` writes them
back into `CellRecord::rawSubRecords`. The actual model and save path already
represent placed references as separate records under CELL children GRUPs via
`Data::childrenOfCell()` and `Data::setRefrParentCell()`.

Remove the embedded-record path. Populate the editor from real `RefrRecord`
children, create/remove actual child records, allocate FormIDs, and batch the
collection, parent index, and child-group changes through one undo command.
Acceptance requires a synthetic CELL/REFR save-reload and undo/redo test. **Status
2026-09-25 — done.** `CellReferenceEditor` now edits a working list sourced from
real `RefrRecord` children. `CellEditor` allocates FormIDs, inserts/removes typed
REFR records, updates parent/order indexes through one `MacroCommand`, and
removes legacy embedded REFR subrecords before the outer cell commit. Synthetic
coverage verifies add/remove, save/reload, and undo/redo.

### Series 4 — New-plugin and new-record workflow

**Priority: P1 authoring viability.** The user guide describes game/master
selection for new plugins, but the actual new-plugin dialog only supplies a
filename and empty content-file list. The Object Window Add path is limited,
the NPC/RACE/CLAS/FACT creation path is hard-coded and bypasses the normal
add command, and some paste paths create records with FormID zero.

Add a new-plugin wizard for game, active master order, plugin/light type,
author, and next local FormID. Add a table-driven blank-record factory for
Object Window categories with defaults and required components, and route all
creation through `AddRecordCommand` with valid ID allocation. Synthetic tests
must cover header masters, game detection, local IDs, save/reload, and undo. **Status
2026-09-25 — core done.** The new-plugin flow now collects game, active master
order, plugin/light type, author, and next local FormID; configured headers save
and reload correctly. `BlankRecordFactory` plus `AddRecordCommand` covers GLOB,
GMST, NPC_, RACE, CLAS, and FACT creation, and paste paths allocate nonzero IDs.
The remaining work is routing legacy type-specific paste branches through undo
commands and broadening the factory to every Object Window category.

### Series 5 — Record-specific tabs and real form pickers

**Priority: P1 workflow parity.** Several custom data widgets create controls
without signal/commit paths, including NPC, CREA, FACT, CLAS, and PACK;
`RaceDataWidget` edits only a local list; container/keyword/spell tables do not
consistently commit their vectors; FormID controls are raw hexadecimal text;
and a SCEN factory is overwritten by a read-only table factory.

Build a `FormPickerWidget` backed by a form index with type filtering, search,
duplicate detection, and navigation. Give every custom widget load/validate/
apply behavior and table models with undoable commits. Remove the overwritten
SCEN registration or implement a persisted editor. Add offscreen Qt tests for
each widget and picker workflow. **Status 2026-09-25 — picker core done.**
`FormPickerWidget` now provides indexed search, type filtering, duplicate
FormID detection, previous/next navigation, and is used by component FormID
properties and CELL reference base-object editing. Existing custom record
widgets still need full load/validate/apply transactions and persisted table
models.

### Series 6 — Archive orchestration, older BSA targets, and extraction safety

**Priority: P1 file workflow.** The current v0x69 writer now has correct folder
records, LZ4 frame compression, stored fallbacks, independent table tests, and
product UI wiring. Remaining gaps are explicit source-root/archive-path
mapping, v0x67/v0.68 BSA targets for Oblivion/Fallout/Skyrim, explicit BA2
GNRL/DX10 selection, and containment-safe bulk extraction. The current UI
passes absolute paths to the writer, which reconstructs the root from the
output directory; `ArchiveBrowserDialog::extractAll()` does not canonicalize
entry names before joining them to the destination.

Introduce explicit `sourcePath`/`archivePath` entries or a source root,
versioned archive targets, and canonical extraction checks that reject absolute,
drive-qualified, UNC, and `..` paths. Use `QSaveFile` for output replacement.
Synthetic malicious-path, Unicode-name, duplicate-name, differing-root, and
v0.67/68/69 fixtures are required. **Status 2026-09-25 — core done.** BSA
creation now accepts an explicit source root, supports v0x67/v0x68 zlib targets
alongside v0x69 LZ4, and writes extracted files through `QSaveFile`. Archive
browser bulk extraction rejects absolute, drive-qualified, UNC, and `..` entry
paths before joining to the destination. Tests cover source-root preservation,
older compressed targets, and malicious paths; broader archive UX and game-
specific target presets remain.

### Series 7 — Save-time and interactive validation

**Priority: P1 safety net.** The current validation command covers only
NPC, weapon, quest, and coverage validators. A separate plugin validator uses
an obsolete raw header layout and does not match the current 24-byte record
writer. There is no cross-record unresolved-FormID index, missing-master
check, parent/worldspace/cell validation, required-component check, or
navigable diagnostic model.

Build a Data-level reference index and per-record rule table covering unresolved
FormIDs, missing masters, invalid relationships, duplicate IDs, missing scripts,
quest/dialogue links, required components, and escaping asset paths. Add an
optional pre-save severity policy and remove the obsolete raw-byte validator
from the active path. Synthetic plugin graphs must prove every diagnostic and
its source location. **Status 2026-09-25 — core expanded.** `AssetValidator`
now checks declared master files, zero FormIDs, duplicate IDs, unresolved
cross-record references, and orphaned records through the Data-level form index;
the existing navigable report/filter/export dialog remains active. Broader
relationship-specific rules and save-policy integration remain.

### Series 8 — Replace pseudo-save editors with active-document transactions

**Priority: P1 correctness.** Weather/light and water editors discover GMST
records by EditorID substring and write standalone flat streams instead of
saving the active document. AI Package and Dialogue Tree save paths call
record operations without first saving their writer streams. These controls
look like CK editors but can omit records, bypass the current document, and
cannot be trusted as active-plugin transactions.

Route these dialogs through `Data`, `AddRecordCommand`,
`EditRecordCommand`, and the document save path. Replace heuristic setting
discovery with game/version-aware catalogs and implement actual WTHR/LIGH/
SOUN/WATR editing where advertised. Synthetic record tests must prove active
document save, reload, and undo for each workflow. **Status 2026-09-25 — core
expanded.** Weather/light, water, and AI Package editors now save the active
Document instead of writing standalone ESM streams; AI Package creation uses
allocated FormIDs and `AddRecordCommand`. Dialogue save/add/remove paths use
typed `EditRecordCommand`/`AddRecordCommand` plus parent-index commands. Full
WTHR/LIGH/SOUN/WATR editing and synthetic active-document round trips remain.

### Series 9 — NIF animation write-back

**Priority: P1 asset authoring.** `AnimationEditor` loads an in-memory NIF
animation model but discards the source parser/path and exposes only JSON/XML
import/export. `NifAnimationWriter` exists but is not connected to the editor,
rejects keyframe-count changes, and replaces the original non-atomically.
Existing animation tests do not cover NIF persistence.

Retain the parser and source path, convert edits back to controller/data blocks,
support add/remove/replace operations, preserve unknown blocks, and use
`QSaveFile` for replacement. A synthetic NIF must load, edit, save, reload, and
prove both changed keyframes and preservation of unrelated blocks. **Status
2026-09-25 — internal dialect done, real Bethesda blocks done, Skyrim 1.5
keyframe encoding still open.** `AnimationEditor` keeps the parsed source NIF
and its path, and a `Save NIF` action writes edited channels back (Euler edits
are converted to quaternions). `NifBlockFile` reads and writes the real
"Gamebryo File Format, Version 20.2.0.7" container while keeping the header,
string table and every block payload verbatim, so a save only rewrites the
blocks it deliberately patches and unknown blocks cannot drift. The layout is
verified against shipped files: `test_nifblockfile` re-serializes a sample of
Skyrim's NIFs byte for byte (checked against all 22,044 NIFs in
`Skyrim - Meshes0/1.bsa` while the format was derived). Two container bugs
were found and fixed in the process: the header's post-author `u32` only
exists in the Starfield-era header, and the per-block footer is a separate
trailing table rather than a per-block suffix. `NifAnimationWriter` now resolves
a controller's keyframe data through the node's own controller reference (the
controller field layout changed between game generations) and follows the
1.5 `controller -> interpolator -> data` and 1.6+ `controller -> data` chains.
The writer refuses to touch any keyframe block it cannot reproduce byte for
byte, so an unconfirmed layout is declined instead of corrupting the file.
Remaining: the Skyrim 1.5 / Oblivion `NiTransformData` encoding is still
unconfirmed (the 1.6+ `NiKeyframeData` flat 44-byte-per-key layout is
confirmed), so those blocks are read-only for now; the confirmed evidence is
that a 1.5 block is a count followed by three channels whose keys are stored
with the first key carrying no time, but the channel ordering and grouping
observed in shipped files do not yet match a single consistent fit. Also
outstanding: keyed/idle event round-trips.

**Open evidence gaps (measured, not assumed).** Three things are known to be
unverified and are worth recording precisely rather than as a single "needs
testing" line:

- `NiKeyframeData` (1.6+) is confirmed only against its own encoder, not
  against a shipped animated NIF. It does not appear in any Starfield NIF, and
  Starfield ships no HKX, so there is currently no sample of it on this
  machine; see the Starfield notes below.
- 218 of the 260 loose Starfield NIFs are rejected. They are third-party
  "Blender Mesh Plugin" exports, not Bethesda or Creation Kit output: the
  header parses correctly through the group table, but the exporter emits a
  section this build does not model, leaving the payload start 125 bytes
  early. Rejecting them with a precise error is intentional; supporting a
  non-conformant exporter is a separate decision.
- The 1.5 `NiTransformData` fit needs more *samples of that same generation*,
  not other games: Skyrim SE 1.5 is installed and supplied 4,533 such blocks,
  and Oblivion would add more of the same format. What is missing is a
  consistent model, not data.

**Starfield BA2 — done, and it changed the animation target.** `BsaArchive`
now reads the Starfield `BTDX` container. Layout (verified against the shipped
archives): a 32-byte header (magic, version, `GNRL` tag, file count u32, name
table offset u64, trailing u64; 36 bytes with an extra u32 on the v3 variant),
then `fileCount` fixed 36-byte declarations (file hash u32, 4-byte extension,
directory hash u32, flags, offset u64, packed u32, unpacked u32, the
`0xBAADF00D` marker), then the data, then a name table of `u16` length + path
per file. Version 2 is zlib and version 3 a raw LZ4 block; Starfield writes
paths with forward slashes. `test_bsaarchive` covers opening the archive and
extracting NIFs back.

**The BA2 version field is not a monotonic series.** The Starfield container
is "version 2" while Skyrim SE is `0x69` (105) and Skyrim LE is `0x68`, so
`version >= 2` is *not* a valid way to detect the new layout — it rejects
every Skyrim archive. `BTDX` is the only reliable discriminator. This was
mistaken during this work and broke `test_bsawrite`, `test_xwmadecoder`,
`test_assetresolver`, `test_archivebrowser` and `test_nifblockfile` until the
existing suite caught it.

**Starfield keeps no HKX, and animation is not in the NIF either.** With BTDX
reading in place, every one of the 76 readable Starfield archives was tallied
by file extension: 1,472,873 entries in total, comprising `.mesh` (685,586),
`.wem` audio (328,603), `.ffxanim` (279,323) and `.nif` (110,124). There is
not a single `.hkx`. An earlier note in this section named HKX as the Starfield
animation target; that was wrong — it was an assumption, not an observation,
and the archive census disproves it.

A block-type census of 4,000 shipped Starfield mesh and face NIFs backs this
up: the only blocks present are geometry and skinning ones (`BSGeometry`,
`BSSkin::Instance`/`BoneData`, `SkinAttach`, `BSFaceGenNiNode`,
`BSWeakReferenceNode`, `NiNode`, shader/extra data). No `NiKeyframeController`,
no `NiKeyframeData`. The NIF container round-trips these files byte for byte,
so this is not a parsing failure.

Where Starfield animation actually lives is therefore still open, and the
candidates are Starfield's own `.mesh` binary format (whose `BsMeshData`
fragment OpenCK already decodes for geometry) and `.ffxanim` FaceFX animation,
which `REMAINING.md` §8 already places outside the current scope. Both are
substantially larger than the NIF work. Until one of them is parsed, the NIF
keyframe write-back in `NifAnimationWriter` cannot serve Starfield.

**Starfield `DX10` texture archives: the container is now mapped, the payload
is not.** BTDX reading covers `GNRL` only, so the 30 texture archives
(`Starfield - Textures01..11`, `LODTextures01/02`, `GeneratedTextures`, and the
per-quest `*- Textures.ba2`) still fail to open. The `DX10` header turned out to
be the *same* 36 bytes as `GNRL` (magic, version, type, file count, name table
offset, a u64 of 1, and a u32 compression method); only the declaration area
differs. Measured against `Starfield - Textures11.ba2` (1,327 files, LZ4):

- The declaration area is a flat run of **sentinel-terminated records of exactly
  two sizes**, 48 and 24, every one ending in `0xBAADF00D`. Walking them yields
  exactly 1,327 records of 48 bytes — precisely the file count — plus 2,786
  records of 24 bytes, totalling 130,560 bytes with no slack. A 48-byte record
  starts a file; the 24-byte records after it belong to that file.
- The 48-byte file record: `hash u32`, extension `char[4]` (`"dds\0"`, so it is
  NUL-terminated unlike `GNRL`), `dirHash u32`, then `u8 0`, `u8 3`, and
  `u16 24` — that 24 being the size of the DDS header that is *not* stored. Then
  `+24` is the **absolute file offset** of the file's first chunk, `+28` is 0,
  `+38` is **bits per texel** (16 on `_normal.dds`, 8 on the rest — BC7-family
  and BC5-family), and `+36` is the **base mip size in bytes** (1,048,576 for a
  1024x1024 16bpp texture, 524,288 for 8bpp, i.e. the 1-byte-per-texel BCn
  rule). Cross-checked: each file's `+24` equals the previous file's last chunk
  `off + f8`, so the chunks tile the data region contiguously and in order.
- The 24-byte chunk record: `offset u32`, `0 u32`, `packed size u32`,
  `uncompressed size u32`, `u16 first mip`, `u16 last mip`, sentinel. `packed
  size` is confirmed because `off + f8` lands exactly on the next chunk's `off`.
  Every texture has two chunks, mip 1 and mips 2..10.

So the parts still unknown are narrow and nameable: **mip 0 is in no chunk**, the
`+32 u32` field is neither an offset nor a size in the data region (it varies
2,933 to 831,281 against chunk totals of ~1.7 KB to ~105 KB), and the `+20` field
(packing as `u8 0x0B`/`0x8B`, `u8 0x4B`/`0x43`, `u16 0x0800`; the first byte
matches the 11-mip count for a 1024x1024 texture) has not been mapped to a DDS
format code. Until mip 0's location and the format code are pinned down, a
reader cannot emit a valid `.dds`, which is the point of the whole exercise. The
obvious next step is to locate mip 0 and decode `+32`; the structural work above
is already done and does not need redoing.

**Starfield mesh archives open with many weak-reference stub NIFs** (a single
`BSWeakReferenceNode` pointing at real geometry), so anything sampling these
archives must skip stubs or it will conclude the wrong thing — as an earlier
sample of this work did.

**`NiTransformData` (Skyrim 1.5 / Oblivion) remains the only NIF-resident
keyframe format reachable here**, and it is still refused for writing until its
encoding is confirmed. Hand-decoding samples was not converging, so this is now
approached as a fit rather than a guess: `test_ntdlayout` pulls every reachable
`NiTransformData` block and tries all 24 candidate layouts (six channel
orderings x first-key-carries-time x counts-up-front), keeping only a layout
that consumes *every* block exactly. A layout that fits some blocks but not all
is reported and deliberately left unproven. Run it when a Skyrim archive is
resident. Oblivion would add more samples of the same generation; it is not
installed on this machine.

**The game folders are on-demand installs, and that makes the archive tests
flaky.** Individual `.ba2` files flip between resident and evicted between runs
and even between processes launched back to back — `QFile::exists` and a direct
`QFile::open` return "cannot find the file" for a path that PowerShell opens
fine a second later. `test_bsaarchive` now warms each archive (a small read
forces the rehydration) and retries before failing, which is what keeps the
suite green while heavy scanning is in progress. Tests that need these files
should use the same helper rather than assuming the file is present, and must
skip rather than fail when the game is absent.

### Beyond §9 — local CK/tool compatibility gaps

These are additional evidence-backed series discovered from the local install,
separate from the nine required parity series:

- **CK configuration:** add read-only import/inspection for
  `CreationKit.ini`, `CreationKitPrefs.ini`, `CreationKitCustom.ini`,
  `EditorColors.xml`, and CK archive lists instead of relying only on
  OpenCK-local Qt settings.
- **POFX compatibility:** shipped POFX files use nested typed particle data,
  while `ParticleBundle` expects simplified top-level node/emitter fields.
- **Landscape brushes:** shipped LBR files are single JSON brush objects;
  `BrushDefinition` expects a `brushes` array with simplified fields.
- **Object-window filters:** shipped filters use wildcard/exact rules and
  per-rule concatenation flags; `ObjectWindowFilter` currently maps unknown
  types to `Contains` and does not implement wildcard matching.
- **Material rules:** shipped `RuleTemplates` use nested `TemplateRules`, and
  applied texture options/regex rules differ from OpenCK's simplified schemas.
- **AssetWatcher integration:** the CK ships BSFBX, BSTextureConverter, xg_xs,
  xtexconv, and Starwatcher components; OpenCK has no equivalent plugin bridge
  or settings integration.
- **Papyrus toolchain:** the CK ships `PapyrusCompiler.exe`, assembler,
  `PCompiler.dll`, and a project schema; OpenCK's compiler detection should
  integrate the installed Starfield toolchain rather than only `pp64.exe`.
- **Mod deployment:** the CK/Xbox install exposes Vortex deployment manifests
  and deployed asset trees; OpenCK detects Vortex/MO2 but does not consume
  deployment manifests, ownership, or hardlink state.
- **Wwise/FaceFX authoring:** OpenCK has wrappers and partial FaceFX framing,
  but not Wwise project authoring or FaceFX graph evaluation/playback. The
  latter remains the separate §8 runtime boundary.

### Delivery order and verification boundary

1. Series 1–3 first: they can silently corrupt or lose user edits.
2. Series 4–6 next: they are prerequisites for normal plugin and asset
   authoring workflows.
3. Series 7–9 after the core transactions are reliable.
4. The local executable/tool observations above are evidence for fixtures and
   integration design, not permission to copy CK binaries, assets, or source.
5. Runtime acceptance still requires manual/game-tool validation; the headless
   gates cover format, model, editor-session, and save/undo behavior only.
