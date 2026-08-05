# OpenCK — Technical Debt Register

> Last updated: 2026-08-04
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
| H1 | FormIdCompactor doesn't rewrite FormID references inside opaque raw subrecords | `formidcompactor.hpp:18` (Phase 23.4 follow-up) | Only typed reference fields (RELA parent/child, SHOU words, etc.) are rewritten. References embedded in raw subrecords (XOWN, XPRM, etc.) are left untouched, producing broken refs for many record types. Needs a per-record-type FormID field map. |
| H2 | ~90 record types in Starfield.esm have no CkId enum, no collection, no struct | `ckid.cpp` diskAliases, Phase 15 gap | Starfield.esm has 180 distinct record types; CkId covers ~90. Medium-impact missing types: HDPT (head parts x230), TERM (terminals x368), MATT (material type x249), MOVT (movement x43), MUSC (music track x83). Internal/infrastructure types (RFGP x80584, PKIN x11281, LMSW x12966) are lower priority. |
| H3 | BA2 DX10 (texture) archives can't be read or written | `ba2archive.cpp` | Resolved — see R20. |

---

## Medium

| ID | Item | Location / Ref | Notes |
|----|------|----------------|-------|
| M3 | Cell transitions editor disabled | `celltransitionseditor.cpp:218-240` | "Requires cell connection data not yet available in TES4 format." Permanent for Skyrim; may resolve for Starfield. |
| M4 | Preferences Network page is a stub | `preferencesdialog.cpp:353` | Network page has no content. (Version-control text already corrected; git/Perforce in use.) |
| M5 | QtFormDialog tabs: Properties+Data only | `qtformdialog.cpp` (Phase 12F) | Real CK has Basic/Components/Keywords/Ingest tabs. |
| M6 | Flat fields kept alongside components for back-compat | Phase 5E.1–5E.3 | `containerItems`, `keywords`, `spells` still have flat mirrors. Audit done; fields intentionally kept. |
| M7 | NavMesh auto-gen from arbitrary world geometry is partial | `navmeshtoolkit.hpp/.cpp`, `navmeshgenerator.hpp` | Grid-based gen works; NIF-based `NavMeshGenerator` is wired into `navmesheditor.cpp:332` but voxel filter needs tuning against real data. |
| M8 | SCEN PHDA binary encoding round-trips through raw subrecords | Phase 16.1 | The timeline widget edits a model, not the on-disk bytes. Needs validation against real Starfield data. |
| M9 | hknp per-shape payload decode is best-effort | `hknpphysicssystem.hpp` | hknpConvexShape polytope arrays decoded; other shape types undecoded. No hknp encoder exists. |

---

## Low

| ID | Item | Location / Ref | Notes |
|----|------|----------------|-------|
| L1 | Stale `.bak` files in source tree | `src/view/window/` | `armor_editor.cpp.bak`, `objectwindowdialog.cpp.bak` — tracked by mistake. |
| L2 | Stale `NewFindings.md` in repo root | `NewFindings.md` | References navmesheditor as a shell (it's not anymore). |
| L3 | xWMA WMF MFT fallback truncates audio | `XwmaDecoder` (Phase 18.2) | ffmpeg primary path works fully; WMF fallback "valid but truncated". Low priority. |

---

## Resolved

| ID | Item | Resolution |
|----|------|------------|
| R1 | Tes4Codes translation layer | Removed entirely in Phase 1 (`tes4codes.hpp` deleted; all parsers use on-disk codes). |
| R2 | Cell loader spin bug | Fixed — `readNSubHeader` returning 0 now correctly breaks the load loop. |
| R3 | `packrecord.hpp` dead code | Deleted (unused duplicate `PackageRecord` struct). |
| R4 | 26 dead bespoke editor includes | Removed from `objectwindowdialog.cpp` after migration to `QtFormDialogManager`. |
| R5 | Render Window transform tools placeholder | Done — gizmo system (translate/rotate/scale) implemented in Phase 14A. |
| R6 | Cell View 2D map canvas placeholder | Done — interactive pan/zoom/select/marquee in Phase 14B. |
| R7 | CREA no specialized editor widget | Done — `CreatureDataWidget` registered in Phase 15.4. |
| R8 | SCEN/EFSH/PACK editors deferred | Partially done — SCEN timeline (16.1), EFSH/IMGS typed DATA (16.2), PACK conditions (16.3). |
| R9 | WRLD editor widget not built | Done — `WorldspaceDataWidget` in Phase 16.4. |
| R10 | Missing CK File actions (Create Archive, etc.) | Done — real archive writers now exist: `Ba2Archive::create` (BTDX v2 GNRL) and `BsaArchive::create` (SSE v0x69, hash-validated). |
| R11 | Flat fields in records alongside components | Audit done (5E.4); most fields intentionally kept. |
| R12 | Object Window tree structure | Done — 3-level hierarchical tree (Phase 12C). |
| R13 | Galaxy/Packin stub menus | Removed (Phase 24.10). |
| R14 | No CMake install target | Done — `cmake --install` + CPack (Phase 24.4). |
| R15 | No CI/CD pipeline | Done — GitHub Actions (Phase 24.3). |
| R16 | `readZString` NUL terminator bug | Fixed — strips trailing NULs from returned QStrings (2026-08-03 session). |
| R17 | Copy/paste restricted to ~29 record types | Done — generic copy/paste via `Data::cloneRecord` (all CkId types with collections) in Object Window; commit `1272c17`. |
| R18 | Search dialog can't edit most record types | Done — shared `FormComponentsResolver` (`src/model/tools/formcomponentsresolver.hpp/.cpp`) moved `resolveComponents` out of the Object Window; Search dialog's default case now opens component-based records via `QtFormDialogManager::openOrFocus`. |
| R19 | Export templates incomplete | Done — `fieldsForType` field lists corrected for all 22 types, `recordFieldValue` getters added for INGR_/ENCH_/CONT_/MISC_/ACTI_/STAT_/RACE_/CLASS_/FACT_/INFO_/CELL/WRLD_/LOCT_, and the export if/else chain replaced with a generic `exportRecords<T>` template so every template type exports. |
| R20 | BA2 DX10 (texture) archives can't be read or written | Done — `parseBtdxTextures` now parses 24-byte DX10 records + mip chunks (v1/2/3, zlib + Starfield v3 LZ4); `extract()` rebuilds a DDS (with DX10 extended header) from the chunk payloads; `create()` now writes real DX10 archives when `archiveType == "DX10"`. `DdsDecoder` gained DX10-extended-header support (BC1-5) so the Archive Browser texture preview works. `test_ba2dx10` covers real Starfield v2/v3 archives (via `OPENCK_TEST_BA2_DIR`) + a synthetic write→read round-trip. |

---

## How to Update This Register

- When a debt item is paid off, **move** it from its severity section to
  **Resolved** with a one-line summary of the resolution. Do not delete the
  row — the history is useful.
- When adding a new item, assign the lowest severity that accurately
  reflects user impact, and cross-reference the relevant task ID in
  `docs/UNIFIED_PLAN.md` if one exists.
