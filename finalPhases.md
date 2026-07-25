# OpenCK Final Completion Plan

> 10 Phases · 53 Steps · From infrastructure to polish

---

## Phase 1: Infrastructure & Config Centralization

✅ **3/5 complete** — `FilePaths` utility created, all 9 hardcoded paths replaced. Temp files cleaned, `.gitignore` updated, STATUS.md synced. Remaining: step 1.1 verification.

| Step | Task | Files | Status |
|------|------|-------|--------|
| 1.1 | Create `ConfigPaths` utility — centralize `editor.ini` path in one place | `libs/files/filepaths.hpp` | |
| 1.2 | Replace all 9 hardcoded `applicationDirPath() + "/editor.ini"` references with `ConfigPaths` | `main.cpp`, `shortcutmanager.cpp`, `datatable.cpp`, `datadialog.cpp`, `lootwrapper.cpp`, `mainwindow.cpp`, `preferencesdialog.cpp`, `thememanager.cpp`, `toolbarcustomizationdialog.cpp` | |
| 1.3 | Clean up stale root-level temp files (`temp_*.txt`, `build_*.txt`, `fix_*.py`) | root dir | |
| 1.4 | Update `STATUS.md` to reflect actual project state (currently describes early-stage) | `STATUS.md` | |
| 1.5 | Add/update `.gitignore` entries for temp/build artifacts | `.gitignore` | |

---

## Phase 2: NIF Pipeline — Implement NifPyFileWrapper Stubs

✅ **7/7 complete** — All methods (`saveNif`, `extractShapes`, `extractTextures`, `validateNif`, `compareNifs`, `getPyniflyVersion`) have real implementations. Stderr capture fixed via `SeparateChannels`.

> Currently 5 of 7 public methods return `false` with "not yet implemented" warnings.

| Step | Task | Details | Status |
|------|------|---------|--------|
| 2.1 | Implement `saveNif()` | Write modified NIF back to disk via PyNifly script + QProcess | |
| 2.2 | Implement `extractShapes()` | Parse NIF and return mesh/node names via Python extraction script | |
| 2.3 | Implement `extractTextures()` | Extract texture paths from NIF materials via Python script | |
| 2.4 | Implement `validateNif()` | Run PyNifly validation checks, return error list | |
| 2.5 | Implement `compareNifs()` | Structural comparison of two NIFs, return diff report | |
| 2.6 | Fix `getPyniflyVersion()` | Query actual installed version via `pip show` or Python import instead of hardcoded `"1.0.0"` | |
| 2.7 | Fix stderr capture in `loadNif()` | `MergedChannels` makes `readAllStandardError()` empty — use `SeparateChannels` | |

---

## Phase 3: Blender Integration — Implement BlenderLauncher Stubs

✅ **9/9 complete** — All methods implemented. `isNifCompatibleVersion()` updated to require Blender 4.4+. Missing `installNifToolsAddon()`/`checkNifToolsInstallation()` stubs filled in.

> 8 of 16 public/private functions have no implementation. Linker errors if called.

| Step | Task | Details | Status |
|------|------|---------|--------|
| 3.1 | Implement `findNifAddonInBlender()` | Shared helper — run `blender --background --python-expr` to query installed addons | |
| 3.2 | Implement `detectPyNiflyInstallation()` | Scan Blender addons for PyNifly using 3.1 | |
| 3.3 | Implement `detectNifToolsInstallation()` | Scan Blender addons for NifTools using 3.1 | |
| 3.4 | Implement `exportNifViaBlender()` | Headless Blender NIF export, return `NifExportResult` | |
| 3.5 | Implement `importNifViaBlender()` | Headless Blender NIF import | |
| 3.6 | Implement `generateNifPreview()` | Multi-angle preview generation (reuse `preview_generator.py`) | |
| 3.7 | Implement `batchExportModels()` | Iterate plugin records with modelPath, export all | |
| 3.8 | Implement `validateNifFile()` | Load NIF via PyNifly, check for structural errors | |
| 3.9 | Fix `isNifCompatibleVersion()` | Correct version comparison to match header docs (Blender 4.4+ with PyNifly) | |

---

## Phase 4: ObjectWindow Refactoring

✅ **5/5 complete** — `getModelPathForRecord()` helper extracted. `openInBlender`, `previewNif`, `compareNifs` refactored to ~30–50 lines each. Dialogs use `BlenderLauncher::getRecommendedBlenderPath()`.

> `objectwindowdialog.cpp` has 3 near-identical 10-case switches (~200 lines of duplication).

| Step | Task | Details | Status |
|------|------|---------|--------|
| 4.1 | Extract `getModelPathForRecord(Data*, int formId)` helper | Single function that returns the modelPath for any record type | |
| 4.2 | Refactor `openInBlender()` to use helper | ~100 lines → ~10 lines | |
| 4.3 | Refactor `previewNif()` to use helper | ~80 lines → ~10 lines | |
| 4.4 | Refactor `compareNifs()` to use helper | ~80 lines → ~10 lines | |
| 4.5 | Wire NifPreviewDialog/NifComparisonDialog to use `BlenderLauncher::getRecommendedBlenderPath()` | Replace hardcoded `"blender"` string | |

---

## Phase 5: Error Handling & Robustness

✅ **7/8 complete** — All individual fixes applied (null checks, DDS/WAV validation, division-by-zero, profile parsing, exit code checking in comparison dialog, orphaned signal removed). `fieldvalidators.hpp` created and deployed to 7 editors; full ~50-editor coverage remains as future work.

> Multiple missing null checks, truncation guards, and logic bugs found in audit.

| Step | Task | File:Line | Status |
|------|------|-----------|--------|
| 5.1 | Add null-check for `qobject_cast` result | `preferencesdialog.cpp:159` | |
| 5.2 | Add DDS header truncation checks — validate bytes read from file | `assetvalidator.cpp:216-237` | |
| 5.3 | Add WAV `fmt` chunk size validation — guard against truncated files | `assetconverter.cpp:484-508` | |
| 5.4 | Fix division-by-zero when `totalSize == 0` | `cloudsave.cpp:240` | |
| 5.5 | Fix profile parsing logic — `\|\| !key.isEmpty()` makes first condition dead | `modmanagerdetection.cpp:131` | |
| 5.6 | Add Blender process exit code checking in preview dialogs | `nifpreviewdialog.cpp`, `nifcomparisondialog.cpp` | |
| 5.7 | Remove orphaned `themeChanged()` signal connection (never connected) | `preferencesdialog.hpp:18` | |
| 5.8 | Add field range validation to all editors — prevent corrupting ESM files | All editor dialogs | |

---

## Phase 6: 3D Viewport Enhancements

✅ **6/6 complete** — Already implemented. Viewport has interleaved position+normal+UV+color vertex format, Phong lighting (ambient+diffuse+specular+emission), texture sampling, and VAO batching. NIF parser reads custom format with full vertex data.

| Step | Task | Details | Status |
|------|------|---------|--------|
| 6.1 | Implement NIF version handling | Parse version string, route to correct parser branch | |
| 6.2 | Add normals extraction to NIF parser | Read `NiNormal` data from vertex data | |
| 6.3 | Add UV coordinate extraction | Read `NiTexCoord` data | |
| 6.4 | Add texture coordinate mapping | Apply UVs to mesh for texture rendering | |
| 6.5 | Improve lighting model | Use `NiMaterialProperty` for ambient/diffuse/specular | |
| 6.6 | Add mesh batching / VAO optimization | Batch draw calls for complex scenes | |

---

## Phase 7: Editor Completions

> Several editors have placeholder UI instead of functional 3D previews.

| Step | Task | Details | Status |
|------|------|---------|--------|
| 7.1 | Spell Editor 3D preview | Replace placeholder label with `NifViewportWidget`, load spell effect NIF | |
| 7.2 | Enchantment Editor 3D preview | Replace placeholder with `NifViewportWidget` | |
| 7.3 | Landscape heightmap persistence | Save heightmap to `CellRecord` data on close | |
| 7.4 | Landscape brush repaint | Connect brush tool to viewport `update()` signal | |
| 7.5 | Landscape height limit slider | Implement real clamping based on slider value | |
| 7.6 | Object palette from game data | Replace hardcoded object list with actual record query | |
| 7.7 | Object placement persistence | Serialize cell references to ESM on save | |

---

## Phase 8: Papyrus & Dialogue Completion

> Compiler missing statement types. Dialogue editor has incomplete condition editing.

| Step | Task | Details | Status |
|------|------|---------|--------|
| 8.1 | Papyrus: implement if/else/elif statements | Add AST nodes and codegen | |
| 8.2 | Papyrus: implement while/for loops | Add loop codegen | |
| 8.3 | Papyrus: add type checking | Validate variable types at compile time | |
| 8.4 | Dialogue: implement conditional response editing | Quest stage conditions, variable checks | |
| 8.5 | Dialogue: add voice file association | Link dialogue lines to `.wav` files | |
| 8.6 | Quest graph: implement quest stage editing | Stage flags, indices, objectives | |

---

## Phase 9: Testing

> 11 tests exist. Zero coverage for NIF parser, BlenderLauncher, ShortcutManager, ThemeManager, editors.

| Step | Task | New Test File | Status |
|------|------|---------------|--------|
| 9.1 | Unit test for `ShortcutManager` | `tests/test_shortcutmanager.cpp` — init, get/set, save/load, resetToDefaults | |
| 9.2 | Unit test for `ThemeManager` | `tests/test_thememanager.cpp` — themeFromName, applyTheme, round-trip | |
| 9.3 | Unit test for `NifPyFileWrapper` | `tests/test_nifpyfilewrapper.cpp` — initialize, loadNif, error cases | |
| 9.4 | Unit test for `BlenderLauncher` | `tests/test_blenderlauncher.cpp` — findBlender, version parsing | |
| 9.5 | Unit test for modelPath helper | `tests/test_objectwindow.cpp` — all 10 record types | |
| 9.6 | Unit test for config centralization | `tests/test_configpaths.cpp` — read/write round-trip | |
| 9.7 | Integration test | `tests/test_nifintegration.cpp` — full NIF import→edit→export cycle | |

---

## Phase 10: Documentation & Final Polish

| Step | Task | Details | Status |
|------|------|---------|--------|
| 10.1 | Update `STATUS.md` | Reflect accurate current state across all phases | |
| 10.2 | Update `TECHNICAL_DEBT.md` | Mark resolved items (P4-01..04, P5-01..06, etc.) | |
| 10.3 | Update `ROADMAP.md` | Reflect completion milestones | |
| 10.4 | Update `IMPLEMENTATION_PLAN.md` | Remove stale entries, sync with actual code | |
| 10.5 | Add API doc comments to public interfaces | Key classes: Data, NifPyFileWrapper, BlenderLauncher, ShortcutManager | |
| 10.6 | Final build + test pass verification | `cmake --build build_ninja && ctest --test-dir build_ninja` — 10/10 green | |

---

## Progress Tracker

| Phase | Steps | Complete | Status |
|-------|-------|----------|--------|
| 1 — Infrastructure | 5 | 3/5 | Temp files cleaned, .gitignore updated, STATUS.md synced |
| 2 — NIF Pipeline | 7 | 7/7 | All methods implemented |
| 3 — Blender Integration | 9 | 9/9 | All methods implemented, version check fixed |
| 4 — ObjectWindow Refactor | 5 | 5/5 | Helper extracted, 3 methods refactored |
| 5 — Error Handling | 8 | 7/8 | Applied except field validators for all editors |
| 6 — 3D Viewport | 6 | 6/6 | Already implemented (Phong lighting, normals, UVs, textures, VAOs) |
| 7 — Editor Completions | 7 | 0/7 | |
| 8 — Papyrus & Dialogue | 6 | 0/6 | |
| 9 — Testing | 7 | 0/7 | |
| 10 — Documentation | 6 | 0/6 | |
| **TOTAL** | **66** | **~34** | |
