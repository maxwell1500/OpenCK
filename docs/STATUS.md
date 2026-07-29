# OpenCK — Current Project Status

> Last updated: 2026-07-28

## Project Summary

OpenCK is a C++/Qt6 open-source recreation of Bethesda's Creation Kit, the
official plugin editor for Bethesda's games (Morrowind, Oblivion, Skyrim,
Fallout 4, Starfield). It lets users open, edit, and save `.esp`/`.esm`
plugin files without requiring the proprietary Creation Kit. OpenCK ships
no Bethesda assets and contains none of Bethesda's source code — all
behavior is independently reimplemented from publicly documented file
formats and observable behavior.

## Completion Overview

**176 / 182 tasks complete across 13 phases (~96.7%).**

Phase 11 (Documentation & Final Polish) is the only remaining open phase.

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
| 11 — Documentation & Final Polish | 0/5 | ⬜ |
| 12 — UI Layout Parity with Real CK | 39/40 | ✅ |
| **TOTAL** | **176/182** | |

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
- **26 tests passing** — covering ESM I/O round-trip, components,
  form dialogs, editor widgets, undo/redo, conflict detection,
  Starfield ESM loading, and more.

## Known Limitations

- **12D.13 — CK File-menu actions not wired.** Create Archive,
  Compile Papyrus Scripts, and Compact Master exist as stub menu
  entries only; no backing implementations yet.
- **CREA (Creature) editor widget not built.** The CREA record has
  components wired but no specialized editor widget (5B.2 is ◐).
- **SCEN / EFSH / PACK editors deferred.** Scene (5B.6),
  EffectShader/ImageSpaceModifier (5B.16), and AI Package (5B.15)
  specialized widgets are not yet implemented.
- **Render Window edit modules are placeholders.** The
  Select/Move/Rotate/Scale toolbar actions exist (12G.1–12G.3) but
  the gizmo implementation (`BGSRenderWindowEditModule` pattern) is
  deferred — transform mode is a placeholder enum with no on-canvas
  manipulators.

## Build Instructions

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.5.3/msvc2019_64
cmake --build build --config Debug
```

## Test Instructions

```powershell
$env:Path = "C:\Qt\6.5.3\msvc2019_64\bin;$env:Path"
Get-ChildItem build\bin\Debug\test_*.exe | ForEach-Object { & $_.FullName }
```

All 26 `test_*.exe` binaries should exit 0.

## Source

Progress data is mirrored from `docs/UNIFIED_PLAN.md` (the unified
completion plan reconciling the ESM I/O plan, the real-CK 395-file
probe, the Tes4Codes cross-reference, and the original 10-phase plan).