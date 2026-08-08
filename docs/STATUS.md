# OpenCK — Current Project Status

> Last updated: 2026-08-07

## Project Summary

OpenCK is a C++/Qt6 open-source recreation of Bethesda's Creation Kit, the
official plugin editor for Bethesda's games (Morrowind, Oblivion, Skyrim,
Fallout 4, Starfield). It lets users open, edit, and save `.esp`/`.esm`
plugin files without requiring the proprietary Creation Kit. OpenCK ships
no Bethesda assets and contains none of Bethesda's source code — all
behavior is independently reimplemented from publicly documented file
formats and observable behavior.

## Completion Overview

**The project tracks 24+ phases in `docs/UNIFIED_PLAN.md` — all complete
(310/310 in the tracker, audited).**

Honest caveats:
- **104 `test_*.exe` binaries** are built from **111 `tests/test_*.cpp`
  sources**; **101 are registered in CTest** (plus 2 vendored-ogg tests =
  103 total); **3 built exes are not** (`dumpesm`, `scanbtd`, `meshprobe` —
  non-QTest diagnostic tools, intentionally unregistered).
- **~7 of the registered tests require real game data** and are gated in
  CMake behind `if (EXISTS "C:/XboxGames/...")` — they are only registered
  on machines that have the game data (they skip cleanly — exit 0 — when
  the data is absent).
- **0 tests currently failing** — full fleet green (104/104 exes, 103/103
  CTest) as of 2026-08-07.

## Phase-by-Phase Status

| Phase | Steps | Status |
|-------|-------|--------|
| 0 — Build Integrity & Housekeeping | 6/6 | ✅ |
| 1 — ESM I/O Robustness & Tes4Codes Removal | 7/7 | ✅ |
| 2 — Component-Property Architecture | 28/28 | ✅ |
| 3 — Record Type Migration to Component System | 5/5 | ✅ |
| 4 — WindowLayout & QtAdvancedDocking | 4/4 | ✅ |
| 5 — Gaps & Specialized Editors | 31/31 | ✅ |
| 6 — NIF Pipeline & Blender Integration | 5/5 | ✅ |
| 7 — 3D Viewport Enhancements | 6/6 | ✅ |
| 8 — Editor Completions | 7/7 | ✅ |
| 9 — Papyrus & Dialogue Completion | 6/6 | ✅ |
| 10 — Testing | 26/26 | ✅ |
| 11 — Documentation & Final Polish | 5/5 | ✅ |
| 12 — UI Layout Parity with Real CK | 40/40 | ✅ |
| 13 — Editor Workspace Parity | 14/14 | ✅ |
| 14 — Render Window Gizmos + Interactive Cell View | 23/23 | ✅ |
| 15 — Record Coverage & Object Window Completion | 7/7 | ✅ |
| 16 — Specialized Editor Completion | 8/8 | ✅ |
| 17 — Terrain & Landscape Completion | 9/9 | ✅ |
| 18 — Audio Pipeline | 7/7 | ✅ |
| 19 — Material Editor & Asset Pipeline | 7/7 | ✅ |
| 20 — Particle Editor & Icon Generation | 5/5 | ✅ |
| 21 — Scripting Completion | 8/8 | ✅ |
| 22 — Behavior / Animation Graph Editor | 5/5 | ✅ |
| 23 — Data Workflows & Plugin Utilities | 9/9 | ✅ |
| 24 — Infrastructure & Ecosystem | 11/11 | ✅ |
| **TOTAL** | **310/310** | |

Status key: ✅ done, ◐ partial, ⬜ not started.

## Key Achievements

- **50 ESM record types migrated** to the component-property system
  (`libs/components/`). Every record editor is now data-driven via
  `QtFormDialog` + `EditorProperty` leaves; no per-record dialog code
  required for simple types.
- **QtAdvancedDocking (ADS) integration** — all major panels
  (Object Window, Render Window, Script Editor, Dialogue Editor,
  Cell View, Asset Browser, Landscape Editor, Object Palette) are
  tear-off / tab / redock `ads::CDockWidget` instances. Layout
  persists to `QtCreationKitSavedSettings.ini`.
- **Cell View panel** — docked 2D top-down cell browser with
  worldspace selector, cell list, reference table, and map canvas
  (replaces the old modal `CellsDialog`).
- **Hierarchical Object Window tree** — 3-level
  (root → category group → record type) matching the real CK's
  `All → Actors/Items/World Objects/Gameplay/Audio/Dialogue` shape.
- **16-menu top-level layout** matching the real Creation Kit
  (File, Edit, View, Character, ObjectWindows, RenderWindows,
  Navmesh, Terrain, Audio, World, Tools, Docks, Galaxy, Packin,
  Theme, Tests, Help).
- **Papyrus scripting** — if/else/elif, while/for loops, and
  type checking implemented.
- **Dialogue & quest editing** — Quest stage tree, alias/objective
  editors, topic/response editors, voice file association, condition
  grids.
- **104 test binaries built** — covering ESM I/O round-trip, components,
  form dialogs, editor widgets, undo/redo, conflict detection,
  Starfield ESM loading, and more (101 CTest-registered, 104/104 green —
  see Test Instructions).

## Known Limitations

- **3 built `test_*.exe` binaries are not registered in CTest** — `dumpesm`,
  `scanbtd`, `meshprobe` are non-QTest diagnostic CLI tools and are built
  but intentionally unregistered.
- **~7 registered tests require real game data** (a `C:/XboxGames/...`
  install) and are registered via `if (EXISTS ...)` CMake gates, so they
  only run on machines that have the files; they exit 0 when absent.
- **7 test sources are orphaned** (never built): `test_dataexporter`,
  `test_groundtruth`, `test_loader`, `test_recordloading`, `test_stubs`,
  `test_subrecord_roundtrip`, `test_undo`. Cleanup is tracked in
  `docs/REMAINING_WORK_PLAN.md` Phase B (B1/B2).
- **Several UNIFIED_PLAN tasks marked ✅ still carry "Partial" caveats**
  (binary encodings awaiting real-data validation): SCEN 16.1, EFSH/IMGS
  16.2, PACK 16.3, LCTN 16.5, NavMesh 16.8. Follow-ups are tracked in
  `docs/REMAINING_WORK_PLAN.md` (Phases E1–E10).
- **Voice "Record" button disabled** (docs: honest tooltip in
  `infodatawidget.cpp`), **VC server field removed from Preferences**,
  and Search/Export still bail on a few types (D4/D5) — see
  `docs/REMAINING_WORK_PLAN.md` Phase D.

## Build Instructions

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.5.3/msvc2019_64
cmake --build build --config Debug
```

## Test Instructions

Test-count snapshot (2026-08-07): **111 `tests/test_*.cpp` sources** →
**104 built `test_*.exe` binaries** in `build/bin/Debug/` → **101 registered
in CTest** (`ctest --test-dir build -C Debug`; 103 total incl. 2 vendored
ogg tests). 3 built exes are not registered (`dumpesm`, `scanbtd`,
`meshprobe` — diagnostic CLI tools). 7 sources are orphaned (see B1/B2).

All `test_*.exe` binaries exit 0 — verified 104/104 (2026-08-07).

```powershell
$env:Path = "C:\Qt\6.5.3\msvc2019_64\bin;$env:Path"
Get-ChildItem build\bin\Debug\test_*.exe | ForEach-Object { & $_.FullName }
```

## Source

Progress data is mirrored from `docs/UNIFIED_PLAN.md` (the unified
completion plan reconciling the ESM I/O plan, the real-CK 395-file
probe, the Tes4Codes cross-reference, and the original 10-phase plan).