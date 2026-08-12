# OpenCK — Current Project Status

> Last updated: 2026-08-12

## Project Summary

OpenCK is a C++/Qt6 open-source recreation of Bethesda's Creation Kit, the
official plugin editor for Bethesda's games (Morrowind, Oblivion, Skyrim,
Fallout 4, Starfield). It lets users open, edit, and save `.esp`/`.esm`
plugin files without requiring the proprietary Creation Kit. OpenCK ships
no Bethesda assets and contains none of Bethesda's source code — all
behavior is independently reimplemented from publicly documented file
formats and observable behavior.

## Completion Overview

**All tracked closing phases in `docs/REMAINING_WORK_PLAN.md` are complete
(Phases A–G ✅).** The earlier "310/310 in the tracker" claim in
`docs/UNIFIED_PLAN.md` was audited and corrected — see the plan doc, which
supersedes it. Release `v1.0.0` is cut (installer attached); the tracker's
Phase G row records the re-cut on the finished commit.

Honest caveats:
- **107 `test_*.exe` binaries** are built from **108 `tests/test_*.cpp`
  sources** (1 of the sources, `test_stubs.cpp`, compiles into the
  `test_stubs` linkage-shim static lib); **104 are registered in CTest**
  (plus 2 vendored-ogg tests = 106 total); **3 built exes are not**
  (`dumpesm`, `scanbtd`, `meshprobe` — non-QTest diagnostic tools,
  intentionally unregistered).
- **~7 of the registered tests require real game data** and are gated in
  CMake behind `if (EXISTS "C:/XboxGames/...")` — they are only registered
  on machines that have the game data (they skip cleanly — exit 0 — when
  the data is absent).
- **0 tests currently failing** — full fleet green: Debug 107/107 exes,
  106/106 CTest locally, and the Release CI gate green on GitHub Actions
  (100% — 97/97 registered on CI, 2026-08-12, run 31604541397). A `Tests →
  Run All Tests` menu action runs every `test_*.exe` via `openck --cli
  selftest` (Phase F4).

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
- **18 top-level menus** matching the real Creation Kit's shape
  (File, Edit, View, World, Character, ObjectWindows, RenderWindows,
  Navmesh, Terrain, Audio, Docks, Gameplay, Plugins, Export, Tools,
  Theme, Tests, Help). Empty placeholder menus (Galaxy/Packin) were
  removed rather than left as stubs; menu actions are all reachable
  or honestly disabled (see `docs/REMAINING_WORK_PLAN.md` Phase D).
  The Tests menu (F4) runs the headless self-test suite.
- **Papyrus scripting** — if/else/elif, while/for loops, and
  type checking implemented.
- **Dialogue & quest editing** — Quest stage tree, alias/objective
  editors, topic/response editors, voice file association, condition
  grids.
- **107 test binaries built** — covering ESM I/O round-trip, components,
  form dialogs, editor widgets, undo/redo, conflict detection,
  Starfield ESM loading, and more (104 CTest-registered, 107/107 green —
  see Test Instructions).

## Known Limitations

- **3 built `test_*.exe` binaries are not registered in CTest** — `dumpesm`,
  `scanbtd`, `meshprobe` are non-QTest diagnostic CLI tools and are built
  but intentionally unregistered.
- **~7 registered tests require real game data** (a `C:/XboxGames/...`
  install) and are registered via `if (EXISTS ...)` CMake gates, so they
  only run on machines that have the files; they exit 0 when absent.
- **0 test sources are orphaned** — the B1/B2 cleanup is complete:
  `test_groundtruth` and `test_subrecord_roundtrip` were rebuilt and
  registered, `test_loader` was rebuilt and registered, and the dead
  sources (`test_dataexporter`, `test_recordloading`, `test_undo`) were
  deleted; `test_stubs` remains as an intentional linkage shim
  (static lib, no exe).
  - **Several UNIFIED_PLAN tasks marked ✅ still carry "Partial" caveats**
  (binary encodings awaiting real-data validation): SCEN PHDA (E4),
  EFSH/IMGS/PACK/LCTN raw layouts (E1), NavMesh NIF gen (E5). All resolved
  except **E4 (SCEN PHDA encoder), which remains data-blocked**: Skyrim SE /
  Starfield.esm contain zero PHDA subrecords (both use the new-generation
  scene schema), so validating an encoder needs a FO4 install or a
  classic-format scene mod sample. Raw-subrecord round-trip is byte-exact.
  Tracked in `docs/REMAINING_WORK_PLAN.md` Phase E.
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

Test-count snapshot (2026-08-12): **108 `tests/test_*.cpp` sources** →
**107 built `test_*.exe` binaries** in `build/bin/Debug/` → **104 registered
in CTest** (`ctest --test-dir build -C Debug`; 106 total incl. 2 vendored
ogg tests, `test_loader` registered with `OPENCK_LOG_DIR`). 3 built exes
are not registered (`dumpesm`, `scanbtd`, `meshprobe` — diagnostic CLI
tools). 0 orphan sources remain.

All `test_*.exe` binaries exit 0 — verified 107/107 Debug and 107/107
Release locally (2026-08-12); Release gate green on CI 100% (2026-08-12,
run 31604541397). From the app, `Tests → Run All Tests` (or
`openck --cli selftest`) runs the same fleet and reports per-test PASS/FAIL.

```powershell
$env:Path = "C:\Qt\6.5.3\msvc2019_64\bin;$env:Path"
Get-ChildItem build\bin\Debug\test_*.exe | ForEach-Object { & $_.FullName }
```

## Source

Progress data is mirrored from `docs/UNIFIED_PLAN.md` (the unified
completion plan reconciling the ESM I/O plan, the real-CK 395-file
probe, the Tes4Codes cross-reference, and the original 10-phase plan).