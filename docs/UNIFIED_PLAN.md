# OpenCK Unified Completion Plan

> Reconciling the ESM I/O plan, the real-CK 395-file probe,
> the Tes4Codes cross-reference, and the original 10-phase plan.

Sources reconciled here:
- `finalPhases.md` — original 10-phase "Final Completion Plan"
- `docs/IMPLEMENTATION_PLAN.md` — Phase 1 record I/O wiring
- `docs/CK_Real_Integration_Plan.md` — Starfield CK probing (395 source paths)
- `XREF_tes4codes_vs_groundtruth.md` — Tes4Codes translation layer findings

**Status key**: ✅ done, ◐ partial, ⬜ not started

---

## Phase 0: Build Integrity & Housekeeping
> Closing out the git-checkout-damage recovery and Phase 1–5
> finalPhases.md items that are now complete.

| # | Task | Source | Status |
|---|------|--------|--------|
| 0.1 | Fix build after git checkout damage (240 linker errors) | session | ✅ |
| 0.2 | Restore esmreader.hpp/esmwriter.hpp/records.hpp API | session | ✅ |
| 0.3 | Clean 38 stale root temp files | finalPhases 1.3 | ✅ |
| 0.4 | Update .gitignore (temp artifacts, blender/pynifly binaries) | finalPhases 1.5 | ✅ |
| 0.5 | Update STATUS.md to ~42% realistic state | finalPhases 1.4 | ✅ |
| 0.6 | Update finalPhases.md progress tracker | session | ✅ |

---

## Phase 1: ESM I/O Robustness & Tes4Codes Removal
> The XREF ground-truth confirmed the Tes4Codes translation layer
> is unnecessary — no on-disk code collisions exist. Remove it
> and fix the remaining I/O gaps.

| # | Task | Details | Status |
|---|------|---------|--------|
| 1.1 | Remove `Tes4Codes::fromTes4()` translation | Replace all internal codes with on-disk codes in record parsers. All 29 record types using `'ODIT'` → `'MODL'`; 19 using `'ITM0'` → `'DNAM'`; `'IACL'` → `'CNTO'`; `'INPC'` → `'SPLO'`; `'CLAS'` → `'CNAM'` | ⬜ |
| 1.2 | Remove `Tes4Codes::toTes4()` translation | Same reversal for the write direction | ⬜ |
| 1.3 | Delete `Tes4Codes` namespace entirely | After all parsers updated | ⬜ |
| 1.4 | Make `readNSubHeader()` bypass translation | Call `readRawNSubHeader()` directly | ⬜ |
| 1.5 | Handle game-version-specific subrecord codes | e.g., `FLAG` for Starfield KEYM/MISC vs `FNAM` for Skyrim/FO4. Add version-aware branching | ⬜ |
| 1.6 | Run `test_groundtruth.exe` against each supported game's ESM | Build game-specific ground truth tables before applying 1.1–1.5 | ⬜ |
| 1.7 | Verify 20/20 tests still pass after code changes | | ⬜ |
| 1.8 | Document `Document::save()` for all record types | Ensure save round-trips all known record types | ◐ |
| 1.9 | Add record NAME constants for 4CC → CkId::Type mapping | `RecordNameMapping` utility | ◐ |

**Files**: `libs/files/esm/tes4codes.hpp`, `libs/files/esm/esmreader.cpp`, all record parsers in `libs/files/esm/*record.cpp`

---

## Phase 2: Component-Property Architecture
> The single biggest architectural gap. The real CK builds every
> record editor from ~90 reusable BGS*/TES* Components, each
> exposing EditorProperty leaves, rendered by a generic
> QtFormDialog. OpenCK has ~50 bespoke editors instead.

### Tier 1 — Universal Components (every record type has these)

| # | Task | Component | OpenCK File | Status |
|---|------|-----------|-------------|--------|
| 2.1 | Implement `TESFullName_Component` | `fullName: String` | `libs/components/tesfullname.hpp` | ✅ stub |
| 2.2 | Implement `TESModel_Component` | `modelPath, lodModelPath: String` | `libs/components/tesmodel.hpp` | ⬜ |
| 2.3 | Implement `TESTexture_Component` | `iconPath, smallIconPath: String` | `libs/components/testexture.hpp` | ⬜ |
| 2.4 | Implement `TESHealth_Component` | `health: Int` | `libs/components/teshealth.hpp` | ⬜ |
| 2.5 | Implement `TESValue_Component` | `value: Int` | `libs/components/tesvalue.hpp` | ⬜ |
| 2.6 | Implement `TESWeight_Component` | `weight: Float` | `libs/components/tesweight.hpp` | ⬜ |
| 2.7 | Implement `TESDescription_Component` | `description: String` (multiline) | `libs/components/tesdescription.hpp` | ⬜ |
| 2.8 | Implement `TESContainer_Component` | `items: FormComponentArray<TypedFormValuePair>` | `libs/components/tescontainer.hpp` | ⬜ |
| 2.9 | Implement `BGSKeywordForm_Component` | `keywords: FormArray` | `libs/components/bgskeywordform.hpp` | ⬜ |

### Tier 2 — Equipment Components

| # | Task | Component | Status |
|---|------|-----------|--------|
| 2.10 | `TESBipedModel_Component` | biped slots + male/female models | ⬜ |
| 2.11 | `TESEnchantableForm_Component` | enchantment form + max charges | ⬜ |
| 2.12 | `BGSInstanceNamingRulesForm_Component` | Starfield instance naming rules | ⬜ |
| 2.13 | `BGSPickupPutdownSounds_Component` | pickup/drop sound forms | ⬜ |

### Tier 3 — Actor/World Components (deferred, ~40+ components)

| # | Task | Status |
|---|------|--------|
| 2.14 | `TESActorBaseData_Component`, `TESAIForm_Component`, `TESSpellList_Component`, etc. | ⬜ deferred |

### EditorProperty Leaves

| # | Task | Status |
|---|------|--------|
| 2.15 | Implement `BGSBoolEditorProperty`, `BGSStringEditorProperty`, `BGSFormEditorProperty` | ◐ in `libs/components/editorproperty.hpp` |
| 2.16 | Implement `BGSArrayEditorProperty`, `BGSFormArrayEditorProperty`, `BGSFormComponentArrayEditorProperty` | ⬜ |
| 2.17 | Implement `BGSEnumWithImageEditorProperty`, `BGSBitfieldEditorProperty`, `BGSMinMaxEditorProperty` | ⬜ |
| 2.18 | Implement `QtFormDialog` — generic form dialog that walks components and renders properties | ◐ `src/view/window/qtformdialog.cpp` |
| 2.19 | Implement `QtFormDialogManager` — manages open dialogs | ◐ same file |
| 2.20 | Implement `EditorPropertyGrid` widget — the property grid itself | ◐ `src/view/widgets/editorpropertygrid.cpp` |
| 2.21 | Implement `FormComponentWidget` base widget | ◐ `src/view/widgets/formcomponentwidget.cpp` |

**Deliverable**: A record type with Tier 1 components renders in `QtFormDialog` without any bespoke editor code.

**Reference**: `docs/CK_Real_Integration_Plan.md` lines 127–470 for the full list of real CK paths.

---

## Phase 3: Migrate 8 Record Types to Component System
> Migrate the most common record types first, covering ~80% of
> common editing. Once proven, the remaining ~40 types follow.

| # | Task | Record Type | Bespoke Editor File | Status |
|---|------|-------------|---------------------|--------|
| 3.1 | STAT — Static Objects | `stat_editor.cpp` | ⬜ |
| 3.2 | MISC — Miscellaneous Items | `misceditor.cpp` | ⬜ |
| 3.3 | ARMO — Armor | `armor_editor.cpp` | ⬜ |
| 3.4 | WEAP — Weapons | `weaponeditor.cpp` | ⬜ |
| 3.5 | BOOK — Books | `book_editor.cpp` | ⬜ |
| 3.6 | ALCH — Alchemy | `alch_editor.cpp` | ⬜ |
| 3.7 | CONT — Containers | `cont_editor.cpp` | ⬜ |
| 3.8 | ACTI — Activators | `actieditor.cpp` | ⬜ |

After each migration: delete the old bespoke editor file and rewire `ObjectWindowDialog::editSelected` to use `QtFormDialogManager`.

**Deliverable**: 8 record types editable via the generic component dialog.
**Verify**: Edit a STAT, an ARMO, and an NPC_ end-to-end in the UI.

---

## Phase 4: WindowLayout & QtAdvancedDocking Polish
> The real CK saves/restores its dock layout to
> `QtCreationKitSavedSettings.ini`. OpenCK's ADS integration works
> but lacks persistence.

| # | Task | Details | Status |
|---|------|---------|--------|
| 4.1 | Save dock layout on close | Serialize `CDockManager` state to `editor.ini` | ⬜ |
| 4.2 | Restore dock layout on open | Deserialize on startup | ⬜ |
| 4.3 | Default layout for first launch | Pre-configured dock arrangement | ⬜ |
| 4.4 | Window menu to reset layout | "Reset to Default Layout" action | ⬜ |

---

## Phase 5: Migrate Remaining Record Editors (~40 types)
> Once the component system is proven on 8 types, migrate the
> remaining ~40 bespoke editors. Keep bespoke dialogs only for
> complex types that genuinely need custom widgets.

| # | Task | Record Types | Status |
|---|------|-------------|--------|
| 5.1 | Tier-1-only types (have only fullname/model/texture/value/weight) | TREE, LIGH, FURN, MSTT, DOOR, FLOR, etc. | ⬜ |
| 5.2 | Tier-1+2 types (add biped/enchant/sounds) | SPEL, ENCH, SOUN, SCRL, AMMO, etc. | ⬜ |
| 5.3 | Complex types (keep bespoke, but use component grid for basic section) | NPC_, RACE, CELL, QUST, DIAL, INFO, WTHR, LAND, PACK, PERK, FACT, CLASS, REFR, NAVM | ⬜ |
| 5.4 | Wire `ObjectWindowDialog::editSelected` to `QtFormDialogManager` | Replace record-type-specific cases with generic dispatch | ⬜ |
| 5.5 | Delete old bespoke editor .cpp/.hpp files | ~40 files deleted | ⬜ |
| 5.6 | Update CMakeLists.txt to remove deleted files | | ⬜ |

---

## Phase 6: NIF Pipeline & Blender Integration
> These are already applied from the original Phase 2+3 plan.
> Remaining: field validators for all editors.

| # | Task | Details | Status |
|---|------|---------|--------|
| 6.1 | NifPyFileWrapper — all 7 methods | Already implemented | ✅ |
| 6.2 | BlenderLauncher — all 9 methods + bonus | Already implemented | ✅ |
| 6.3 | ObjectWindow refactoring — modelPath helper | Already refactored | ✅ |
| 6.4 | Error handling — 7 of 8 steps | Applied | ✅ |
| 6.5 | fieldvalidators.hpp — deploy to remaining ~43 editors | Currently deployed to 7 | ⬜ |

---

## Phase 7: 3D Viewport Enhancements

✅ **6/6 complete** — Already implemented before this plan was written. Phong lighting, interleaved position/normal/UV/color vertex format, texture sampling, VAO batching. No additional work needed.

| # | Task | Status |
|---|------|--------|
| 7.1 | NIF version handling | ✅ Already implemented |
| 7.2 | Normals extraction | ✅ Already implemented (recalculateNormals + shader) |
| 7.3 | UV coordinate extraction | ✅ Already implemented |
| 7.4 | Texture coordinate mapping | ✅ Already implemented |
| 7.5 | Improved lighting model | ✅ Already implemented (Phong in GLSL) |
| 7.6 | Mesh batching / VAO optimization | ✅ Already implemented |

---

## Phase 8: Editor Completions
> From original Phase 7. Several editors have placeholder UI.

| # | Task | Details | Status |
|---|------|---------|--------|
| 8.1 | Spell Editor 3D preview | Replace placeholder with NifViewportWidget | ⬜ |
| 8.2 | Enchantment Editor 3D preview | Replace placeholder with NifViewportWidget | ⬜ |
| 8.3 | Landscape heightmap persistence | Save heightmap to CellRecord data on close | ⬜ |
| 8.4 | Landscape brush repaint | Connect brush tool to viewport update() signal | ⬜ |
| 8.5 | Landscape height limit slider | Implement real clamping based on slider value | ⬜ |
| 8.6 | Object palette from game data | Replace hardcoded list with actual record query | ⬜ |
| 8.7 | Object placement persistence | Serialize cell references to ESM on save | ⬜ |

---

## Phase 9: Papyrus & Dialogue Completion
> From original Phase 8.

| # | Task | Details | Status |
|---|------|---------|--------|
| 9.1 | Papyrus if/else/elif statements | Add AST nodes and codegen | ⬜ |
| 9.2 | Papyrus while/for loops | Add loop codegen | ⬜ |
| 9.3 | Papyrus type checking | Validate variable types at compile time | ⬜ |
| 9.4 | Dialogue conditional response editing | Quest stage conditions, variable checks | ⬜ |
| 9.5 | Dialogue voice file association | Link dialogue lines to .wav files | ⬜ |
| 9.6 | Quest graph stage editing | Stage flags, indices, objectives | ⬜ |

---

## Phase 10: Testing
> From original Phase 9. 20 tests exist (up from original 11).

| # | Task | Test File | Status |
|---|------|-----------|--------|
| 10.1 | ShortcutManager unit test | `test_shortcutmanager.cpp` | ✅ |
| 10.2 | ThemeManager unit test | `test_thememanager.cpp` | ✅ |
| 10.3 | NifPyFileWrapper unit test | `test_nifpyfilewrapper.cpp` | ✅ |
| 10.4 | BlenderLauncher unit test | `test_blenderlauncher.cpp` | ✅ |
| 10.5 | ObjectWindow modelPath helper | `test_objectwindow.cpp` | ✅ |
| 10.6 | Config paths round-trip | `test_configpaths.cpp` | ✅ |
| 10.7 | NIF integration test | `test_nifintegration.cpp` | ✅ |
| 10.8 | ESM I/O round-trip test | `test_pluginio.cpp` | ✅ |
| 10.9 | Integration (collection + JSON) | `test_integration.cpp` | ✅ |
| 10.10 | Conflict detection | `test_conflict.cpp` | ✅ |
| 10.11 | Data model operations | `test_datamodel.cpp` | ✅ |
| 10.12 | EditRecordCommand undo/redo | `test_editrecordcommand.cpp` | ✅ |
| 10.13 | Undo stack operations | `test_undostack.cpp` | ✅ |
| 10.14 | Export/import round-trip | `test_exportimport.cpp` | ✅ |
| 10.15 | LOD generator | `test_lodgenerator.cpp` | ✅ |
| 10.16 | Compressed record zlib | `test_compressedrecord.cpp` | ✅ |
| 10.17 | Header parsing robustness | `test_headerparsing.cpp` | ✅ |
| 10.18 | Column validator | `test_columnvalidator.cpp` | ✅ |
| 10.19 | Search algorithm | `test_searchalgorithm.cpp` | ✅ |
| 10.20 | Starfield ESM loading | `test_starfieldesm.cpp` | ✅ |
| 10.21 | Component unit test | `test_component.cpp` | ⬜ (exists but not counting) |
| 10.22 | QtFormDialog unit test | `test_qtformdialog.cpp` | ⬜ |
| 10.23 | Editor lifecycle test | `test_editor_lifecycle.cpp` | ⬜ |
| 10.24 | EditorProperty unit test | `test_editorproperty.cpp` | ⬜ |

---

## Phase 11: Documentation & Final Polish
> From original Phase 10 plus CK_Real_Integration updates.

| # | Task | Details | Status |
|---|------|---------|--------|
| 11.1 | Update STATUS.md | Match unified plan | ⬜ |
| 11.2 | Update TECHNICAL_DEBT.md | Resolve tracked items | ⬜ |
| 11.3 | Update ROADMAP.md | Reflect completion milestones | ⬜ |
| 11.4 | Add API doc comments to public interfaces | Data, NifPyFileWrapper, BlenderLauncher, ShortcutManager | ⬜ |
| 11.5 | Final build + test pass | cmake --build && ctest — 20/20 green | ⬜ |

---

## Progress Tracker

| Phase | Steps | Status |
|-------|-------|--------|
| 0 — Build Integrity | 6/6 | ✅ |
| 1 — ESM I/O & Tes4Codes Removal | 2/9 | ◐ (1.8, 1.9 partial, rest ⬜) |
| 2 — Component-Property Architecture | 3/21 | ◐ (2.1, 2.15, 2.18-2.21 partial) |
| 3 — Migrate 8 Record Types | 0/8 | ⬜ |
| 4 — WindowLayout & ADS Polish | 0/4 | ⬜ |
| 5 — Migrate Remaining 40 Record Types | 0/5 | ⬜ |
| 6 — NIF Pipeline & Blender Integration | 4/5 | ✅ (6.5 field validators ⬜) |
| 7 — 3D Viewport Enhancements | 0/6 | ⬜ |
| 8 — Editor Completions | 0/7 | ⬜ |
| 9 — Papyrus & Dialogue | 0/6 | ⬜ |
| 10 — Testing | 20/24 | ◐ |
| 11 — Documentation | 0/5 | ⬜ |
| **TOTAL** | **~35/106** | |

## Key Architectural Insight

The real CK has **two parallel editor pathways**:
1. **Legacy**: Per-record-type `TES*Editor` dialogs (the 127 `_Editor.cpp` files)
2. **Modern**: `QtCreationKitFormDialog` + Components (the QtFormEditing path)

OpenCK should replicate only the modern path. The legacy path exists in the CK for
back-compat with pre-existing code. We don't have that constraint — we build fresh.

The component system (Phase 2) is the critical path. Everything else depends on it.
Once components exist, recording editing becomes data-driven:
- A record type declares `FormComponents` in its header
- `QtFormDialog` walks the components and renders `EditorProperty` widgets
- No per-record dialog code needed

## How to Start

```
Phase 1 → Phase 2 (Tier 1) → Phase 3 (migrate 8 types) → Phase 4 → Phase 5 → Phases 6-11
```

Phase 1 (Tes4Codes removal) is a mechanical find-replace across all record parsers
that can be done in parallel with Phase 2 (component implementation).

---

*Replaces: `finalPhases.md`, `docs/IMPLEMENTATION_PLAN.md`*
*Supersedes: `docs/CK_Real_Integration_Plan.md` (keep as reference)*
*Updated: 2026-07-24*
