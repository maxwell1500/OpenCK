# OpenCK Gap Analysis: Real Creation Kit vs. Current State

Compiled from comprehensive codebase audit (230+ source files), real CK feature
research, integration plan analysis, and planning document review.

---

## TL;DR

The project documents claim **99% complete** but that is illusory. Two **critical**
bugs mean **every saved file is corrupt** and **every edit silently does nothing**.
Only **27% of record types** from the real CK are implemented (34 vs 127). Entire
subsystems (animation, particles, navmesh, render window) are entirely unbuilt.
The planning documents are **self-contradictory** — STATUS.md says "0 technical
debt items remaining" while Stage2.md lists 50+ unfixed issues. Estimated effort
to reach a usable state: **6–12 months** for one developer.

---

## Tier 1 — Showstoppers (nothing works without these)

### 1. ESM Subrecord Names Are Invented (CRITICAL)

Source: Stage2.md C-01
Effort: 20–30 hours

The record parsers use entirely invented subrecord names (`FORM`, `ITM2`,
`ODIT`, `FNAM`) that do not match any Bethesda ESM format. **Every file saved
through this system is corrupt** — cannot be loaded by the game, CK, or any
mod manager. Despite Phase8.md claiming this is "fixed" (they fixed the
`tes4codes.hpp` translation layer), the record parsers themselves still use
wrong names. Every `load()` and `save()` in every record needs to be checked
and corrected against the real subrecord codes.

### 2. RecordEditCommand.execute() Is a No-Op (CRITICAL)

Source: Stage2.md C-02
Effort: 1–2 hours

`RecordEditCommand::execute()` captures the current state but **never applies
the new value**. Every user edit silently does nothing. The user thinks they
edited a record, but saving and reloading shows the original values.

### 3. Core Save() Architecture Is Broken (Blocking all record editing)

Effort: ongoing

The component→flat mirror direction in save() violates const correctness, and
the flat→component direction silently discards property-grid edits. The
`externallySerialized` flag for primitive components is fragile — if not
properly managed through clone/blank/copy, data is silently lost or duplicated.
We have made progress on reading from components at write time but the approach
needs to be applied universally.

---

## Tier 2 — Major Missing Subsystems (months of work)

### 4. ~93 Missing Record Types (73% gap)

The real CK has 127 record editor files. OpenCK has 34. Completely absent
record types include:

| Category | Missing 4CC | Description |
|---|---|---|
| Combat | `AMMO` | Ammunition |
| | `PROJ` | Projectiles |
| | `HAZD` | Hazards |
| | `EXPL` | Explosions |
| World Objects | `DOOR` | Doors |
| | `FURN` | Furniture |
| | `LIGH` | Lights |
| | `MSTT` | Movable static |
| | `KEYM` | Keys |
| Actors | `CREA` | Creatures (TES3/4) |
| | `LVLN` / `LVLC` / `LVSP` | Leveled NPC / Creature / Spell lists |
| | `LVLI` | Leveled Item lists |
| Script/Quest | **`SCPT`** | **Papyrus script records — critical** |
| | `MESG` | Message boxes |
| | `NOTE` | Notes |
| | `FLST` | Form lists |
| Visual/Audio | `TXST` | Texture sets |
| | `CLMT` | Climate |
| | `IMGS` / `IMAD` | Image Space / Modifier |
| | `DEBR` | Debris |
| | `EYES` | Eye textures |
| | `HAIR` | Hair styles |
| | `HDPT` | Head parts |
| Navmesh | `NAVM` | Entire navigation mesh system |
| Misc | `KYWD` | Keyword definitions |
| | `OTFT` | Outfits |
| | `SHOU` | Shouts (Skyrim) |
| | `SLGM` | Soul gems |
| | `RELA` | Relationships |
| | `RGNS` | Regions |
| | `ANIO` | Animated objects |
| | `ART` | Art objects |
| | `IDLE` | Idle animations |
| | `IPCT` / `IPDS` | Impact data sets |
| | `CSTY` | Combat styles |
| | `DOBJ` | Default objects |
| | `LSCR` | Load screens |
| | `MUSC` / `MUSP` | Music tracks |
| | `SNCT` | Sound categories |
| | `SMRK` | Sound markers |
| | `VEFX` | Visual effects |
| | `SPGD` | Shader particle geometry |
| | `LGTM` | Lighting templates |
| | ... and ~50 more | Starfield-specific, FO4-specific |

Each needs: struct definition, load(), save(), blank(), component seeding,
column definitions. At ~1–2 hours each optimistically, that is **100–200 hours**.

Additionally, of the 34 existing records, **many have extremely incomplete
load() methods**:

| Record | What's Parsed | What's Missing |
|---|---|---|
| `RACE` | EDID + FLAG only | FULL name, skills, height/weight, head parts, bone data, voice types, body data, movement types |
| `DIAL` | EDID + FLAG only | FULL (topic name), DATA (type), QNAM — everything goes to rawSubRecords |
| `INFO` | FLAG, CNAM, CTDA, TLOI | Speaker (QNAM), response text (TRDT), actor (NAM1), emotion, scripts |
| `NPC_` | EDID, ACBS, FULL, RNAM, CNAM, ANAM, etc. | Body parts, attributes (str/int/etc), skills array, factions, AI packages, formation |
| `PACK` | EDID, FLAG, PKDT, PLDT, PTDT | PKD2/PLD2 (Skyrim), PNAM, conditions, script fragments |
| `QUST` | EDID, FLAG, FULL, DNAM, DATA | Stages (QSTA/QSDT), objectives (QOBJ), aliases, rewards, scripts |
| `PERK` | EDID, FLAG, DESC, ITM2, CTDA | Rank data (PRKR), multiple effect entries |
| `FACT` | EDID, FLAG, FULL, XNAM, ITM2, RNAM | DATA (flags), JAIL/CRGR conditions, multiple relation types |
| `CELL` | EDID, DATA, XCLC, XOWN, XLOC, FULL | XLCN (location), XCLL (lighting), 20+ others |
| `REFR` | NAME, DATA, XOWN, DNAM, XESP, SCRI | XSCL (scale), XRGD (ragdoll), XLTW (water), XTRI (lock) |
| `SOUN` | EDID, FNAM, SNDD/SNDX | CNAM (category), GNAM (type), HNAM (attenuation) |
| `WTHR` | EDID, FLAG, SNAM only | NAM0–9 clouds, DNAM colors, DATA timing — all raw |
| `SPEL` | FULL, EDID, FLAG, SPIT, SNAM, SPDT | Effects array not structured |
| `MGEF` | EDID, FLAG, MDOB, SNAM, ITM2, ODIT | Archetype, counter-effect, visual data, sounds |
| `ENCH` | EDID, FLAG, FULL, ENIT | Effect list, conditions |

### 5. No Render Window (3D Viewport)

**Real CK**: Full 3D viewport with orbit/pan/zoom camera, gizmo transforms
(move/rotate/scale), grid snapping, object selection and manipulation, visibility
toggles (sky/lights/markers/water/collision/wireframe/fog), cell preview,
navmesh visualization.

**OpenCK**: `nifviewportwidget` exists for model preview only. No worldspace/cell
rendering, no selection, no manipulation, no gizmos. **Deferred indefinitely**
in the integration plan. This is the single biggest missing feature for
world-building workflows — without it you cannot place objects, edit landscapes,
or view cells in context.

Estimate: **months of work** (3D engine integration, picking, transforms, undo).

### 6. No Landscape Editing

**Real CK**: Heightmap sculpting (raise/lower/flatten/smooth/noise), vertex
painting (texture layers with alpha blending), heightmap import/export (.raw),
brush-based with radius/strength/falloff.

**OpenCK**: `landscapeeditor.cpp` exists as a shell but **terrain rendering is
broken** (VBO double-allocation — Stage2 H-02). The landscape engine is
non-functional.

Estimate: **40–80 hours**.

### 7. No NavMesh System

**Real CK**: Full navmesh editing with triangle/vertex/edge creation, automatic
generation (Recast-based), validation, cover data, preferred paths, cell border
linking, color-coded error feedback (green=connected, red=error).

**OpenCK**: `navmesheditor.cpp` exists as a shell. No NavMeshGenerator
integration with navmesh record (NAVM is not defined in the enum). No
auto-generation, no validation, no cover data.

Estimate: **100+ hours**.

### 8. No Animation System

**Real CK**: Animation timeline, keyframe editing, NIF keyframe format (.kf),
import/export, viewport playback, blending, transitions.

**OpenCK**: The NIF parser **skips** NiKeyframeData/NiTransformData blocks
(`nifparser.cpp` lines 602–606). `animationeditor.cpp`, `timeline.cpp`, and
all animation commands exist but are **wired to nothing**. Phase5.md describes
7 new files needed but nothing has been started.

Estimate: **10–14 hours** (Phase5.md estimate).

### 9. No Particle System

**Real CK**: Particle editor (NiParticleSystem), emitter-based effects (fire,
smoke, sparks, magic), viewport rendering, BSLightingShaderProperty.

**OpenCK**: `ParticleEffectsParser` is a **stub** that returns dummy data.
The editor UI is fully coded but wired to nothing. No NiParticleSystem block
classes. Phase6.md describes 6 new files needed but nothing has been started.

Estimate: **6–8 hours** (Phase6.md estimate).

### 10. No BSA/BA2 Writing

**Real CK**: Creates `.bsa` (legacy) and `.ba2` (Fallout 4/Starfield) archives
for mod distribution with asset packing (meshes, textures, sounds).

**OpenCK**: BA2 **reader** exists in `libs/files/ba2/ba2archive.cpp`. No writer
of any kind. No BSA reader or writer. No mod archiving at all.

Estimate: **20–40 hours**.

### 11. No ESL (Light Master) Support

**Real CK**: Creates `.esl` files — compact mod format with FE namespace,
max ~4000 forms, bypasses the 255-plugin limit. "Convert Active File to Light
Master" and "Compact Active File Form IDs" tools exist.

**OpenCK**: No ESL creation, no compact FormID tool, no FE namespace handling,
no `.esl` in the file format dispatch.

Estimate: **10–20 hours**.

---

## Tier 3 — Pervasive Quality Issues (codebase is fragile)

### 12. 200+ const_cast Calls (H-01)

Source: Stage2.md H-01
Effort: 4–6 hours

Systemic undefined-behaviour risk. The view layer uses `const_cast` to route
around const-correctness issues throughout. This is fragile and could silently
break with compiler optimisations.

### 13. OGG Audio Broken (M-01)

Source: Stage2.md M-01 + Phase8.md
Effort: 4–8 hours

The OGG decoder treats Vorbis data as raw PCM — produces noise instead of
audio. The encoder (`oggencoder.cpp:27–39`) **always returns false** with no
error message. Phase8.md explicitly lists this as remaining. Audio is completely
broken for both playback and export.

### 14. Float Position Truncation (H-03)

Source: Stage2.md H-03
Effort: 1–2 hours

`QSpinBox` (integer-only) is used for float position/rotation/scale fields in
the REFR editor. Coordinates are silently truncated to integers — objects can
never be placed at fractional positions and sub-metre precision is lost.

### 15. Object Palette Byte-Order Mismatch (H-04)

Source: Stage2.md H-04
Effort: <1 hour

Saves in big-endian (`QDataStream::BigEndian`), loads in little-endian
(`QDataStream::LittleEndian`). All saved object palette files are unreadable.

### 16. getColumnId() Always Returns Wrong Column (H-05)

Source: Stage2.md H-05
Effort: <30 minutes

Compares `name_` against `name_` (itself) instead of against the array member
`columns_[i]->name_`. Search/filter functionality is broken for all columns.

### 17. Feature Flag Overlap (H-06)

Source: Stage2.md H-06
Effort: <30 minutes

`Feature_ViewId = 3` overlaps with `Feature_Constant | Feature_AllowTouch`.
The ViewId feature can never work correctly — it is indistinguishable from
the combination of two other flags.

### 18. UI Hangs on Compiler Hang (H-07)

Source: Stage2.md H-07
Effort: 1–2 hours

`process.waitForFinished(-1)` in the Papyrus compiler blocks the UI thread
forever if the compiler process hangs. The application becomes unkillable
without Task Manager.

### 19. saveRecord() Validates After Mutation (M-04)

Source: Stage2.md M-04
Effort: 2–4 hours

Across all editors, `saveRecord()` mutates the record first, then validates.
If validation fails, the record is already partially mutated with no rollback.
Silently corrupts data on validation failure.

### 20. Logger Data Race (M-10)

Source: Stage2.md M-10
Effort: 1–2 hours

`m_initialized` boolean is read and written without synchronisation across
threads. Undefined behaviour if a log line is emitted before the logger is
initialised (common during static construction).

### 21. Brute-Force FormID Search (M-07)

Source: Stage2.md M-07
Effort: 2–4 hours

`Data::createNewRecord()` uses O(n*m) brute-force FormID search. Causes
visible UI lag spikes in large plugins with many records.

### 22. Missing Qt parent in MastersList (M-05)

Source: Stage2.md M-05
Effort: <30 minutes

`MastersList` ignores the `parent` parameter and never sets a Qt parent.
Ownership is broken — memory leaks on every dialog close.

### 23. topologicalSort Returns Empty on Cycle (M-06)

Source: Stage2.md M-06
Effort: 1–2 hours

When a dependency cycle is detected, `topologicalSort()` silently returns an
empty vector. No error reporting — the caller flushes the entire masters list.
Ambiguous between "no plugins" and "circular dependency detected".

### 24. LandscapeEditCommand Stores Unused Params (M-08)

Source: Stage2.md M-08
Effort: 2–3 hours

`LandscapeEditCommand` stores `centerX`, `centerY`, and `radius` that are
never read. Misleading for future maintenance and wastes memory.

### 25. CMakeLists.txt Issues (M-09)

Source: Stage2.md M-09
Effort: 2–4 hours

Missing Qt5/Win32 components in CMake conditionals. No install targets. Some
paths are hardcoded. The build system is not portable.

### 26. No MacroCommand/EditRecordCommand Integration

The `MacroCommand` class exists but is not wired to the `EditRecordCommand`
system. Multi-step undo (e.g., editing a record field then adjusting a related
field) cannot be undone as a single unit.

---

## Tier 4 — Missing Editor Features

### 27. No Find/Replace

Search exists (Ctrl+F) but has **no replace functionality**. The real CK has
Ctrl+H with batch find/replace across all loaded records with field-scoping,
regex, and case-sensitivity options.

### 28. No Use Info Window

Right-click "Use Info" on a form — shows all other forms that reference it,
count of placements in cells, and dependency chains. **Critical for modding**
to understand what breaks if you remove a record.

### 29. No Scene (SCEN) Editor

Starfield/Skyrim scenes are multi-participant scripted sequences with phases,
actions per phase, dialogue assignments, actor animations, camera cuts.
Entirely absent from OpenCK — no SCEN record, no editor, no dispatch entry.

### 30. No Quest-Integrated Dialogue View

The real CK has a Dialogue Views tab inside the Quest editor that shows all
dialogue trees organised by NPC/topic. OpenCK has standalone DIAL/INFO editors
but no quest-integrated dialogue view.

### 31. No Havok Preview

Physics/collision preview for placed objects (ragdoll, havok-clutter behaviour)
is entirely missing. Modders cannot verify that physics interactions work
without loading the game.

### 32. No Lip Sync / FaceFX

Dialogue lip-sync generation (.lip / .fuz files) is entirely missing. Voice
files cannot be synchronised with facial animations.

### 33. No FaceGen

No facial geometry editing, no tint/hair/eyes/scars editing, no Ctrl+F4 FaceGen
export. The infamous "Dark Face Bug" workaround can't even be triggered.

### 34. No Spell Wizard

`spellwizard.cpp` exists but is likely a non-functional stub. The real CK has
a "Spell from Template" wizard that creates spells from predefined archetypes.

### 35. No Quick Access / Favourites

The real CK allows bookmarking frequently-used records. No equivalent in
OpenCK.

### 36. No Batch Rename Tool

The real CK's batch action window includes batch rename with pattern matching.
OpenCK has `batchtools.cpp` but batch rename is not implemented.

### 37. No Keyframe Animation in Timeline

`timeline.cpp` exists as a widget shell but has no paintEvent, no keyframe
editing, no time ruler, no lane display, no playback controls.

### 38. No World LOD / PreVis / PreCombine

The real CK has menus for World LOD, Generate PreVis, and Generate PreCombined
meshes for performance optimisation. None of these exist in OpenCK.

### 39. No Havok Behaviour Editing

The real CK (especially FO4/Starfield) has Havok Behaviour Project editing for
custom animation behaviour graphs. Not present.

### 40. No DDS Texture Editing

`textureeditordialog.cpp` exists but cannot actually edit DDS textures. Only
viewing is possible.

---

## Tier 5 — Documentation & Process Issues

### 41. Planning Documents Contradict Each Other

- **STATUS.md**: claims "99% complete", "68/69 steps", "0 tech debt items"
- **TECHNICAL_DEBT.md**: claims "100% resolved, 0 items remaining"
- **finalPhases.md**: claims "69/69 (100%)" — every step checked done
- **Stage2.md**: lists **50+ unfixed issues** including 2 criticals (C-01, C-02)
- **Phase5.md / Phase6.md / Phase7.md**: describe entire unbuilt subsystems
  (animation, particles, workflow improvements) with zero completion indicators
- **FinalRuns.md**: shows 17 remaining items, contradicts TECHNICAL_DEBT.md
- Multiple phase numbering schemes exist that don't align with each other

The status documents appear to have been written optimistically (or as a
roadmap) while the detailed phase documents and audit reveal reality.

### 42. Test Coverage Is Thin for Real-World Workflows

24 tests pass but they test individual components in isolation. **There is no
test that:**

- Loads a real plugin, edits a record, saves it, reloads it, and verifies the
  edit persisted (C-02 would be trivially caught)
- Creates a new plugin from scratch, adds records, saves, and reloads
- Tests the edit→undo→redo→save→reload round-trip
- Tests ESL/ESL conversion
- Tests the subrecord name round-trip (C-01 would be trivially caught)
- Tests save with the component system (externallySerialized flag management)
- Tests compressed record round-trip with modified data
- Tests landscape edit persistence
- Tests navmesh load/save

### 43. Only 4 .ui Files Exist

All ~80 editor dialogs are coded in C++ directly (no Qt Designer .ui files).
This makes UI iteration slower, designer collaboration harder, and increases
the LOC count for each dialog. The real CK uses Qt Designer extensively.

### 44. Inconsistent Class Naming

Some records use prefix naming (`ArmorRecord`, `WeaponRecord`), others don't
(`GameSetting`, `GlobalVariable`). The CkId enum uses `Type_NPC_` but the
class is `NpcRecord`. Some filenames are inconsistent (`Armorrecord.cpp` vs
`weaprecord.cpp`).

### 45. Dead Code / Unused Files

- `Type_RunLog` in the CkId enum is never instantiated
- `dataimporter2.cpp` and `dataimporter_new.cpp` suggest multiple abandoned
  import implementations
- Several `fix_records*.py` scripts in the root suggest batch-editing was done
  to get the code compiling, not to get it correct

---

## Estimated Total Effort

| Tier | Area | Effort Estimate |
|---|---|---|
| 1 | Fix subrecord names (C-01) | 20–30 h |
| 1 | Fix RecordEditCommand (C-02) | 1–2 h |
| 1 | Fix component save architecture | 10–20 h |
| 2 | Add ~93 missing record types | 100–200 h |
| 2 | Fill out 25 incomplete load() methods | 50–100 h |
| 2 | Render window / 3D viewport | 200–400 h |
| 2 | Landscape editing | 40–80 h |
| 2 | NavMesh system | 100+ h |
| 2 | Animation system | 10–14 h |
| 2 | Particle system | 6–8 h |
| 2 | BSA/BA2 writing | 20–40 h |
| 2 | ESL support | 10–20 h |
| 3 | Fix 50+ code quality issues | 40–60 h |
| 4 | Missing editor features | 60–100 h |
| 5 | Documentation & test cleanup | 20–40 h |
| **Total** | | **~700–1300 hours** |
| | | **(6–12 months for one developer)** |

## Bright Spots (What Actually Works)

Despite the vast gap, several subsystems are genuinely solid:

- **ESM/ESP binary reader** — handles compressed records, subrecord navigation,
  decompression, string tables. For reading, it works.
- **Component-Property architecture** — well-designed abstraction. Tier 1 and
  Tier 2 components are clean and composable.
- **QtAdvancedDocking integration** — functional dock manager with tear-off
  tabs, resizable panels, and layout management.
- **34 record types load (partially)** — enough structure to read most plugin
  files without crashing.
- **Undo stack** — functional undo/redo with macro grouping and merged stacks.
- **Multi-threaded loader** — background loading with progress signals works.
- **Plugin conflict detection** — side-by-side comparison dialog works.
- **NIF model rendering** — the 3D viewport widget renders NIF models correctly
  with normals, UVs, and lighting.
- **Papyrus compiler** — functional if/else/elif/while/for compiler with type
  checking (but no SCPT record support to persist compiled scripts).
- **4 game parsers** (Morrowind, Oblivion, Skyrim, Fallout 4/Starfield) exist
  with game-specific subrecord handling.

These form a real foundation — but the path to a usable CK replacement is
measured in months, not days.
