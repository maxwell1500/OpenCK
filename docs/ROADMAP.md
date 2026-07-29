# OpenCK — Future Roadmap

> Last updated: 2026-07-28
> Prioritized goals for closing the remaining gaps with the real Creation Kit
> and extending OpenCK's capabilities. Cross-references point to tasks in
> `docs/UNIFIED_PLAN.md` and items in `docs/TECHNICAL_DEBT.md`.

## Horizon Legend

| Horizon | Scope |
|---------|-------|
| **Short-term** | Next release. Closes known high-severity gaps. |
| **Medium-term** | Next 2–4 releases. New editor systems and workflow features. |
| **Long-term** | 6+ months out. New game support, exotic editors, ecosystem. |

---

## Short-term (next release)

| # | Goal | Ref | Notes |
|---|------|-----|-------|
| S1 | Wire 12D.13 missing File-menu actions | UNIFIED_PLAN 12D.13; TD H6 | Create Archive → wrap `Tools/Archive2`; Compile Papyrus Scripts → existing compiler wrapper; Compact Master → formID renumbering pass. |
| S2 | Implement Render Window gizmo system (move/rotate/scale manipulators) | TD H1; UNIFIED_PLAN 12G | The `BGSRenderWindowEditModule` pattern — on-canvas manipulators wired to the existing transform-mode enum. |
| S3 | Interactive Cell View 2D map | TD H2; UNIFIED_PLAN 12B.5 | Click to select references, zoom/pan, hover highlight. |
| S4 | CREA specialized editor widget | TD H3; UNIFIED_PLAN 5B.2 | Creature-specific data panel (soul, combat style, body parts) composed into `QtFormDialog`. |
| S5 | Add remaining 100 record types to Object Window tree | TD M3; UNIFIED_PLAN 12C | Pull from the real CK's 127 `_Editor.cpp` list; wire each through `QtFormDialogManager`. |

---

## Medium-term (next 2–4 releases)

| # | Goal | Ref | Notes |
|---|------|-----|-------|
| M1 | NavMesh editor | — | `BGSRenderWindowNavmeshEditModule` pattern; triangle paint, link/edge editing, navmesh-to-cell validation. |
| M2 | Scene editor | TD H4; UNIFIED_PLAN 5B.6 | `BGSSceneView` pattern — action list, phase timeline, actor assignment. |
| M3 | AI Package editor | TD H4; UNIFIED_PLAN 5B.15 | PKDT/PLDT/PTDT editing with conditions grid and schedule data. |
| M4 | Worldspace editor widget | TD H5; UNIFIED_PLAN 5B.8 | Map data, climate, water, LOD settings, cell grid. |
| M5 | EffectShader / ImageSpaceModifier editor | TD H4; UNIFIED_PLAN 5B.16 | Shader parameter panels, image-space curve editing. |
| M6 | BA2 archive browser | — | Read/browse Bethesda's BA2 archive format; extract and inspect assets. |
| M7 | Find Forms / Search-Replace dialog | — | `BGSFindFormsDialog` pattern — form-id/name/type search with replace across loaded plugins. |
| M8 | Version control integration | — | `TESVersionControl` pattern — commit/branch/diff of plugin files. |
| M9 | LOD generation pipeline | UNIFIED_PLAN 10.15 (test exists) | Productionize the LOD generator beyond the current round-trip test. |

---

## Long-term (6+ months out)

| # | Goal | Notes |
|---|------|-------|
| L1 | Spaceship editor (Starfield) | Starfield-specific ship builder module. |
| L2 | Galaxy view | Starfield galaxy-level overview panel (currently a stub menu). |
| L3 | Particle editor | Particle system authoring and preview. |
| L4 | Material editor | Material/shader graph editor for Bethesda material formats. |
| L5 | Houdini integration | Side-chain Houdini export/import for procedural world generation. |
| L6 | Reflection probe editing | Place, edit, and bake reflection probes in worldspaces. |
| L7 | Crowd region editing | Crowd/NPC population region authoring. |
| L8 | Morph editor | Head/morph part authoring for face-gen pipelines. |
| L9 | Full multi-game support | Morrowind (TES3), Oblivion (TES4), Skyrim (TES5), Fallout 4, Starfield — record formats and game-specific editors. |
| L10 | Plugin compaction tools | Master/esp compaction, formID renumbering, dependency rewriting. |
| L11 | Snippet import/export (CSV/TSV) | Bulk record snippet exchange via CSV/TSV for spreadsheet workflows. |

---

## Sequencing Notes

- **S1–S5 are prerequisites** for most medium-term work — they close
  the high-severity items in `TECHNICAL_DEBT.md` and unblock real
  editing workflows.
- **M1 (NavMesh)** and **M2 (Scene)** depend on the Render Window
  gizmo system (S2) being in place, since both require on-canvas
  interaction.
- **L9 (multi-game)** is the largest long-term effort and should be
  sequenced after the Starfield editor surface (L1, L2) stabilizes,
  so the game-dispatch layer has concrete cases to generalize from.