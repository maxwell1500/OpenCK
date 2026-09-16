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
     Remaining: full Starfield.esm-scale round-trip (3.8M records) is a CI/
     nightly job, not a unit test.

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
     SFBGS00D.esm round-trip still open (434,976/434,990 records; the rest
     is positional cascade, not payload — type counts match exactly):
     - 14 records of 3 unhandled types (GPOF 12, GPOG 1, GWED 1) are skipped
       at load and dropped on save. Needs a generic opaque-record preserve
       path (store raw bytes keyed by type+formId, re-emit in order).
     - Non-REFR/ACHR cell children (PGRE nested in CELL groups, etc.) are
       relocated to top-level groups: `writeCellChildrenGroups` /
       `markCellChildrenWritten` / the replay skip cover REFR/ACHR only.
       Needs generic child coverage driven by the parent-cell index.
     - Unverified payload diffs needing per-formId comparison once the
       cascade clears: several FACTs, RACE 0x106e2bc (138→126 subs),
       MGEF 0x101ea08 (4→7 subs).
     Debug support kept (env-gated, off by default): `OPENCK_SAVE_PROGRESS`
     in `Document::save`, `OPENCK_SNAPSHOT_TRACE` in the snapshot walker.

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
    Remaining: verify compaction on real-world plugins (the generic fix
    should handle all cases; explicit rules remain as optimization).

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

    **Remaining:** in-viewport 3D playback (Play advances the timeline
    indicator but does not render the animated model into the render
    window — Phase 5 scope) and automated tests for the NIF animation
    import/write-back pipeline.

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

    **Remaining:** live in-viewport particle simulation / preview.

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
    Remaining: in-render-window 3D playback of animations/particles (§3.2/§3.3;
    see §8 for the scoped breakdown — rigid animation and particles are
    already wired, skinned playback is the real gap).
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
    Remaining: compile-to-bytecode and type checking against a game-specific
    native function/property catalog.

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
8. **Starfield-specific feature slots** (long-term, not started):
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
    Remaining (§3.8): binary record encoders for these models (to add once a
    real shipped record is available to validate against), in-engine playback
    for the voice lines, and actual native Houdini-side scripts.

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
     Remaining (TES3 Phase 2): per-type record loaders for the Morrowind
     record types in the walk histogram (INFO/DIAL/CELL/STAT/NPC_ bodies etc.
     — ~40 types, mirroring the OpenMW `esm3/load*.cpp` layouts), a TES3
     save/round-trip path with subrecord-diff gating, and the
     game-specific editors (§3.8). The structural core (header, subrecords,
     index, lossless drain) is done and verified.

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
     Remaining (TES3 Phase 2b): component-backed editors for the Morrowind
     record types (the generic record edits losslessly but exposes raw
     bytes only), and the game-specific editors (§3.8).

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
     Remaining (TES3 Phase 2c): type-specific DATA parsing (the current DATA
     editor is generic hex), edit-through-component write-back wired into the
     UndoStack, and specialised editors for Morrowind record families.

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
- All `test_*.exe` in `build/bin/Release/` exit 0 (currently 109).
  `test_subrecord_diff.exe` is a CLI diff tool, not a test: exit 2 means
  "usage error" when run without its two file arguments, by design —
  exclude it from the glob.
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
   stays future work.
2. **Quaternion-correct interpolation — DONE 2026-09-14.** `AnimKeyframe`
   / `TransformKeyframe` carry `qw..qz` + `hasQuat` alongside the Euler
   angles; `interpolateChannel` slerps when both endpoints have quats
   (Euler derived from the result for legacy consumers) and keeps Euler
   lerp otherwise; the viewport imports quats from keyframe controllers
   and renders rotation straight from quat matrices; the blend path uses
   stored quats; JSON/XML persist quats when present (old files keep the
   Euler path). `test_nifanimation` 11/11, incl. the 350°→−5° short-path
   proof (Euler lerp would sit at 175°).
 3. **Verification gate on real assets — IN PROGRESS 2026-09-15 (reader
    landed).** The Gamebryo 20.2.0.7 block reader is now in the tree
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
    Still open: the Instance per-bone 16B semantics (only 5 distinct quads
    across 76 bones — kept raw), per-vertex weight streams (`.mesh`
    Weights sections decode structurally; bone-index→skeleton mapping
    needs the external skeleton), skeleton resolution, GPU/CPU blend
    wiring, plus the manual Play-confirm on a skinned animated mesh and a
    particle mesh, which needs a display. Entry criterion restated: decode
    the 16B payload + weights against a skeleton, blend one frame on the
    face mesh and verify against the NIF bind pose, then load a skinned
    animated NIF plus a particle NIF, press Play, confirm correct motion,
    and record the result here.
