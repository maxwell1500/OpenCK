# REMAINING.md — Everything left to make OpenCK truly functional

> **Single source of truth.** All previous planning/status/debt/roadmap/phase
> documents were reconciled against the codebase and deleted (2026-08-16).
> Anything not listed here is **done** or **intentionally closed** (see §6).
> Don't re-add work that's already shipped.
>
 > Baseline on this machine: Release build clean, 109/109 test executables
 > green (incl. the real-data gates that load Starfield.esm + Magnus.esm +
 > Vvardenfell.esp), GUI + CLI smoke tests pass.

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
   Warning floor is now ~302: 180× "top-level GRUP before declared TES4 end"
   (one root cause — the TES4 header declares more than the file provides;
   Header::load already recovers, one fix silences all 180), 40× compressed
   records whose loaders still exit early inside the decompressed buffer
   (`restoreStreamFromCompression`), ~30 unknown/garbage record names that
   look like downstream artifacts of those two, plus a handful of `_CPN`
    stragglers. Fixed 2026-09-07: `Variant::load(Format_GMST)` now drains
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
    Remaining: full Starfield.esm-scale round-trip (3.8M records) is a CI/
    nightly job, not a unit test. The ~36 `Variant::load` GMST/GLOB LOG_ERRORs
    (errors, not warnings — outside both gates) remain.

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

    **Status 2026-09-08:** The "fail loudly" half is complete with enhanced
    diagnostics. `scanStaleRawReferences` collects ALL stale references and
    the refusal message lists every offending record/subrecord/byte-offset
    with the stale and expected FormIDs. `refusalMessage()` exposes the full
    diagnostic to callers. `XPRM` is correctly excluded. A "Convert to ESL
    (Light Master)..." menu action (File menu) now wires the compactor into
    the UI: prompts the user, shows the diagnostic on refusal, and offers
    Save As .esl on success. Remaining: document and add rewrites for the
    still-unknown Starfield subrecord layouts so real plugins can be
    compacted instead of refused.

4. **DIAL/INFO relationship walking.** INFO records nested under DIAL are
    parsed but not walked into a DIAL→INFO tree for the dialogue editor.

    **Status 2026-09-08:** `DialRecord::load()` now parses INAM into
    `responseIds` (was falling through to `rawSubRecords`). `save()`
    re-emits INAM from `responseIds` when `hasInam` is set. Round-trip
    verified by `testSyntheticMultiTypeRoundTrip` (DIAL with 2 response
    IDs round-trips identically). Remaining: `DialogueTreeEditor`
    should use `responseIds` (now populated) to show INFO children;
    `DialogueEditorWidget::populateTree()` should use
    `infosUnderDial()`; `addInfo` should update `responseIds` and
    `m_infoParentDial`.

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
 3. **Editor write-back smoke tests.** Automated test per editor: open a fixture
    record, perform a canonical edit, assert the UndoStack gained a command and
    the record changed.

    **Status 2026-09-08:** `test_editor_writeback` added — 7 test cases covering
    StatRecord, GlobalVariable, CellRecord, WorldspaceRecord, NpcRecord,
    PackageRecord, and the no-change case. Each case: add fixture → push
    `EditRecordCommand` → verify record changed → undo → verify reverted →
    redo → verify re-applied. All pass.

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

    **Status 2026-09-08:** `editor.cpp` now checks `OPENCK_DATA_DIR` env var
    (takes priority over config file, before auto-detection). Tests still
    hardcode the path; they should read the env var too.
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
 7. Dead-code sweep: unused stubs, duplicate enums, `Q_UNUSED` params, commented
    blocks, `catch(...)` sites.

    **Status 2026-09-08:** Audited. No commented-out blocks, no TODO/FIXME
    markers, no truly dead functions. All 55 `Q_UNUSED` sites are in Qt
    virtual overrides (parameter required by signature — correct pattern).
    Both `catch(...)` blocks are top-level safety nets in `main.cpp` and
    `crashhandler.cpp` (legitimate). Nothing to remove.
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
- All `test_*.exe` in `build/bin/Release/` exit 0 (currently 109).
  `test_subrecord_diff.exe` is a CLI diff tool, not a test: exit 2 means
  "usage error" when run without its two file arguments, by design —
  exclude it from the glob.
- `openck --cli info <SeydaNeen.esp>` exits 0.
- `docs/record_formats.md` warning rows trend to zero.
- `tools/gen_record_audit.ps1` regenerates `docs/record_formats.md`.
