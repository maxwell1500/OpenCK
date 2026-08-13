# Changelog

All notable changes to OpenCK are documented here. This project follows
[semantic versioning](https://semver.org). Version numbers come from
`project(openck VERSION ...)` in `CMakeLists.txt`; each release tag matches
`CPACK_PACKAGE_VERSION` so the installer filename aligns with the tag.

## [1.0.0] - 2026-08-12

First release. All tracked closing phases in `docs/REMAINING_WORK_PLAN.md`
are complete (Phases A-G).

### Added
- **ESM/ESP editing** for Morrowind, Oblivion, Skyrim, Fallout 4 and
  Starfield plugin files via the component-property architecture: every
  record exposes its subrecords as typed `EditorProperty` leaves, so the
  form dialog is fully data-driven.
- **FormIdCompactor** (ESL conversion): remaps owned record FormIDs into the
  0x000-0xFFF light-master range, rewriting typed members, the FO4/Starfield
  placed-reference table (XAPR/XLKR/XLRT/XTEL/XLYR/XMSP/XRFG/XMBR/XASP/XEZN/
  XLRL plus Starfield-only XLCN/XTNM/XPCK/XPCS/XLIB/XATR/XNDP/XSAD/XCZR/XCZC/
  TODD/GNAM/HNAM/JNAM/XCOL/XLOC/XVL2/XLMS/XPDO/XPLK) and the Starfield LCTN
  rebuild arrays (ACPR/LCPR/RCPR/ACUR/LCUR/RCUR/ACUN/LCUN/RCUN/ACSR/LCSR/
  RCSR/ACID/LCID/ACEP/LCEP/ACEC/LCEC/RCEC). XPRM treated as FormID-free
  (verified against xEdit `wbDefinitionsSF1` + real `Starfield.esm` dumps).
- **hknp physics** convex-shape block decode and encode (`HknpPhysicsSystem`),
  with an in-memory round-trip test.
- **BA2 archives**: read + write (General/DDS), Starfield mesh BA2 v2 readable.
- **NavMesh auto-gen**: grid-based generator plus NIF-based
  `NavMeshGenerator` with a tuned voxel filter (agentHeight/stepHeight
  headroom model) and an acceptance signal (`NavMesh::cells`).
- **Qt6 UI**: dockable Object Window / Cell View / Render Windows
  (QtAdvancedDocking, LGPL shared lib), Form Dialog with
  Basic / Components / Keywords / Data tabs, FACT/HAZD/REGN editor wizards,
  render preview mesh picker / pivot display / floor grid / camera presets,
  window-layout save/load, export templates for 30 record types, behavior
  graph editor with JSON save/load (`*.ngraph`).
- **Tests menu + headless self-test**: `openck --cli selftest` runs every
  `test_*.exe`, and the app's Tests menu streams the results into a log
  viewer.

### Changed
- QtFormDialog opens records in tabbed form; window-layout actions wired.
- Installer version is derived from the single `project(... VERSION ...)`
  declaration so tag, installer filename, and CPack metadata stay in sync.

### Fixed
- ESL compaction of REFR placed-reference layouts corrected to documented
  xEdit layouts (XAPR/XLRT/XTEL count prefixes removed; XPRD/XCNT/XMBO/XNAM
  entries that are not FormIDs dropped).
- Release CI gate green end-to-end (configure, build, tests, NSIS installer).

### Known limitations
- **SCEN PHDA** (Phase E4): audited against all three shipped games with SCEN
  (Skyrim SE 1706 records, Starfield 7613, Fallout 4 3568) — none contain a
  `PHDA` subrecord; all use the new-generation scene schema. PHDA is a
  classic-format vestige with no live payloads, so no PHDA encoder is
  shipped. Raw-subrecord round-trip is byte-exact.
