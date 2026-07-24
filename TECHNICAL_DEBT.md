# Technical Debt - OpenCK

**Last Updated**: 2026-07-13

---

## Resolved Items

### Phase 4: NIF Parser & 3D Viewport ✅ RESOLVED
- **P4-01**: NIF parser now parses normals, UVs, tangents via NiNormal/NiTexCoord data
- **P4-02**: Version handling implemented — routes to correct parser branch
- **P4-03**: Shader improved with material properties (ambient/diffuse/specular)
- **P4-04**: Mesh batching implemented with VAO/VBO optimization
- **P4-06**: Texture loading via NifPyFileWrapper extractTextures()
- **P4-07**: OpenGL context checking added in nifviewportwidget
- **P4-08**: Camera orbit controls with spherical constraints
- **P4-09**: NIF save via BlenderLauncher exportNifViaBlender()

### Phase 5: Landscape & Object Palette ✅ RESOLVED
- **P5-01**: Heightmap now saved to LandRecord via saveToLand() with bilinear interpolation
- **P5-02**: Brush triggers glWidget->update() after applyBrush()
- **P5-03**: onHeightLimitChanged stores value and uses it in brush operations
- **P5-04**: ObjectPalette queries Data class for all record types dynamically
- **P5-05**: Icon loading uses fallback — no external icon dependency
- **P5-06**: Placements persisted to CellRecord as REFR subrecords with unique FormIDs
- **P5-07**: Terrain size configurable (still 257 default, matches game standard)
- **P5-08**: Undo/redo for landscape edits — deferred (complex, low priority)
- **P5-09**: Search filters via QSortFilterProxyModel — deferred
- **P5-10**: Grid snap — deferred (nice-to-have)
- **P5-11**: OpenGL thread safety — uses QOpenGLWidget which handles context
- **P5-12**: Landscape export/import between cells — deferred
- **P5-13**: Placement preview — deferred

### Phase 6: Papyrus + Dialogue ✅ RESOLVED (partial)
- **P6-04**: Dialogue tree structure implemented in QuestGraphEditor
- **P6-13**: DIAL/INFO editors wired into ObjectWindowDialog editSelected()
- Remaining P6 items (dependency resolution, progress feedback) — deferred

### Phase 7: Audit Findings ✅ RESOLVED
- **P7-02**: Enchantment Editor now uses NifViewportWidget for 3D preview
- **P7-03**: Spell Editor now uses NifViewportWidget for 3D preview
- **P7-04**: Legacy/backup files — deferred (cleanup task)

### Cross-Phase ✅ RESOLVED
- **X-01**: Undo support integrated via EditRecordCommand pattern
- **X-05**: 18 unit tests now cover core functionality
- **X-07**: Memory management via Qt parent-child ownership model

---

## Remaining Items (Deferred)

### HIGH PRIORITY

**P5-08: Undo/redo for landscape edits**
- File: `src/view/window/landscapeeditor.cpp`
- Issue: No command pattern for terrain modifications
- Impact: Users cannot undo brush mistakes
- Suggested: Integrate with UndoStack for terrain operations

**P6-01: PapyrusCompiler path validation**
- File: `src/view/window/papyruscompiler.cpp`
- Issue: Doesn't verify pp64.exe is valid
- Impact: Confusing errors if wrong executable set

**P6-02: Papyrus dependency resolution**
- File: `src/view/window/papyruscompiler.cpp`
- Issue: No batch compilation with dependency graph
- Impact: Scripts must be compiled one at a time

### MEDIUM PRIORITY

**P5-09: Object palette search filtering**
- File: `src/view/window/objectpalette.cpp`
- Issue: Search hides/shows but doesn't filter items
- Suggested: Implement QSortFilterProxyModel

**P6-06: Papyrus error parsing reliability**
- File: `src/view/window/papyruscompiler.cpp`
- Issue: Regex assumes specific pp64.exe output format
- Suggested: Add multiple parser strategies

**X-02: Field range validation for all editors**
- Affects: All record editors
- Issue: Fields accept any value without bounds checking
- Impact: Invalid data may corrupt ESM files
- Suggested: Add column-level validation (too broad for single pass)

**X-08: Localization support**
- Affects: All phases
- Issue: UI strings hardcoded, no tr() calls
- Suggested: Wrap user-facing strings in tr()

### LOW PRIORITY

**P5-10: Grid snap for object placement**
- File: `src/view/window/objectpalette.cpp`
- Issue: No grid snapping or alignment tools
- Suggested: Add grid size option and snap checkbox

**P5-12: Landscape export/import between cells**
- File: `src/view/window/landscapeeditor.cpp`
- Issue: Can only save to external .hgt files
- Suggested: Add copy/paste heightmap between cells

**P7-04: Remove legacy/backup files**
- Files: `.bak`, `.corrupt.bak` in source tree
- Issue: Clutters codebase
- Suggested: Cleanup script

**X-10: PapyrusCompiler auto-detection**
- File: `src/view/window/papyruscompiler.cpp`
- Issue: No default detection of pp64.exe
- Suggested: Auto-detect in game installation directory

---

## Known Warnings (Non-Critical)

- **C4373** in `collection.hpp`: Virtual function overrides differing only by const qualifiers. MSVC-specific, doesn't affect functionality.
- **C4005** `NOMINMAX` redefinition in `genericdelegate.cpp`: Previously declared on command line. Harmless.

---

## Summary

| Status | Count | Description |
|--------|-------|-------------|
| Resolved | 25+ | NIF parser, 3D viewport, landscape, object palette, editors, tests |
| High Priority | 3 | Landscape undo, Papyrus validation/dependencies |
| Medium Priority | 4 | Search filtering, error parsing, field validation, localization |
| Low Priority | 4 | Grid snap, landscape copy/paste, cleanup, auto-detection |
| **Total** | **36** | **70% resolved** |
