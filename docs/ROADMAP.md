# OpenCK — Future Roadmap

> Last updated: 2026-08-01
> Prioritized goals for closing the remaining gaps with the real Creation Kit
> and extending OpenCK's capabilities. Cross-references point to tasks in
> `docs/UNIFIED_PLAN.md` and items in `docs/TECHNICAL_DEBT.md`.
> Phases 13–20 of `docs/UNIFIED_PLAN.md` map onto the horizons below.

## Horizon Legend

| Horizon | Scope |
|---------|-------|
| **Short-term** | Next release. Closes known high-severity gaps. |
| **Medium-term** | Next 2–4 releases. New editor systems and workflow features. |
| **Long-term** | 6+ months out. New game support, exotic editors, ecosystem. |

---

## Short-term (next release) — Phase 14 active, 15–17 queued

| # | Goal | Ref | Notes |
|---|------|-----|-------|
| S1 | Wire 12D.13 missing File-menu actions | UNIFIED_PLAN 12D.13; TD H6 | Create Archive → wrap `Tools/Archive2`; Compile Papyrus Scripts → existing compiler wrapper; Compact Master → real formID renumbering (23.4). |
| S2 | Render Window gizmo system (move/rotate/scale manipulators) | TD H1; UNIFIED_PLAN 14A | The `BGSRenderWindowEditModule` pattern — on-canvas manipulators wired to the existing `EditMode` enum in `nifviewportwidget.hpp:244`. Toolbar already in place (12G). 13 tasks, in progress. |
| S3 | Interactive Cell View 2D map | TD H2; UNIFIED_PLAN 14B | Click to select references, zoom/pan, hover highlight. `CellMapCanvas` in `cellsdialog.cpp:32` is paint-only today. 10 tasks, in progress. |
| S4 | Record coverage + Object Window completion | UNIFIED_PLAN 15 | ~52 of 88 categories are `Type_None` shells; add `.filter` support, CREA editor, Warnings dock population, status-bar wiring. |
| S5 | Specialized editor completion | UNIFIED_PLAN 16 | SCEN, EFSH, PACK, WRLD, LCTN, PNDT, CCT editors + NavMesh tools. |
| S6 | Terrain & landscape completion | UNIFIED_PLAN 17 | `.lbr` brush system, overlay masks, heightmap import, terrain undo/redo, BTD files. |

---

## Medium-term (next 2–4 releases) — Phases 18–22

| # | Goal | Ref | Notes |
|---|------|-----|-------|
| M1 | Audio pipeline | UNIFIED_PLAN 18 | Playback engine, XWM/FUZ decode, LipGenerator (Fonix), FaceFX, Wwise soundbanks, RoboVoicer TTS. |
| M2 | Material editor & asset pipeline | UNIFIED_PLAN 19 | BSMaterial property graph (64 shader-model rule templates), DDS import, texture conversion (xtexconv rules), mesh/phys LOD, FBX→NIF. |
| M3 | Particle editor + icon generation | UNIFIED_PLAN 20 | `.pofx` bundle nodes, LOD presets, projectile variables, 3-light icon renderer. |
| M4 | Scripting completion | UNIFIED_PLAN 21 | `.ppj` projects, Script Manager, flags, spell-check, Papyrus LSP, remote debugger protocol. |
| M5 | Behavior / animation graph editor | UNIFIED_PLAN 22 | FlowChartX pattern — 43+ node types, blend trees, animation event validation. |
| M6 | Data workflows & plugin utilities | UNIFIED_PLAN 23 | CSV Snippets (nested templates), OPAL lists, Find Forms, real compaction, MMS, Bethesda.net upload. |
| M7 | Infrastructure & ecosystem | UNIFIED_PLAN 24 | i18n, VCS (Perforce/Git), CI/CD, packaging, headless API, shortcut expansion. |

---

## Long-term (6+ months out)

| # | Goal | Notes |
|---|------|-------|
| L1 | Spaceship editor (Starfield) | Starfield-specific ship builder module. |
| L2 | Galaxy view | Starfield galaxy-level overview panel (currently a stub menu). |
| L3 | Worldspace/planet generation editors | PNDT planet editor (16.6), CCT creature editor (16.7), OPAL procedural placement (23.2). |
| L4 | Multi-game record dispatch | Morrowind (TES3), Oblivion (TES4), Skyrim (TES5), Fallout 4, Starfield — record formats and game-specific editors. |
| L5 | Houdini integration | Side-chain Houdini export/import for procedural world generation. |
| L6 | Reflection probe editing | Place, edit, and bake reflection probes in worldspaces. |
| L7 | Crowd region editing | Crowd/NPC population region authoring. |
| L8 | Morph editor | Head/morph part authoring for face-gen pipelines (CK `[Morph] bChargenMorphLODsEnabled`). |
| L9 | Icon generation renderer | 3-light rig, per-context sizes (20.5). |
| L10 | RoboVoicer / voice automation | TTS pipeline for dialogue VO (18.6). |

## Sequencing Notes

- **S2/S3 (Phase 14)** are the active milestone — they close the two
  highest-severity items in `TECHNICAL_DEBT.md` (TD H1, TD H2) and are
  prerequisites for the other short-term goals.
- **S4–S6 (Phases 15–17)** close the biggest *visible* parity gaps:
  dead Object Window categories, missing record editors, and the
  landscape toolset.
- **M1–M7 (Phases 18–24)** deliver the real CK's professional pipelines
  (audio, materials, particles, scripting, behavior graphs, data
  workflows) plus ecosystem infrastructure (i18n, VCS, CI, packaging).
- **L9 (multi-game)** is the largest long-term effort and should be
  sequenced after the Starfield editor surface (L1, L2) stabilizes,
  so the game-dispatch layer has concrete cases to generalize from.