# OpenCK Project Status

**Last Updated**: 2026-07-24
**Current Progress**: ~28/66 steps (42%) across 10 phases
**License**: GNU General Public License v3.0

---

## Project Overview

OpenCK is an open-source replacement for Bethesda Game Studios' Creation Kit, written in C++17/Qt6. It provides comprehensive editing capabilities for Bethesda plugin files (ESM/ESP) with a modern, extensible architecture.

---

## Phase Completion Status

| Phase | Steps | Complete | Status |
|-------|-------|----------|--------|
| 1 — Infrastructure & Config Centralization | 5 | 2/5 | Partially applied (temp files cleaned, .gitignore updated, STATUS.md updated) |
| 2 — NIF Pipeline — NifPyFileWrapper | 7 | 7/7 | Fully applied — all methods implemented |
| 3 — Blender Integration — BlenderLauncher | 9 | 9/9 | Fully applied — including missing methods added |
| 4 — ObjectWindow Refactoring | 5 | 5/5 | Fully applied — modelPath helper extracted, 3 methods refactored |
| 5 — Error Handling & Robustness | 8 | 7/8 | Applied except full field validator coverage (~50 editors) |
| 6 — 3D Viewport Enhancements | 6 | 0/6 | Not started |
| 7 — Editor Completions | 7 | 0/7 | Not started |
| 8 — Papyrus & Dialogue Completion | 6 | 0/6 | Not started |
| 9 — Testing | 7 | 0/7 | Not started |
| 10 — Documentation & Final Polish | 6 | 0/6 | Not started |
| **TOTAL** | **66** | **~28/66** | |

---

## Architecture

### Layered Design
```
openck_view (UI layer)
    ↓
openck_model (business logic)
    ↓
openck_files (file I/O)
    ↓
openck_esm (ESM binary format)
```

### Key Components
- **Data Class**: Central data management for all record collections
- **Document Class**: Handles ESM/ESP file loading and saving
- **NifPyFileWrapper**: NIF file operations via Python integration
- **BlenderLauncher**: Blender integration for 3D operations
- **UndoStack**: Undo/redo support throughout the application
- **ShortcutManager**: Customizable keyboard shortcuts

---

## Testing Status: 20/20 Passing

| Test File | Coverage |
|-----------|----------|
| test_shortcutmanager.cpp | ShortcutManager init, get/set, save/load, resetToDefaults |
| test_thememanager.cpp | ThemeManager themeFromName, applyTheme, currentTheme |
| test_nifpyfilewrapper.cpp | NifPyFileWrapper initialize, isInitialized, loadNif error cases |
| test_blenderlauncher.cpp | BlenderLauncher findBlender, parseVersionNumber, isNifCompatibleVersion |
| test_objectwindow.cpp | ObjectWindow modelPath helper (all 10 record types) |
| test_configpaths.cpp | Config path read/write round-trip |
| test_nifintegration.cpp | NIF parser, asset conversion error handling |
| test_pluginio.cpp | ESM I/O round-trip |
| test_integration.cpp | Collection + JSON integration |
| test_conflict.cpp | Conflict detection |
| test_datamodel.cpp | Data model operations |
| test_editrecordcommand.cpp | EditRecordCommand undo/redo |
| test_undostack.cpp | Undo stack operations |
| test_exportimport.cpp | Export/import round-trip |
| test_lodgenerator.cpp | LOD generator |
| test_compressedrecord.cpp | Compressed record (zlib) handling |
| test_headerparsing.cpp | Header parsing robustness |
| test_columnvalidator.cpp | Column validator logic |
| test_searchalgorithm.cpp | Search algorithm tests |
| test_starfieldesm.cpp | Starfield ESM direct loading |

---

## Build System

### Configuration
- **Framework**: Qt 6.5.3
- **Compiler**: MSVC 2019 (msvc2019_64)
- **Language**: C++17
- **Build Tool**: CMake
- **Platform**: 64-bit Windows

### Build Commands
```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.5.3/msvc2019_64
cmake --build build --config Debug --target openck

# Run all tests
$tests = Get-ChildItem build\bin\Debug\test_*.exe | % { $_.FullName }
foreach ($t in $tests) { & $t 2>&1 | Out-Null }
```

---

## Remaining Phases (Phases 6–10)

Phases 6–10 have not yet been started. See `finalPhases.md` for the detailed 66-step plan.

---

## License

OpenCK is released under the GNU General Public License v3.0. See the [LICENSE](LICENSE) file for details.

*For the most current phase tracking, see `finalPhases.md`.*
