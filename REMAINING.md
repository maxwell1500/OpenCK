# REMAINING.md — Everything left to make OpenCK truly functional

> **Single source of truth.** All previous planning/status/debt/roadmap/phase
> documents were reconciled against the codebase and deleted (2026-08-16).
> Anything not listed here is **done** or **intentionally closed** (see §6).
> Don't re-add work that's already shipped.
>
> Baseline on this machine: Release build clean, 107/107 test executables
> green (incl. the real-data gates that load Starfield.esm + Magnus.esm +
> SeydaNeen.esp), GUI + CLI smoke tests pass.

---

## 1. Data-integrity gaps (highest priority — these are correctness bugs)

1. **Residual reader warnings → zero.** Loading Starfield.esm still produces
   ~8 "unconsumed bytes" / compression-misalignment warnings (INFO `01047741`
   over-read of 4 bytes; compressed NPC/AI-package records such as
   `Traits_OctopedeA_BlisterCrab_Large`, `EncShip_TradeAuthority_A_Atlas02_AutopilotAI`,
   `EncShip_TradeAuthority_C_Highlander02_AutopilotAI`, `LC017_LvlStarborn_01_Flames`,
   `BE_KT02_Partygoer04`, `BE_KT02_PartyGoer08`). Track them against
   `docs/record_formats.md` and drive every row to zero with the W1 diagnostics.

2. **Untouched-plugin round-trip must be payload-identical.** Build a
   subrecord-diff tool (per-record list of subrecord name+payload) and run it
   for every record type over the full corpus (Starfield.esm, Magnus.esm,
   SeydaNeen.esp). Load → save untouched → diff; drive every differing
   subrecord to zero. Today only SeydaNeen is covered.

3. **FormIdCompactor leaves opaque FormID references stale.** `XPRM` and other
   opaque Starfield raw payloads are not rewritten on compaction
   (`formidcompactor.cpp::rewriteRawSubRecords` only handles the known set).
   Either decode-and-rewrite or fail loudly when such a payload would be
   compacted.

4. **DIAL/INFO relationship walking.** INFO records nested under DIAL are
   parsed but not walked into a DIAL→INFO tree for the dialogue editor.

5. **Master-record state machine on save.** Verify that a materialized
   (deferred) master record saved without edits is not emitted as an override,
   and that an edit promotes base → modified correctly (State_Base /
   State_Modified / State_ModifiedOnly) across the corpus save path.

6. **ObjectPalette save/load asymmetry.** "Save Placement" never writes a file
   (it only appends in-memory) while "Load Placement" reads a binary file.
   Make save write the same format Load reads, and rename the extension away
   from `.json` since it is binary (`QDataStream`, little-endian).

7. **Field range validation on editors.** Editors still accept out-of-range
   values that can corrupt ESM files (the long-running X-02 item). `ColumnValidator`
   exists — deploy it to the remaining editors and enforce ranges on every
   numeric field.

---

## 2. UI write-back wiring (every edit must be undoable)

1. **Editor write-back audit.** Build a table of every editor/dialog in
   `src/view/window/`: does it read from `Data` and commit through
   `EditRecordCommand`/UndoStack? Fix the stragglers.
2. **Weather/light and water editors.** GMST add/edit/delete in
   `weatherlighteditor.cpp` / `watereditor.cpp` still discard edits in places;
   route them through the UndoStack like the Object Window's Game Setting add
   (done).
3. **Editor write-back smoke tests.** Automated test per editor: open a fixture
   record, perform a canonical edit, assert the UndoStack gained a command and
   the record changed.

---

## 3. Editor / feature gaps

1. **Dialogue editor.** Conditional response editing (quest-stage conditions,
   variable checks), voice-file association (`.wav` links), and quest-graph
   stage editing (flags/indices/objectives) are not implemented.
2. **Animation timeline.** `NifKeyframeData`/`NiTransformData` block parsing,
   a timeline widget, keyframe undo commands, NIF write-back, and in-viewport
   preview (Phase5 scope) are not built; the timeline editor for SCEN is
   pending.
3. **Particle FX.** The NIF particle block parser (`NiParticleSystem`,
   `NiPSys*`, `BSLightingShaderProperty`) is missing; the particle effects
   parser is a stub; there is no viewport particle simulation or particle
   editor.
4. **NavMesh reachability.** NavMesh generation works but uses centroid-based
   cell assignment; a reachability flood-fill pass is a documented residual.
5. **In-viewport object manipulation** (move/rotate/scale placed references in
   the render window) is not built.
6. **Mod-manager integration** (Mod Organizer 2 / Vortex) UI is unwired.
7. **OBScript editor** — long-term, not started.
8. **Starfield-specific feature slots** (long-term, not started):
   spaceship editor, galaxy view, worldspace/planet-generation editors
   (PNDT planets, OPAL placement), reflection probes, crowd-region authoring,
   morph/face-gen editor, RoboVoicer (TTS pipeline), Houdini integration.
9. **Multi-game record dispatch** — game-specific record formats and editors
   for Morrowind / Oblivion / Skyrim / FO4 / Starfield behind one dispatch.
   Morrowind save-format conversions are partly handled; a full per-game
   layout pass is the largest remaining effort.

---

## 4. Test infrastructure

1. **`OPENCK_DATA_DIR`** env var honored by all real-data tests (currently
   hardcoded to `C:/XboxGames/Starfield/Content/Data`); skip cleanly when
   absent.
2. **Materialization matrix test** — index count vs. loaded count vs. warning
   count for every type from a full master load; assert warnings == 0 or an
   explicitly shrinking allowlist.
3. **Per-type round-trip subrecord-diff tests** (the tool from §1.2) as part of
   the suite.
4. **Fake-data lint** — CI grep that fails on hardcoded game-content strings in
   `src/` so sample data comes from fixtures.
5. **API doc comments** (Doxygen) on the main public interfaces (`Data`,
   `NifPyFileWrapper`, `BlenderLauncher`, `ShortcutManager`).
6. **Final build gate** — zero-warning clean build, all tests, memory-leak
   check, coverage target.
7. **CTest registration** for the 3 remaining non-QTest binaries
   (`dumpesm`, `scanbtd`, `meshprobe`).

---

## 5. Code-quality debt (verified still open)

1. `Data::createNewRecord` brute-forces FormID allocation (re-scans
   `allCollections()` per candidate); cache the used-FormID set.
2. Editor `saveRecord()` paths that validate **after** mutating the record
   (partial mutation on validation failure) — validate first, commit via a
   temp copy.
3. `LandscapeEditCommand` rewrites the full heightmap per brush stroke and
   stores unused params — implement partial updates or drop the params.
4. `Logger` singleton is not thread-safe before init and never restores the
   Qt message handler — guard + restore.
5. CMake: no install targets, Qt6 path hardcoded, a couple of missing
   component deps for Qt5.
6. 7 remaining `const_cast` call sites in `src/` — add proper mutable getters.
7. Dead-code sweep: unused stubs, duplicate enums, `Q_UNUSED` params, commented
   blocks, `catch(...)` sites.
8. Stale `.bak` files and `external/vorbis` build outputs clutter the tree —
   add a cleanup rule + gitignore.

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

---

## 7. How to verify progress

- `cmake --build build --config Release` clean.
- All `test_*.exe` in `build/bin/Release/` exit 0 (currently 107).
- `openck --cli info <SeydaNeen.esp>` exits 0.
- `docs/record_formats.md` warning rows trend to zero.
- `tools/gen_record_audit.ps1` regenerates `docs/record_formats.md`.
