# OpenCK Development Roadmap

This file tracks the development roadmap and current status of OpenCK, an open-source replacement for Bethesda's Creation Kit.

## Current Status

OpenCK has a working foundation for multi-game support (Skyrim LE/SE/AE, Oblivion, Morrowind, Starfield) with:
- ESM binary reader/writer (complete, handles headers, subrecords, strings)
- 30 record parsers implemented with complete load/save cycles
- 4 static libraries with layered architecture (`openck_esm` → `openck_files` → `openck_model` → `openck_view`)
- **18 unit tests** passing across test executables
- IdCollection/IdTable framework with undo support
- Game detection (registry + common paths + XboxGames)
- Modernized CMake build system (Qt 6.7, C++17, MSVC)
- 3D viewport with OpenGL rendering, normals, UVs, lighting
- Landscape editor with brush tools and heightmap persistence
- Object palette with REFR format placement
- Batch Export UI (Record Data + Asset Conversion)
- Theme management (Dark/Light/System)
- Shortcut and toolbar customization

**All 30 record types have complete parsers, column definitions, and editor dialogs.**

## Project Vision

Create a fully functional, multi-game Creation Kit replacement that supports editing, creating, and managing plugins for Skyrim (LE/SE/AE), Fallout 4, Starfield, Oblivion, Morrowind, and New Vegas — with full record editing, conflict detection, and mod management capabilities.

---

## Completed Phases

### Phase 1: Infrastructure & Config Centralization ✅
- All 30 record types wired into Data::continueLoading() and Document::save()
- Collection containers for all record types
- Record type NAMEs (4CC) mapped to CkId::Type enum
- FilePaths::configFilePath() centralizes editor.ini path
- All hardcoded references replaced

### Phase 2: Record Editing & Editor Integration ✅
- All 30 collection types have proper column definitions
- All 30 record types have working editor dialogs in ObjectWindowDialog
- FactEditor for FACT_, MaterialEditor for MATE_
- Undo/redo integration via EditRecordCommand pattern
- Column-level validation via ColumnValidator

### Phase 3: Advanced Editing Features ✅
- 3D viewport with OpenGL, normals, UVs, material lighting
- Landscape editor with brush tools, heightmap persistence
- Cell/worldspace navigation
- Quest graph visual editor
- Dialogue tree editor
- Papyrus compiler with error parsing
- Spell/Enchantment 3D previews
- Object palette with game data integration
- Batch Export UI

### Phase 4: Plugin Management ✅
- Plugin load order management
- LOOT integration
- Plugin merge tool
- Bashed Patch generation
- Conflict detection and resolution
- Mod manager detection
- Cloud save support

### Phase 5: NIF Pipeline & Blender Integration ✅
- NifPyFileWrapper with saveNif, extractShapes, extractTextures, validateNif, compareNifs
- BlenderLauncher with findBlender, export/import via headless mode
- Preview generation, batch export, NIF validation
- Version compatibility checking (>= 2.93)

### Phase 6: ObjectWindow Refactoring ✅
- Extracted getModelPathForRecord() helper
- Refactored openInBlender, previewNif, compareNifs (~260 lines → ~30 lines)
- Wired dialogs to use BlenderLauncher::getRecommendedBlenderPath()

### Phase 7: Error Handling & Robustness ✅ (7/8)
- Null checks, truncation guards, division-by-zero guards
- Profile parsing fix, orphaned signal removal
- Field range validation deferred (too broad)

### Phase 8: Testing ✅
- 18 test executables covering core functionality
- Plugin I/O, undo stack, search, conflicts, export/import
- ThemeManager, NifPyFileWrapper, BlenderLauncher, ObjectWindow modelPath
- NIF integration tests

---

## Remaining Work

### Phase 14: Render Window Gizmos + Interactive Cell View (complete)

- **14A — Gizmo system**: translate/rotate/scale manipulators via OverlayVBO, axis-handle pick + drag, snap-to-grid/angle, undoable REFR writes via EditRecordCommand, Q/W/E/R keys, selection highlight. 13/13 tasks.
- **14B — Interactive Cell View**: pan/zoom view transform, click-select + hover + marquee, table sync, Inspector + Render Window cross-wiring, status bar coords. 10/10 tasks.
- **New**: `gizmomath.hpp/.cpp`, `cellmapview.hpp/.cpp`, `test_gizmomath` (34 checks), `test_cellviewcanvas` (21 checks).

### Phase 15: Record Coverage & Object Window Completion (complete)

- **15.1 ✅**: 44 orphaned record structs wired end-to-end — CkId enums, Data collections/getters/ctor columns, continueLoading routing, getCollectionByType, allCollections (fixed missing SOUN/WTHR/LTEX), allCollectionsWithTypes, Document::save. 28 dead categories re-typed + 16 new categories → 62/88 backed. Fixed Type_None category paint crash.
- **15.2 ✅**: 44 orphan loaders compiled for the first time + fixed.
- **15.4 ✅**: CREA `CreatureDataWidget` + factory; generic component-dialog fallback in editSelected; Inspector generic fallback.
- **15.5 ✅**: ObjectWindow `recordSelected` → `mStatusSelectedObject`; activated `updateRecordCount`/`updatePluginInfo` in setData.
- **15.6 ✅**: `WarningsDockWidget` class + generic `CoverageValidator` (empty/duplicate EditorID across all collections, via new const `allCollectionsWithTypes`); runValidation populates dock.
- **15.7 ✅**: `runValidation` non-modal → dock + status bar count (no QMessageBox).
- **15.3 ⏸**: `.filter` file support deferred (no real filter files to test against).

### Phase 16: Specialized Editor Completion (in progress)

- **16.3 ✅ (partial)**: PACK `PackDataWidget` (EditorID, package/target type, flags, target list) + `"PACK"` factory + editSelected case.
- **16.4 ✅ (partial)**: WRLD `WorldspaceDataWidget` (name, water/climate/lighting/music/terrain refs) + `"WRLD"` factory + editSelected case.
- **16.5 ✅ (partial)**: LCTN `LocationDataWidget` (name, parent, X/Y/Z) + `"LCTN"` factory + editSelected case.
- **16.8 ✅ (partial)**: NAVM `editSelected` case → `NavmeshEditorDialog` seeded from `NavmRecord` + undoable write-back; fixed `NavmRecord` NVTR flag serialization; +`test_navmrecord` round-trip.
- **16.1 ✅ (partial)**: `ScenRecord` struct created + wired end-to-end; "Scene" Object Window category backed; generic dialog with raw-subrecord inspector. Timeline editor pending.
- **16.2 ✅ (partial)**: EFSH/IMGS raw-subrecord inspector in the generic dialog (unparsed subrecords visible losslessly).
- **16.6-16.7**: PNDT/CCT Starfield editors (pending)
- **16.8**: NavMesh record binding (pending)

### High Priority

**P5-08: Landscape undo/redo**
- Integrate terrain brush operations with UndoStack
- Estimated: 1-2 days

**P6-01: PapyrusCompiler path validation**
- Verify pp64.exe is valid before use
- Estimated: 1 day

**P6-02: Papyrus dependency resolution**
- Batch compilation with dependency graph
- Estimated: 1 week

### Medium Priority

**P5-09: Object palette search filtering**
- Implement QSortFilterProxyModel for real-time filtering
- Estimated: 1 day

**X-02: Field range validation**
- Add bounds checking to all editor fields
- Estimated: 1-2 weeks (too broad for single pass)

**X-08: Localization support**
- Wrap UI strings in tr() calls
- Estimated: Ongoing

### Low Priority

**P5-10: Grid snap for object placement**
- Add grid size option and snap checkbox
- Estimated: 2-3 days

**P7-04: Remove legacy/backup files**
- Cleanup .bak and .corrupt files
- Estimated: 1 hour

---

## Record Type Specifications

Each record type follows the same loading/saving pattern:
1. **Header**: RecHeader (size, flags, id, version)
2. **Data subrecords**: Type-specific fields (EDID, FNAM, etc.)
3. **Model subrecords**: MODL, MODB, MODT (optional)
4. **Script subrecords**: SNAM (optional)
5. **Save support**: Write all subrecords back in order

## Technical Notes

### Skyrim Record Format
- Records: NAME (4CC), SIZE (u32), DATA (SIZE bytes)
- Some records have EDID (4CC "EDID") as first subrecord
- Data subrecords vary by record type
- Model subrecords (MODL, MODB, MODT) are optional

### ESM File Format
- TES4 header with HEDR, MAST, INTV, INCC subrecords
- Record list follows header
- Each record has flags, ID, version, and data
- Master files vs plugin files (Master flag)

### Version Detection
- Skyrim LE: HEDR 0.94-1.4, INCC 0-16
- Skyrim SE: HEDR 1.5-1.5.97, INCC 17
- Skyrim AE: HEDR 1.6+, INCC 17
- Oblivion: HEDR 4.x
- Morrowind: HEDR 3.x

## Contributing

Contributions are welcome! Focus areas:
1. Record parser implementations for additional games
2. UI/UX improvements
3. Bug fixes
4. Documentation
5. Testing and code quality

## License

OpenCK is released under the GNU General Public License v3.0. See the [LICENSE](LICENSE) file for details.

---
*Last updated: August 1, 2026*

## Recent Changes (July 2026)
- **Phase 7 complete**: Landscape heightmap persistence, ObjectPalette REFR format
- **Phase 8 complete**: Batch Export UI with Record Data and Asset Conversion tabs
- **Phase 9 complete**: 18 tests building (ThemeManager, NifPyFileWrapper, BlenderLauncher, ObjectWindow, NIF integration)
- **Documentation updated**: STATUS.md, TECHNICAL_DEBT.md, finalPhases.md reflect current state
- **Progress**: 282/310 steps complete (91%) — Phases 14, 15, 20, 24 done. 16 (7/8), 17 (8/9), 18 (3/7), 19 (5/7), 21 (7/8), 23 (8/9) in progress
- **Phase 21 (partial)**: Papyrus LSP client (JSON-RPC transport + editor wiring). Build clean, 68/68 tests.
