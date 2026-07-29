# OpenCK — Technical Debt Register

> Last updated: 2026-07-28
> A living register of all known technical debt items, organized by severity.
> Source cross-references point to tasks in `docs/UNIFIED_PLAN.md`.

## Severity Legend

| Severity | Meaning |
|----------|---------|
| **High** | Blocks a core user-facing workflow or architectural goal. Fix soon. |
| **Medium** | Functional gap or code-smell with workarounds. Fix when touching the area. |
| **Low** | Cosmetic, hygiene, or tooling issue. Fix opportunistically. |
| **Resolved** | Was debt; now paid off. Kept for history. |

---

## High

| ID | Item | Location / Ref | Notes |
|----|------|----------------|-------|
| H1 | Render Window transform tools (Select/Move/Rotate/Scale) are placeholder toolbar buttons with no gizmo implementation | `src/view/widgets/nifviewportwidget.cpp:384-476` (12G.1–12G.3) | The core edit-module work (`BGSRenderWindowEditModule` pattern) is deferred. Transform mode is a placeholder enum. |
| H2 | Cell View 2D map canvas is a basic `paintEvent` placeholder — no interactive selection, no zoom/pan | `src/view/panels/cellsdialog.cpp` (12B.5) | Markers render but the user cannot click-select references or navigate the map. |
| H3 | CREA (Creature) record type has components but no specialized editor widget | 5B.2 (◐) | Falls back to the generic property grid; creature-specific data (soul, combat style, body parts) is not editable. |
| H4 | SCEN (Scene), EFSH (EffectShader), PACK (AI Package) editors deferred | 5B.6, 5B.15, 5B.16 (⬜) | No UI for scene timelines, shader parameters, or AI package editing. |
| H5 | WRLD (Worldspace) editor widget not built (has components) | 5B.8 (◐) | Worldspace-specific subrecords (WNAM, XNAM, MNAM, CNAM, NAM0–NAM9) have no dedicated widget. |
| H6 | 12D.13: Missing CK File-menu actions (Create Archive, Compile Papyrus Scripts, Compact Master) — stub menus only | `ui/mainwindow.ui`, `mainwindow.cpp` (12D.13 ⬜) | Menu entries exist; no backing implementations. |

---

## Medium

| ID | Item | Location / Ref | Notes |
|----|------|----------------|-------|
| M1 | Flat fields in records kept alongside components for back-compat | 5E.1–5E.3 | `containerItems`, `keywords`, `spells` still have flat mirrors read by `data.cpp`/exporters. Audit (5E.4) is done; most fields intentionally kept. |
| M2 | Papyrus type checker may have incomplete coverage for array types and struct properties | Phase 9 (9.3) | Basic type checking works; complex composite types may not be fully validated. |
| M3 | Object Window tree has 7 groups covering 27 record types — real CK has 127 record types | `src/view/windows/objectwindow.cpp` (12C) | ~100 record types not yet exposed in the Object Window tree. |
| M4 | Preferences Network page is a disabled stub | `preferencesdialog.cpp` (12E.2) | Page exists in the tree sidebar but has no functional content. |
| M5 | Galaxy / Packin / Theme / Tests menus are empty stubs | `ui/mainwindow.ui` (12D.8) | Top-level menus added for parity but contain no actions. |
| M6 | QtFormDialog tabs are Properties+Data only (real CK has Basic/Components/Keywords/Ingest tabs) | `src/view/window/qtformdialog.cpp` (12F.3) | Tab structure exists but does not yet split into the real CK's full tab set. |

---

## Low

| ID | Item | Location / Ref | Notes |
|----|------|----------------|-------|
| L1 | CRLF/LF line ending warnings on git commit (Windows checkout) | repo-wide | Cosmetic; git `autocrlf` configuration issue. |
| L2 | `objectwindow.cpp.bak` backup file exists in source tree | `src/view/windows/` | Should be deleted; tracked by mistake. |
| L3 | No CMake install target for end-user deployment | `CMakeLists.txt` | No `install()` rules; users build from source. |
| L4 | `windeployqt` post-build step may not copy the ADS DLL | `CMakeLists.txt` | `qtadvanceddocking-qt6.dll` may need manual placement next to `openck.exe`. |
| L5 | No CI/CD pipeline | — | No automated build/test/run on push. |

---

## Resolved

| ID | Item | Resolution |
|----|------|------------|
| R1 | Tes4Codes translation layer | Removed entirely in Phase 1 (`tes4codes.hpp` deleted; all parsers use on-disk codes). |
| R2 | Cell loader spin bug | Fixed — `readNSubHeader` returning 0 now correctly breaks the load loop. |
| R3 | `packrecord.hpp` dead code | Deleted (unused duplicate `PackageRecord` struct). |
| R4 | 26 dead bespoke editor includes | Removed from `objectwindowdialog.cpp` after migration to `QtFormDialogManager`. |

---

## How to Update This Register

- When a debt item is paid off, **move** it from its severity section to
  **Resolved** with a one-line summary of the resolution. Do not delete
  the row — the history is useful.
- When adding a new item, assign the lowest severity that accurately
  reflects user impact, and cross-reference the relevant task ID in
  `docs/UNIFIED_PLAN.md` if one exists.