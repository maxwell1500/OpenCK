# OpenCK Multi-Phase Recovery Plan

Derived from the comprehensive gap analysis in NewFindings.md. Each phase
lists every finding it addresses, all substeps, estimated effort, and
dependencies.

---

## Phase 0 — Establish Ground Truth

Before any fixes: determine what the real subrecord codes should be.

| Step | Description | Effort |
|---|---|---|
| 0.1 | Dump all subrecords from a known-good Skyrim SE `.esm` using a hex editor or existing reader. Create a canonical reference table: Record 4CC → subrecord NAME (hex) → subrecord NAME (string) → meaning. | 4 h |
| 0.2 | Same for Fallout 4 `.esm` and Starfield `.esm` (if available). Identify game-specific differences. | 4 h |
| 0.3 | Cross-reference against `tes4codes.hpp` to see which translations are correct and which are invented. | 2 h |
| 0.4 | Write a test (`test_subrecord_roundtrip`) that loads a real plugin, iterates every record and subrecord, writes to a new buffer, and compares byte-for-byte with the original. This test will catch ALL subrecord-name regressions. | 4 h |
| | **Total Phase 0** | **14 h** |

**Deliverable**: Canonical subrecord reference table. Failing round-trip test
that captures the current broken state.

**Dependencies**: None.

---

## Phase 1 — Showstopper Bug Fixes

### 1.1 Fix Subrecord Names Across All Records

Addresses finding: **#1 (C-01)**

| Step | Description | Effort |
|---|---|---|
| 1.1.1 | For each of the 34 dispatch-table record types, audit `load()` and rewrite every subrecord `case` label and `NAME` literal to match the canonical table from Phase 0. Start with the 9 component-migrated records (WEAP, ARMO, ALCH, BOOK, MISC, ACTI, CONT, STAT, NPC\_) since they have the most complex subrecord sets. | 10 h |
| 1.1.2 | Fix the 25 non-component records (GMST, GLOB, SPEL, MGEF, DIAL, INFO, PACK, TREE, INGR, ENCH, RACE, CLAS, FACT, PERK, CELL, WRLD, LCTN, REFR, MATL, LAND, SOUN, WTHR, LTEX, LCRT, NIF). | 15 h |
| 1.1.3 | Update `tes4codes.hpp` translation table to match. Remove any invented entries. | 1 h |
| 1.1.4 | Verify `test_subrecord_roundtrip` (from Phase 0) now passes — saved output matches original byte-for-byte. | 1 h |
| 1.1.5 | Fix `dataimporter.cpp`/`dataexporter.cpp` if they reference wrong subrecord names. | 2 h |
| | **Total 1.1** | **29 h** |

### 1.2 Fix RecordEditCommand No-Op

Addresses finding: **#2 (C-02)**

| Step | Description | Effort |
|---|---|---|
| 1.2.1 | In `RecordEditCommand::execute()`, add the actual value-application call that was missing (apply `newValue` via `EditorProperty::setValue` or equivalent). | 0.5 h |
| 1.2.2 | Write a unit test: create a record, edit a field via RecordEditCommand, verify the field changed. | 0.5 h |
| | **Total 1.2** | **1 h** |

### 1.3 Fix Core Save() Architecture

Addresses finding: **#3** (component save direction, const correctness,
externallySerialized fragility)

| Step | Description | Effort |
|---|---|---|
| 1.3.1 | Define the canonical pattern: `save()` reads component fields into local variables at point-of-write (our current approach). Document in AGENTS.md. | 2 h |
| 1.3.2 | Audit all 8 component-migrated records (WEAP, ARMO, ALCH, BOOK, MISC, ACTI, CONT, STAT) to ensure they follow the pattern: no flat→component mirror, no component→flat mirror, read from component at write time, mark primitives externallySerialized. | 4 h |
| 1.3.3 | Verify `externallySerialized` flag survives clone/blank/copy. Add a `clone()` test that loads a record, clones, saves both, and compares. | 3 h |
| 1.3.4 | Remove the `const_cast` on `components` in save() methods where possible — make `findByName` work on const if the caller only reads. | 2 h |
| 1.3.5 | Create `test_component_save_roundtrip`: load real ESM, save to buffer, reload, verify all fields match. | 2 h |
| | **Total 1.3** | **13 h** |

| | **Phase 1 Total** | **43 h** |

---

## Phase 2 — Record Completeness (Part 1: Complete Existing Records)

### 2.1 Fill Out Incomplete load()/save() Methods

Addresses finding: **#4** (15 records with incomplete parsers, table from
NewFindings.md)

| Step | Record | What to Add | Effort |
|---|---|---|---|
| 2.1.1 | `RACE` | FULL name, DATA (skills, height/weight), head parts (HDPT), bone data (BODT/BOD2), voice types (VTCK/VNAM), body data (NAM1/NAM2/NAM3), movement types (MNAM/DNAM/RNAM), attribute modifiers, perk list. Add TESFullName_Component, TESModel_Component. | 6 h |
| 2.1.2 | `DIAL` | FULL (topic name), DATA (dialogue type), QNAM (quest). Add TESFullName_Component, BGSKeywordForm_Component. | 2 h |
| 2.1.3 | `INFO` | QNAM (speaker), TRDT (response data: emotion, sound, camera), NAM1 (actor), response text format, script fragments. | 4 h |
| 2.1.4 | `NPC_` | Body parts/races/skin (RNAM), attributes (strength/int/wisdom/agility/etc), skills array (DNAM), factions (SNAM), AI packages (PKDT/PLDT/PTDT), formation (CNAM), perk list. Add TESFullName_Component (already seeded?), TESModel_Component. | 8 h |
| 2.1.5 | `PACK` | PKD2/PLD2 (Skyrim/FO4 specific fields), PNAM (package template), conditions (CTDA), script fragments, type-specific data blocks. | 4 h |
| 2.1.6 | `QUST` | Stages (QSTA/QSDT), objectives (QOBJ + QOBT), aliases (Alias block), rewards, script fragments. Add BGSKeywordForm_Component. | 6 h |
| 2.1.7 | `PERK` | Rank data (PRKR — conditions, entry points, functionality per rank), multiple effect subrecords. | 3 h |
| 2.1.8 | `FACT` | DATA (flags), JAIL/CRGR/CRVA conditions, multiple relation types (XNAM), rank data (RNAM expanded). | 3 h |
| 2.1.9 | `CELL` | XLCN (location), XCLL (lighting template), XNAM (name), XPRM (physics), XOWN (ownership expanded), XCAS/XCAS (acoustic space), XEZN (encounter zone). | 4 h |
| 2.1.10 | `REFR` | XSCL (standalone scale), XRGD (ragdoll data), XLTW (water type), XTRI (lock data), XHTW (havok data). | 3 h |
| 2.1.11 | `SOUN` | CNAM (category), GNAM (type), HNAM (attenuation), BNAM (output model). | 2 h |
| 2.1.12 | `WTHR` | NAM0–NAM9 (cloud layers), DNAM (colors: upper/lower sky, fog, sun, etc.), DATA (timing: sunrise/sunset), INAM (image space modifier), FNAM (fog distance). Add TESFullName_Component. | 5 h |
| 2.1.13 | `SPEL` | Effects array: Each effect needs (effect formid, magnitude, area of effect, duration, cost), conditions per effect. | 3 h |
| 2.1.14 | `MGEF` | Archetype (DATA), counter-effect, visual data (VNAM), sounds (SNAM — already partially), hit effect (NAM1), lighting (NAM7/NAM8). | 3 h |
| 2.1.15 | `ENCH` | Effect list (same structure as SPEL), conditions, charge/type data (ENIT expanded). | 2 h |
| | **Total 2.1** | **58 h** |

### 2.2 Migrate More Records to Component System

Addresses finding: **#4** (only 9 of 34 use components)

| Step | Record | Components to Add | Effort |
|---|---|---|---|
| 2.2.1 | `INGR` | TESFullName, TESModel, TESTexture, TESValue, TESWeight, BGSKeywordForm (mirrors ALCH pattern). | 2 h |
| 2.2.2 | `TREE` | TESFullName, TESModel, TESTexture, BGSKeywordForm. | 2 h |
| 2.2.3 | `NPC_` | TESFullName (if not already), TESModel, TESTexture, BGSKeywordForm. | 2 h |
| 2.2.4 | `SPEL` | TESFullName, TESModel, TESTexture. | 2 h |
| 2.2.5 | `MGEF` | TESFullName, TESModel, TESTexture. | 2 h |
| 2.2.6 | `ENCH` | TESFullName, TESModel, TESTexture. | 2 h |
| 2.2.7 | `CELL` | TESFullName. | 1 h |
| 2.2.8 | `WRLD` | TESFullName, TESModel (if icon path available). | 2 h |
| | **Total 2.2** | **15 h** |

| | **Phase 2 Total** | **73 h** |

---

## Phase 3 — Pervasive Quality Issues

### 3.1 Fix 200+ const_cast (H-01)

| Step | Description | Effort |
|---|---|---|
| 3.1.1 | Audit every `const_cast` in `src/view/` (view layer). Determine which are UB-risk (casting away const from an originally-const object) vs. harmless (casting away const from an object known to be non-const). | 2 h |
| 3.1.2 | Fix each UB-risk cast: either change the function signature to be non-const correctly, or use `mutable` on the member, or restructure to avoid the cast. | 3 h |
| 3.1.3 | Clean up remaining harmless casts with comments explaining why they are safe. | 1 h |
| | **Total 3.1** | **6 h** |

### 3.2 Fix Float Position Truncation (H-03)

| Step | Description | Effort |
|---|---|---|
| 3.2.1 | In `ref_editor.cpp` (and any other file using `QSpinBox` for float fields), replace `QSpinBox` with `QDoubleSpinBox`. Set appropriate `decimals()`, `singleStep()`, and `range()` for position/rotation/scale. | 1 h |
| 3.2.2 | Verify existing palette/undo/redo still works with float values. | 0.5 h |
| | **Total 3.2** | **1.5 h** |

### 3.3 Fix Object Palette Byte-Order (H-04)

| Step | Description | Effort |
|---|---|---|
| 3.3.1 | In `objectpalette.cpp`, change the write path to use `QDataStream::LittleEndian` (match the read path). Or vice versa — pick one and be consistent. | 0.5 h |
| 3.3.2 | Add a version byte to the palette format so future format changes can be detected. | 0.5 h |
| | **Total 3.3** | **1 h** |

### 3.4 Fix getColumnId() (H-05)

| Step | Description | Effort |
|---|---|---|
| 3.4.1 | In the column class where `getColumnId()` compares `name_` against itself, change to compare against `columns_[i]->name_`. | 0.25 h |
| 3.4.2 | Verify search/filter works on all column types. | 0.25 h |
| | **Total 3.4** | **0.5 h** |

### 3.5 Fix Feature Flag Overlap (H-06)

| Step | Description | Effort |
|---|---|---|
| 3.5.1 | Re-number `Feature_ViewId` from `3` to `4` (next power of two: `1 << 2`). Verify no other flag overlaps. | 0.25 h |
| 3.5.2 | Check all `Feature_*` values are powers of two. | 0.25 h |
| | **Total 3.5** | **0.5 h** |

### 3.6 Fix UI Hang on Compiler Hang (H-07)

| Step | Description | Effort |
|---|---|---|
| 3.6.1 | Replace `process.waitForFinished(-1)` with a timeout (e.g. 30 seconds) and a `QProcess::kill()` fallback. | 1 h |
| 3.6.2 | Add a progress dialog or status-bar message during compilation. | 0.5 h |
| | **Total 3.6** | **1.5 h** |

### 3.7 Fix saveRecord() Validation Order (M-04)

| Step | Description | Effort |
|---|---|---|
| 3.7.1 | In every editor's `saveRecord()` (28+ record editors), swap the order: validate the proposed values first, then mutate the record. | 2 h |
| 3.7.2 | Extract validation into a separate method that takes proposed values and returns error/warning strings. | 1 h |
| | **Total 3.7** | **3 h** |

### 3.8 Fix Logger Data Race (M-10)

| Step | Description | Effort |
|---|---|---|
| 3.8.1 | Wrap `m_initialized` flag with `std::atomic<bool>` or add a `QMutex` guard. | 0.5 h |
| 3.8.2 | Give the logger a trivial-constructible initial state (always safe to call before init). | 0.5 h |
| | **Total 3.8** | **1 h** |

### 3.9 Fix Brute-Force FormID Search (M-07)

| Step | Description | Effort |
|---|---|---|
| 3.9.1 | Add a `QSet<quint32>` or sorted `QVector<quint32>` of allocated FormIDs to `Data`. Maintain on record add/remove. | 2 h |
| 3.9.2 | Replace the O(n*m) search with O(1) lookup in the set. | 1 h |
| | **Total 3.9** | **3 h** |

### 3.10 Fix MastersList Qt Parent (M-05)

| Step | Description | Effort |
|---|---|---|
| 3.10.1 | Pass the `parent` parameter through to `QObject` constructor in `MastersList`. | 0.25 h |
| 3.10.2 | Also ensure `MastersListDialog` properly parents the list widget. | 0.25 h |
| | **Total 3.10** | **0.5 h** |

### 3.11 Fix topologicalSort Empty Return (M-06)

| Step | Description | Effort |
|---|---|---|
| 3.11.1 | Change return type to include error info: return a struct with `{vector, hasCycle, cycleEdges}` or use `std::optional` / `std::expected`. | 1 h |
| 3.11.2 | Update all callers to handle the error case (show dialog: "Circular dependency detected between..." with details). | 0.5 h |
| | **Total 3.11** | **1.5 h** |

### 3.12 Fix LandscapeEditCommand Unused Params (M-08)

| Step | Description | Effort |
|---|---|---|
| 3.12.1 | Remove unreferenced member variables `centerX`, `centerY`, `radius` from `LandscapeEditCommand`. Update constructor/storage accordingly. | 1 h |
| 3.12.2 | If the params are needed for future undo, document with a comment. | 0.5 h |
| | **Total 3.12** | **1.5 h** |

### 3.13 Fix CMakeLists.txt Issues (M-09)

| Step | Description | Effort |
|---|---|---|
| 3.13.1 | Add missing Qt5/Win32 conditional components. | 0.5 h |
| 3.13.2 | Add `install()` targets for all targets and runtime DLLs. | 1 h |
| 3.13.3 | Replace hardcoded paths with CMake variables. | 0.5 h |
| 3.13.4 | Verify build on a clean machine (no pre-existing paths). | 1 h |
| | **Total 3.13** | **3 h** |

### 3.14 Wire MacroCommand to EditRecordCommand

Addresses finding: **#26**

| Step | Description | Effort |
|---|---|---|
| 3.14.1 | Add a `MacroCommand::begin(const QString& name)` and `end()` API for grouping `EditRecordCommand`s. | 1 h |
| 3.14.2 | Wire this into the UI: if user edits field A then field B quickly (within 500ms), group as one undo step. | 2 h |
| 3.14.3 | Verify undo/redo works correctly for grouped edits. | 1 h |
| | **Total 3.14** | **4 h** |

### 3.15 Remove Dead Code

Addresses finding: **#45**

| Step | Description | Effort |
|---|---|---|
| 3.15.1 | Remove or consolidate `dataimporter2.cpp` and `dataimporter_new.cpp` into the main importer. | 2 h |
| 3.15.2 | Remove `Type_RunLog` from CkId enum if never used. | 0.25 h |
| 3.15.3 | Clean up `fix_records*.py` scripts from repo root (they shouldn't be checked in). | 0.25 h |
| | **Total 3.15** | **2.5 h** |

| | **Phase 3 Total** | **31 h** |

---

## Phase 4 — Add Missing High-Priority Record Types

### 4.1 Script Record (SCPT) — Critical

Addresses finding: **#4** (Papyrus scripts cannot be persisted)
Priority: Critical — without this, scripts cannot be saved in plugins.

| Step | Description | Effort |
|---|---|---|
| 4.1.1 | Define `ScriptRecord` struct: fields for compiled script data (SCHR, SCDA, SCTX), source (SCTX — the Papyrus source text), and referenced objects (SCRO). | 2 h |
| 4.1.2 | Implement `load()`: parse SCHR (header: formID, status, refcount, compsize, infosize), SCDA (compiled data), SCTX (source text), SCRO (referenced objects). | 3 h |
| 4.1.3 | Implement `save()`: write SCHR, SCDA, SCTX, SCRO in correct order. | 2 h |
| 4.1.4 | Add `ScriptRecord` dispatch case in `Data::continueLoading()`. | 0.5 h |
| 4.1.5 | Add column definitions for ObjectWindow. | 1 h |
| 4.1.6 | Create basic editor dialog (script source view/edit + compile button). | 4 h |
| 4.1.7 | Wire Papyrus compiler to save compiled output (SCDA) into the record. | 2 h |
| | **Total 4.1** | **14.5 h** |

### 4.2 Leveled Lists (LVLI, LVLN, LVLC, LVSP)

Addresses finding: **#4**

| Step | Description | Effort |
|---|---|---|
| 4.2.1 | Define `LeveledItemRecord` (LVLI), `LeveledNpcRecord` (LVLN), `LeveledCreatureRecord` (LVLC), `LeveledSpellRecord` (LVSP) structs. Common fields: EDID, LVLD (chance none), LVLF (flags), LVLO (entry: level, formid, count) repeated. | 4 h |
| 4.2.2 | Implement load/save for all four. | 6 h |
| 4.2.3 | Add dispatch cases. | 0.5 h |
| 4.2.4 | Add column definitions and basic editor dialog (list of entries with level/formid/count, drag-drop reorder). | 4 h |
| | **Total 4.2** | **14.5 h** |

### 4.3 Keyword Record (KYWD)

Addresses finding: **#4**

| Step | Description | Effort |
|---|---|---|
| 4.3.1 | Define `KeywordRecord` struct: EDID, CNAM (color), FLAG. Simple record. | 1 h |
| 4.3.2 | Load/save. | 1 h |
| 4.3.3 | Dispatch case + column defs + basic editor. | 2 h |
| | **Total 4.3** | **4 h** |

### 4.4 Ammo (AMMO)

| Step | Description | Effort |
|---|---|---|
| 4.4.1 | Define `AmmoRecord`: EDID, FULL, DATA (damage, value, weight), BNAM (projectile), CNAM (short name), YNAM/ZNAM (pickup/putdown), ITM2/ODIT (icon/model), keywords. Add TESFullName, TESModel, TESTexture, BGSKeywordForm components. | 3 h |
| 4.4.2 | Load/save/dispatch/columns/editor. | 4 h |
| | **Total 4.4** | **7 h** |

### 4.5 Form List (FLST)

| Step | Description | Effort |
|---|---|---|
| 4.5.1 | Define `FormListRecord`: EDID, FLST (LNAM repeated: form IDs). Simple. | 1 h |
| 4.5.2 | Load/save/dispatch/columns/editor. | 3 h |
| | **Total 4.5** | **4 h** |

### 4.6 Light (LIGH)

| Step | Description | Effort |
|---|---|---|
| 4.6.1 | Define `LightRecord`: EDID, FULL, DATA (time, radius, color, FOV, fade), ITM2/ODIT (icon/model), FNAM (fade value), SNAM (sound), flags. Add TESFullName, TESModel, TESTexture, BGSKeywordForm. | 3 h |
| 4.6.2 | Load/save/dispatch/columns/editor. | 4 h |
| | **Total 4.6** | **7 h** |

### 4.7 Door (DOOR)

| Step | Description | Effort |
|---|---|---|
| 4.7.1 | Define `DoorRecord`: EDID, FULL, ITM2/ODIT, SNAM (sound open/close), flags. Add TESFullName, TESModel, BGSKeywordForm. | 2 h |
| 4.7.2 | Load/save/dispatch/columns/editor. | 3 h |
| | **Total 4.7** | **5 h** |

### 4.8 Furniture (FURN)

| Step | Description | Effort |
|---|---|---|
| 4.8.1 | Define `FurnitureRecord`: EDID, FULL, ITM2/ODIT, MNAM (marker entry points), flags. Add TESFullName, TESModel, TESTexture, BGSKeywordForm. | 2 h |
| 4.8.2 | Load/save/dispatch/columns/editor. | 3 h |
| | **Total 4.8** | **5 h** |

### 4.9 Key (KEYM)

| Step | Description | Effort |
|---|---|---|
| 4.9.1 | Define `KeyRecord`: EDID, FULL, DATA (value, weight), ITM2/ODIT. Add TESFullName, TESModel, TESTexture, TESValue, TESWeight. | 2 h |
| 4.9.2 | Load/save/dispatch/columns/editor. | 3 h |
| | **Total 4.9** | **5 h** |

### 4.10 Movable Static (MSTT)

| Step | Description | Effort |
|---|---|---|
| 4.10.1 | Define `MovableStaticRecord`: EDID, ITM2/ODIT, DATA (flags/damage), SNAM (sound). Add TESModel, TESTexture, BGSKeywordForm. | 2 h |
| 4.10.2 | Load/save/dispatch/columns/editor. | 3 h |
| | **Total 4.10** | **5 h** |

### 4.11 Texture Set (TXST)

| Step | Description | Effort |
|---|---|---|
| 4.11.1 | Define `TextureSetRecord`: EDID, TX00–TX07 (diffuse, normal, glow, etc.), flags. | 2 h |
| 4.11.2 | Load/save/dispatch/columns/editor with texture-preview thumbnails. | 4 h |
| | **Total 4.11** | **6 h** |

### 4.12 Image Space (IMGS) / Image Space Modifier (IMAD)

| Step | Description | Effort |
|---|---|---|
| 4.12.1 | Define records: IMGS has EDID, DATA (HDR bloom, saturation, contrast, etc.), DNAM (blur, DOF). IMAD is similar with animatable values. | 3 h |
| 4.12.2 | Load/save/dispatch/columns/editor with color sliders. | 4 h |
| | **Total 4.12** | **7 h** |

### 4.13 Climate (CLMT)

| Step | Description | Effort |
|---|---|---|
| 4.13.1 | Define `ClimateRecord`: EDID, DATA (sunrise/sunset, phase), WLST (weather list with chance), FNAM (sun texture), TNAM (cloud texture), flags. | 2 h |
| 4.13.2 | Load/save/dispatch/columns/editor. | 3 h |
| | **Total 4.13** | **5 h** |

### 4.14 Navmesh Record (NAVM)

Addresses finding: **#7** — prerequisite for navmesh editor.

| Step | Description | Effort |
|---|---|---|
| 4.14.1 | Define `NavmeshRecord`: EDID, NAVM (vertices, triangles, edges, external connections), NVNM — large, complex binary block. Study format from community docs. | 6 h |
| 4.14.2 | Implement load() — parse NVMX/NVPP/NVSI/NVCI subrecords. | 6 h |
| 4.14.3 | Implement save() — full write-back of navmesh geometry. | 4 h |
| 4.14.4 | Dispatch + columns. No editor dialog yet (Phase 7 builds the graphical editor). | 1 h |
| | **Total 4.14** | **17 h** |

### 4.15 Region (RGNS)

| Step | Description | Effort |
|---|---|---|
| 4.15.1 | Define `RegionRecord`: EDID, DATA (region flags), RDAT (data types), RDOT/RDMP/RDGS/RDMD/RDST (various region data subrecords). | 3 h |
| 4.15.2 | Load/save/dispatch/columns/editor. | 4 h |
| | **Total 4.15** | **7 h** |

### 4.16 Scene (SCEN)

Addresses finding: **#29**

| Step | Description | Effort |
|---|---|---|
| 4.16.1 | Define `SceneRecord`: EDID, FULL, DATA (flags), PNAM (actor list), phases, actions, dialogue assignments. Very large record. | 4 h |
| 4.16.2 | Load/save (basic — subrecords that aren't hand-parsed go to raw). | 4 h |
| 4.16.3 | Dispatch + columns. Editor dialog deferred to Phase 7 (quest/dialogue improvements). | 1 h |
| | **Total 4.16** | **9 h** |

### 4.17 Outfit (OTFT)

| Step | Description | Effort |
|---|---|---|
| 4.17.1 | Define `OutfitRecord`: EDID, FULL, INAM (item list — form IDs). Simple. Add TESFullName. | 1 h |
| 4.17.2 | Load/save/dispatch/columns/editor. | 2 h |
| | **Total 4.17** | **3 h** |

### 4.18 Message (MESG)

| Step | Description | Effort |
|---|---|---|
| 4.18.1 | Define `MessageRecord`: EDID, FULL (message text), ITMT (item list for menu buttons), flags. | 1.5 h |
| 4.18.2 | Load/save/dispatch/columns/editor. | 2.5 h |
| | **Total 4.18** | **4 h** |

### 4.19 Note (NOTE) & Scroll (SCRL)

| Step | Description | Effort |
|---|---|---|
| 4.19.1 | Define record structs (both similar to BOOK). NOTE: TEXT subrecord for note body. SCRL (Scroll — a single-use spell item): EDID, FULL, DATA (value, weight), ITM2/ODIT, magic effects. | 3 h |
| 4.19.2 | Load/save/dispatch/columns/editor for both. | 4 h |
| | **Total 4.19** | **7 h** |

### 4.20 Shout (SHOU) & Word of Power (WOOP)

| Step | Description | Effort |
|---|---|---|
| 4.20.1 | Shout: EDID, FULL, SNAM (words of power), flags. Word: EDID, FULL, TNAM (translation), CNAM (word), flags. | 2 h |
| 4.20.2 | Load/save/dispatch/columns/editor. | 3 h |
| | **Total 4.20** | **5 h** |

### 4.21 Impact (IPCT/IPDS), Debris (DEBR), Projectile (PROJ), Hazard (HAZD)

Batch of visual/combat records — simpler structures.

| Step | Description | Effort |
|---|---|---|
| 4.21.1 | Define, load, save, dispatch: IPCT (impact DAT effect, texture set, flags), IPDS (impact data set — list of IPCT refs with angle/pitch), DEBRIS (EDID, DATA (percent, model, texture)), PROJ (EDID, FULL, DATA (speed, gravity, range), flags, model), HAZARD (EDID, FULL, DATA (radius, lifetime, interval), spell). | 8 h |
| 4.21.2 | Column defs + basic editors for each. | 5 h |
| | **Total 4.21** | **13 h** |

### 4.22 Sound Descriptor (SNDR), Sound Category (SNCT), Sound Marker (SMRK)

| Step | Description | Effort |
|---|---|---|
| 4.22.1 | SNDR: EDID, CNAM (category), GNAM (type: combat/ambience/UPS), HNAM (attenuation shape), BNAM (output model), flags. | 2 h |
| 4.22.2 | SNCT: EDID, FNAM (static volume), PNAM (static pitch), flags. Simple. | 1 h |
| 4.22.3 | SMRK (sound marker for placement): EDID, full/icon (for object window), DATA (sound formid, flags). | 1.5 h |
| 4.22.4 | Load/save/dispatch/columns/editor for all three. | 4 h |
| | **Total 4.22** | **8.5 h** |

### 4.23 Music (MUSC), Combat Style (CSTY), Default Object (DOBJ), Load Screen (LSCR)

| Step | Description | Effort |
|---|---|---|
| 4.23.1 | Define, load, save, dispatch for all four. MUSC is a simple track list. CSTY has combat distance/behavior flags. DOBJ maps global form IDs to defaults. LSCR has EDID, DESC (loading text), icon, NIF (loading model/screenshot). | 6 h |
| 4.23.2 | Column defs + basic editors. | 4 h |
| | **Total 4.23** | **10 h** |

### 4.24 Visual Effects (VEFX), Shader Particle (SPGD), Lighting Template (LGTM)

| Step | Description | Effort |
|---|---|---|
| 4.24.1 | VEFX: EDID, DATA (effect shader formid), ICON. SPGD: EDID, DATA (gravity, speed, lifetime, color, scale). LGTM: EDID, DATA (color, focus/shadow offsets). | 3 h |
| 4.24.2 | Load/save/dispatch/columns/editor. | 3 h |
| | **Total 4.24** | **6 h** |

### 4.25 Animated Object (ANIO), Art Object (ART), Idle Animation (IDLE), Eyes/Head/Hair (EYES/HDPT/HAIR)

| Step | Description | Effort |
|---|---|---|
| 4.25.1 | ANIO: EDID, DATA (animation file path), BNAM (unload event). ART: EDID, FULL (model path via TESModel), MNAM (menu/text). IDLE: EDID, DATA (animation group, loop, replay), conditions (CTDA). | 4 h |
| 4.25.2 | EYES: EDID, FULL, ICON, DATA (flags). HDPT: EDID, FULL, DATA (type, flags), CNAM (extra parts), PNAM (facegen modifier). HAIR: EDID, FULL, ICON, DATA (flags), texture refs. | 3 h |
| 4.25.3 | Load/save/dispatch/columns/editor for all six. | 5 h |
| | **Total 4.25** | **12 h** |

### 4.26 Creatures (CREA), Relationship (RELA), Soul Gem (SLGM)

| Step | Description | Effort |
|---|---|---|
| 4.26.1 | CREA (TES4): Actor-like record with stats, spells, inventory, combat style, skeleton, etc. Large record. | 4 h |
| 4.26.2 | RELA: EDID, DATA (parent form, child form, keyword/association type), flags. | 1 h |
| 4.26.3 | SLGM: EDID, FULL, DATA (value, weight, capacity), ITM2/ODIT. Add TESFullName, TESModel, TESTexture, TESValue, TESWeight. | 2 h |
| 4.26.4 | Load/save/dispatch/columns/editor. | 3 h |
| | **Total 4.26** | **10 h** |

### 4.27 Explosion (EXPL)

| Step | Description | Effort |
|---|---|---|
| 4.27.1 | Define `ExplosionRecord`: EDID, FULL, DATA (radius, force, damage), ENIT (light, sound, impact dataset, flags, ISM, type), model/texture/image space modifier. | 2 h |
| 4.27.2 | Load/save/dispatch/columns/editor. | 3 h |
| | **Total 4.27** | **5 h** |

### 4.28 Starfield-Specific Records (Partial)

If targeting Starfield support: PLAN (planet), STAR (star), BIOM (biome),
PKIN (packin), SPCE (space cell), etc. These are placeholders — the bulk of
Starfield-specific work goes here.

| Step | Description | Effort |
|---|---|---|
| 4.28.1 | Define structs for the most common Starfield records: PLAN (planet properties, biome distribution), STAR (star system data), PKIN (pack-in data — static collections as single units). | 6 h |
| 4.28.2 | Load/save/dispatch for these. Editors deferred. | 4 h |
| | **Total 4.28** | **10 h** |

### 4.29 Creature / FO4-Specific (Leveled lists already covered above)

FO4-specific: OMOD (object mod), AVIF (actor value info), PERK is already
implemented. AMMO and PROJ were above.

| Step | Description | Effort |
|---|---|---|
| 4.29.1 | Define `ObjectModRecord` (OMOD) — weapon/armor mod system: EDID, FULL, DATA (modifies weapon/armor properties), properties (virtual machine). | 4 h |
| 4.29.2 | Load/save/dispatch/columns/editor. | 4 h |
| | **Total 4.29** | **8 h** |

| | **Phase 4 Total** | **~210 h** |

---

## Phase 5 — Animation & Particles

### 5.1 NIF Keyframe Parsing

Addresses finding: **#8** (Phase 5.1 from Phase5.md)

| Step | Description | Effort |
|---|---|---|
| 5.1.1 | Add block classes for `NiKeyframeData`, `NiTransformData`, `NiRotData`, `NiPosData`, `NiFloatData`, `NiTextKeyExtraData`, `NiStringPalette`, `NiStringExtraData` in `nifrecord.hpp`. Each holds typed keyframe arrays (position, rotation, scale, text keys). | 4 h |
| 5.1.2 | Replace the `// skip` in `nifparser.cpp` lines 602–606 with real parsing: read block data, build interpolation modes/quaternions/Euler angles. | 4 h |
| 5.1.3 | Wire parsed keyframes into `NifAnimationState` so it can drive the 3D viewport playback. | 2 h |
| | **Total 5.1** | **10 h** |

### 5.2 Timeline Widget

Addresses finding: **#37** (Phase 5.3 from Phase5.md)

| Step | Description | Effort |
|---|---|---|
| 5.2.1 | Implement `Timeline::paintEvent()` — draw time ruler at top, track lanes, keyframe diamonds, playhead cursor, looping region. | 3 h |
| 5.2.2 | Implement mouse interaction: click to seek, drag playhead, double-click timeline to add keyframe, click keyframe to select, drag keyframe to move. | 3 h |
| 5.2.3 | Wire timeline to `AnimationEditor` and `NifAnimationState`: play/pause/stop buttons, frame counter, current-time display. | 2 h |
| | **Total 5.2** | **8 h** |

### 5.3 Animation Commands

Addresses finding: **#8** (Phase 5.4 from Phase5.md)

| Step | Description | Effort |
|---|---|---|
| 5.3.1 | Implement `AddKeyframeCommand`: store + position + rotation + scale, execute inserts into the NIF data, undo removes. | 1.5 h |
| 5.3.2 | Implement `RemoveKeyframeCommand`: inverse of add. | 1 h |
| 5.3.3 | Implement `MoveKeyframeCommand`: change time position of existing keyframe with interpolation re-sampling. | 1.5 h |
| | **Total 5.3** | **4 h** |

### 5.4 Animation Import/Export

Addresses finding: **#8** (Phase 5.5 from Phase5.md)

| Step | Description | Effort |
|---|---|---|
| 5.4.1 | Implement `NifAnimationExporter`: serialize keyframe data to NIF kf binary format (correct header, block order, string palette). | 3 h |
| 5.4.2 | Implement `NifAnimationImporter`: parse kf files and populate block structures. | 3 h |
| 5.4.3 | Add import/export buttons to `AnimationEditor`. | 1 h |
| | **Total 5.4** | **7 h** |

### 5.5 Particle System

Addresses finding: **#9** (Phase6.md steps)

| Step | Description | Effort |
|---|---|---|
| 5.5.1 | Add NIF block types: `NiParticleSystem`, `NiPSysData`, `NiPSysModifier` (and all subclasses: emitter, gravity, target, orbit, etc.), `BSLightingShaderProperty`. | 4 h |
| 5.5.2 | Replace stub `ParticleEffectsParser` with real parser using `NifParser`. Wire parsed data to editor UI fields. | 3 h |
| 5.5.3 | Implement `ParticleSystem` simulation engine and `ParticleRenderer` (OpenGL points/sprites with billboarding, color, size curves). | 4 h |
| 5.5.4 | Wire particle preview into `NifViewportWidget`. | 2 h |
| 5.5.5 | Clean up old stub code and `particleeffects.cpp` dead code. | 1 h |
| | **Total 5.5** | **14 h** |

| | **Phase 5 Total** | **43 h** |

---

## Phase 6 — 3D World (Render Window + Landscape)

### 6.1 Render Window Foundation

Addresses finding: **#5**

| Step | Description | Effort |
|---|---|---|
| 6.1.1 | Create `RenderWindow` class (new `CDockWidget` content) based on `QOpenGLWidget` / `QOpenGLWindow` with proper camera: orbit/pan/zoom, FOV, near/far planes. Integrate with `MainWindow`. | 8 h |
| 6.1.2 | Implement cell/worldspace rendering: load CELL or WRLD records, iterate REFR references, place NIF models at their transforms in the 3D scene. | 16 h |
| 6.1.3 | Implement object selection: ray-cast picking against loaded meshes, highlight selected, show bounding box / wireframe overlay. | 8 h |
| 6.1.4 | Implement gizmo transforms: translate (move), rotate, scale with handle rendering (three-axis arrows, circles, boxes). Arrow-key nudge and grid snapping. | 12 h |
| 6.1.5 | Implement visibility toggles: sky, lights, markers, water, collision, wireframe, fog, cell borders, grass. | 4 h |
| 6.1.6 | Implement object placement: drag from Object Window onto Render Window surface (ray-cast to terrain), drop-to-ground (F key), undo support. | 8 h |
| 6.1.7 | Implement cell border rendering and cell-switching on pan beyond borders. | 4 h |
| | **Total 6.1** | **60 h** |

### 6.2 Landscape Engine Rewrite

Addresses finding: **#6**

| Step | Description | Effort |
|---|---|---|
| 6.2.1 | Fix VBO double-allocation in `LandscapeEditor` terrain renderer (H-02): ensure VBOs are created once, not re-created on every frame. | 3 h |
| 6.2.2 | Implement heightmap brush tools: raise, lower, flatten, smooth, noise. Each is a shader or CPU deformation with undo support via `LandscapeEditCommand`. | 10 h |
| 6.2.3 | Implement vertex painting (texture layers): paint brush applies alpha to texture layers (clay, dirt, grass, rock, snow). Multi-layer blending. | 8 h |
| 6.2.4 | Implement heightmap import/export (.raw 16-bit or 32-bit float format). | 3 h |
| 6.2.5 | Implement landscape LOD generation for distant terrain. | 4 h |
| 6.2.6 | Clean up unused params in `LandscapeEditCommand` (M-08 if not already done in Phase 3). | 1 h |
| | **Total 6.2** | **29 h** |

### 6.3 Sun/Ambient Lighting in Viewport

| Step | Description | Effort |
|---|---|---|
| 6.3.1 | Implement directional sunlight (sun angle from WRLD/CLMT, shadows with shadow mapping). | 6 h |
| 6.3.2 | Implement ambient light slider and time-of-day preview (interpolate weather colors based on sunrise/sunset times). | 4 h |
| 6.3.3 | Implement IMGS/IMAD preview: apply colour grading tonemap as full-screen post-process. | 4 h |
| | **Total 6.3** | **14 h** |

| | **Phase 6 Total** | **103 h** |

---

## Phase 7 — Advanced Editing (Navmesh, Quest/Dialogue, Scene, LOD)

### 7.1 Navmesh Graphical Editor

Addresses finding: **#7**; depends on NAVM record from Phase 4.14.

| Step | Description | Effort |
|---|---|---|
| 7.1.1 | In `NavMeshEditor`, load the NAVM record for the active cell. Render navmesh triangles in the Render Window (semi-transparent coloured overlay: green=connected, red=unconnected, blue=external). | 6 h |
| 7.1.2 | Implement triangle creation: click to place vertices, connect to form triangles. Edge extrusion (select an edge, drag to create neighbor triangle). | 8 h |
| 7.1.3 | Implement vertex/triangle editing: move vertices (snap to geometry button), split/merge triangles, delete selected. | 4 h |
| 7.1.4 | Implement navmesh validation: detect open edges, degenerate triangles, overlapping geometry, disconnected islands. Show errors in Render Window. | 6 h |
| 7.1.5 | Implement cover data generation: analyse geometry, mark cover edges. | 4 h |
| 7.1.6 | Implement preferred paths and cell-border linking. | 4 h |
| 7.1.7 | Implement auto-generation (Recast/Detour integration): place NavmeshSeedMarker, seed auto-gen, review and finalize. | 8 h |
| 7.1.8 | Wire all edits back to `NavmeshRecord` save — vertex/triangle/edge changes persist on Ctrl+S. | 4 h |
| | **Total 7.1** | **44 h** |

### 7.2 Quest & Dialogue Improvements

Addresses finding: **#30**

| Step | Description | Effort |
|---|---|---|
| 7.2.1 | Add Quest-Integrated Dialogue View tab to `QuestEditor`: show all DIAL topics linked to the quest, all INFO responses under each topic, filterable by NPC. | 6 h |
| 7.2.2 | Improve `DialogueTreeEditor`: visual flowchart with topic nodes (rectangles), response nodes (rounded), condition indicators (diamond), connecting arrows, zoom/pan. | 8 h |
| 7.2.3 | Wire script fragments: show/edit Papyrus script fragments directly in QUST/DIAL/INFO editor panes. Compile on save. | 4 h |
| | **Total 7.2** | **18 h** |

### 7.3 Scene Editor

Addresses finding: **#29**; depends on SCEN record from Phase 4.16.

| Step | Description | Effort |
|---|---|---|
| 7.3.1 | Create `SceneEditor` widget with phase list (ordered tree view), action list per phase, actor assignments. Drag-drop dialogue+animation+package to each action slot. | 8 h |
| 7.3.2 | Save/load scene data to/from SCEN record. | 2 h |
| 7.3.3 | Wire scene to Render Window: preview actor positions and camera cuts per phase. | 4 h |
| | **Total 7.3** | **14 h** |

### 7.4 World LOD / PreVis / PreCombine

Addresses finding: **#38**

| Step | Description | Effort |
|---|---|---|
| 7.4.1 | Implement object LOD generation: for a worldspace, generate reduced-mesh versions of STAT/TREE records beyond LOD border. Write to LOD resource files. | 6 h |
| 7.4.2 | Implement tree LOD billboard generation (reuse/refactor `TreeLodGenerator` if it exists). | 3 h |
| 7.4.3 | Implement PreCombined meshes and PreVis (occlusion visibility). This is advanced CK feature — research format first. | 6 h |
| | **Total 7.4** | **15 h** |

### 7.5 Workflow Tool Improvements

Addresses finding: Phase7.md from original plan (merge, load order, search)

| Step | Description | Effort |
|---|---|---|
| 7.5.1 | Enhanced plugin merge: add FACT_ merge support, merge preview panel (see what will change before committing), auto-resolve for simple conflicts (take from master, take from override). | 4 h |
| 7.5.2 | Load order: circular dependency detection via DFS (not just pairwise), load order validation report (missing masters, version mismatches, FormID overlap), auto-fix button. | 4 h |
| 7.5.3 | Enhanced search: wire regex support to UI (toggle button), search history (last 20 searches in QSettings), saved searches (JSON persistence), multi-criteria search (AND/OR rows). | 4 h |
| 7.5.4 | Add keyboard shortcuts for all workflow tools. | 2 h |
| | **Total 7.5** | **14 h** |

| | **Phase 7 Total** | **105 h** |

---

## Phase 8 — Editor Features & File Formats

### 8.1 BSA/BA2 Archive Writing

Addresses finding: **#10**

| Step | Description | Effort |
|---|---|---|
| 8.1.1 | Implement BA2 writer: general archive format (file table, name table, compressed/uncompressed chunks), texture archive format. | 12 h |
| 8.1.2 | Implement BSA reader + writer: legacy Skyrim/Oblivion archive format (hash-based file lookup). | 8 h |
| 8.1.3 | Create "Create Mod Archive" dialog (File menu): select assets (meshes, textures, sounds), choose archive type (BA2/BSA), output path. | 4 h |
| 8.1.4 | Wire archive creation into batch export workflow. | 2 h |
| | **Total 8.1** | **26 h** |

### 8.2 ESL Light Master Support

Addresses finding: **#11**

| Step | Description | Effort |
|---|---|---|
| 8.2.1 | Add `FilterFlag_ESL` to file flags. Detect/read `.esl` files in the file selector. | 2 h |
| 8.2.2 | Implement "Convert to Light Master" tool: renumber FormIDs into FE namespace (0xFE000000–0xFEFFF), create new `.esl` file. | 6 h |
| 8.2.3 | Implement "Compact Form IDs" tool: scan all records, reassign to smallest possible FormIDs within FE range. | 4 h |
| 8.2.4 | Handle FE namespace correctly in REFR and all cross-references. | 4 h |
| | **Total 8.2** | **16 h** |

### 8.3 Find/Replace

Addresses finding: **#27**

| Step | Description | Effort |
|---|---|---|
| 8.3.1 | Add replace functionality to `SearchDialog`: a second text field "Replace with:" + "Replace" and "Replace All" buttons. Apply replacement via `EditRecordCommand` for undo. | 3 h |
| 8.3.2 | Scope replace to: selected records, visible table rows, all loaded records. | 2 h |
| 8.3.3 | Wire keyboard shortcut Ctrl+H to open dialog in replace mode. | 0.5 h |
| | **Total 8.3** | **5.5 h** |

### 8.4 Use Info Window

Addresses finding: **#28**

| Step | Description | Effort |
|---|---|---|
| 8.4.1 | Create `UseInfoDialog`: right-click on any form/record → "Use Info". Shows all records that reference this one, grouped by reference type. | 4 h |
| 8.4.2 | Add "count in cells" — traverse REFR to show how many times this form is placed in the world. | 2 h |
| 8.4.3 | Add "what breaks if deleted" summary. | 1 h |
| | **Total 8.4** | **7 h** |

### 8.5 OGG Audio Fix

Addresses finding: **#13**

| Step | Description | Effort |
|---|---|---|
| 8.5.1 | Fix `OggDecoder`: use `stb_vorbis` correctly — pass the 16-bit PCM samples to the output buffer, not raw Vorbis packets. | 3 h |
| 8.5.2 | Fix `OggEncoder`: `oggencoder.cpp:27-39` always returns false. Implement real Vorbis encoding with libvorbis: set up vorbis_info/dsp/block, write to memory buffer, flush. | 4 h |
| 8.5.3 | Add round-trip test: encode a known WAV to OGG, decode back, compare amplitude within tolerance. | 1 h |
| | **Total 8.5** | **8 h** |

### 8.6 Spell Wizard

Addresses finding: **#34**

| Step | Description | Effort |
|---|---|---|
| 8.6.1 | Make `SpellWizard` functional: template selection, name/value/weight, effects populated from predefined MGEF lists. | 3 h |
| 8.6.2 | Wire to record creation: wizard outputs a full SPEL record with effects, ENCH, and descriptive text. | 1 h |
| | **Total 8.6** | **4 h** |

### 8.7 Batch Rename Tool

Addresses finding: **#36**

| Step | Description | Effort |
|---|---|---|
| 8.7.1 | Add batch rename to `BatchTools`: pattern text field (with find/replace), prefix/suffix, enumeration with start number + padding. | 3 h |
| 8.7.2 | Preview list before applying, undo support. | 2 h |
| | **Total 8.7** | **5 h** |

### 8.8 Quick Access / Favourites

Addresses finding: **#35**

| Step | Description | Effort |
|---|---|---|
| 8.8.1 | Add "Add to Favourites" right-click action on any record. Store favourites list in QSettings. | 2 h |
| 8.8.2 | Add "Favourites" top-level node in Object Window tree. | 1 h |
| | **Total 8.8** | **3 h** |

### 8.9 DDS Texture Editing

Addresses finding: **#40**

| Step | Description | Effort |
|---|---|---|
| 8.9.1 | Replace the readonly `textureeditordialog.cpp` with a Qt `QGraphicsView` canvas that can open, modify, and save DDS textures (via `QImage` with DDS plugin or `stb_image`). | 4 h |
| 8.9.2 | Add basic paint tools: brush, color picker, eyedropper. | 3 h |
| | **Total 8.9** | **7 h** |

### 8.10 FaceGen (Partial)

Addresses finding: **#33**

| Step | Description | Effort |
|---|---|---|
| 8.10.1 | Implement FaceGen data export (Ctrl+F4): write head geometry NIF (tri file or equivalent), tint textures (DDS), and associated data from NPC_ morph fields. | 6 h |
| 8.10.2 | Add tint editing UI (hair colour, skin, scars, warpaint) with sliders and colour pickers. | 4 h |
| 8.10.3 | Wire to NPC_ editor face tab. | 2 h |
| | **Total 8.10** | **12 h** |

### 8.11 Lip Sync / FaceFX (Partial)

Addressing finding: **#32**
Note: Full FaceFX integration requires a licensed FaceFX runtime — OpenCK
cannot ship one. This step generates .lip data via an open algorithm.

| Step | Description | Effort |
|---|---|---|
| 8.11.1 | Research .lip file format (community-documented: phone-me mapping). | 3 h |
| 8.11.2 | Implement basic .lip generation: analyse audio envelope to produce viseme timing data. | 5 h |
| 8.11.3 | Wire to INFO editor: "Generate Lip Sync" button. | 1 h |
| | **Total 8.11** | **9 h** |

| | **Phase 8 Total** | **~103 h** |

---

## Phase 9 — Testing, Documentation, and Polish

### 9.1 Comprehensive Integration Tests

Addresses finding: **#42**

| Step | Description | Effort |
|---|---|---|
| 9.1.1 | `test_edit_save_roundtrip`: load real ESM, edit a record, save to temp file, reload, verify edit persisted. | 3 h |
| 9.1.2 | `test_create_save_reload`: create new plugin, add one record of each type, save, reload, verify all records present. | 4 h |
| 9.1.3 | `test_undo_redo_roundtrip`: edit record, verify changed, undo, verify reverted, redo, verify changed again, save/reload. | 3 h |
| 9.1.4 | `test_component_save_roundtrip`: for each component-migrated record, load, save, reload, verify all component fields. | 2 h |
| 9.1.5 | `test_subrecord_roundtrip` (from Phase 0): byte-for-byte comparison. | Already done in Phase 0 |
| 9.1.6 | `test_compressed_edit_roundtrip`: load a compressed record, edit, save, reload. | 2 h |
| 9.1.7 | `test_landscape_persistence`: landscape edit → save → reload → verify heightmap. | 2 h |
| 9.1.8 | `test_navmesh_persistence`: navmesh edit → save → reload → verify vertex/triangle count. | 2 h |
| | **Total 9.1** | **18 h** |

### 9.2 Resolve Planning Document Contradictions

Addresses finding: **#41**

| Step | Description | Effort |
|---|---|---|
| 9.2.1 | Rewrite STATUS.md to reflect actual state (use NewFindings.md as source of truth). | 2 h |
| 9.2.2 | Consolidate all phase files into a single plan. Remove contradictory files. | 1 h |
| 9.2.3 | Update TECHNICAL_DEBT.md to list all 50+ actual issues. | 1 h |
| 9.2.4 | Create a single ROADMAP.md that references this plan file. | 1 h |
| | **Total 9.2** | **5 h** |

### 9.3 Inconsistent Naming Cleanup

Addresses finding: **#44**

| Step | Description | Effort |
|---|---|---|
| 9.3.1 | Agree on a naming convention (e.g., `{Record4CC}Record` for structs, `Type_{4CC}` for enum). | 0.5 h |
| 9.3.2 | Rename existing classes that violate convention. Update all references. | 3 h |
| 9.3.3 | Fix filename casing for consistency (e.g., `Armorrecord.cpp` → `ArmorRecord.cpp`). | 1 h |
| | **Total 9.3** | **4.5 h** |

### 9.4 .ui File Migration

Addresses finding: **#43**
Note: This is a long-term polish item, not required for a usable build.

| Step | Description | Effort |
|---|---|---|
| 9.4.1 | For the top 5 most-complex editors (NPC_, QUST, WEAP, ARMO, SCEN), extract layout to `.ui` files. | 5 h |
| 9.4.2 | Document the pattern for future conversions. | 1 h |
| | **Total 9.4** | **6 h** |

### 9.5 AGENTS.md Update

| Step | Description | Effort |
|---|---|---|
| 9.5.1 | Add the canonical save() pattern from Phase 1.3 to AGENTS.md. | 0.5 h |
| 9.5.2 | Add the subrecord naming convention table from Phase 0. | 0.5 h |
| 9.5.3 | Add a checklist for adding a new record type (including the Phase 4 record-planning steps). | 0.5 h |
| | **Total 9.5** | **1.5 h** |

| | **Phase 9 Total** | **35 h** |

---

## Master Summary

| Phase | Name | Effort |
|---|---|---|
| 0 | Establish Ground Truth | 14 h |
| 1 | Showstopper Bug Fixes | 43 h |
| 2 | Record Completeness (Part 1) | 73 h |
| 3 | Pervasive Quality Issues | 31 h |
| 4 | Add Missing High-Priority Record Types | ~210 h |
| 5 | Animation & Particles | 43 h |
| 6 | 3D World (Render Window + Landscape) | 103 h |
| 7 | Advanced Editing | 105 h |
| 8 | Editor Features & File Formats | ~103 h |
| 9 | Testing, Documentation & Polish | 35 h |
| | **Grand Total** | **~760 h** |

≈ 19 weeks at 40 h/week, ≈ 5 months for one developer. With overhead for
debugging, design decisions, and integration testing, plan for **6–9 months**.

### Dependency Graph

```
Phase 0 (ground truth)
  └─► Phase 1.1 (fix subrecord names)
       ├─► Phase 2 (complete existing records — correct names only)
       │    └─► Phase 4 (add new record types — correct names on day one)
       ├─► Phase 3 (quality fixes — independent)
       └─► Phase 1.2 (fix RecordEditCommand — independent)
            └─► Phase 1.3 (fix component save — depends on 1.1-1.2)

Phase 2 ──► Phase 7.2 (quest/dialogue — needs complete DIAL/INFO/QUST)

Phase 4.14 (NAVM record) ──► Phase 7.1 (navmesh editor)
Phase 4.16 (SCEN record)  ──► Phase 7.3 (scene editor)

Phase 5 (animation/particles) ──► independent of 3D world
Phase 6 (render window) ──► Phase 7.1 (navmesh overlay)
                            └─► Phase 7.4 (LOD preview)

Phase 8 (features/formats) ──► largely independent

Phase 9 (testing/docs)    ──► throughout, intensive at end
```
