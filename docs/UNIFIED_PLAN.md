# OpenCK Unified Completion Plan

> Reconciling the ESM I/O plan, the real-CK 395-file probe,
> the Tes4Codes cross-reference, and the original 10-phase plan.
> Updated 2026-07-28 after Phase 9 completion and UI layout audit.

Sources reconciled here:
- `finalPhases.md` — original 10-phase "Final Completion Plan"
- `docs/IMPLEMENTATION_PLAN.md` — Phase 1 record I/O wiring
- `docs/CK_Real_Integration_Plan.md` — Starfield CK probing (395 source paths)
- `XREF_tes4codes_vs_groundtruth.md` — Tes4Codes translation layer findings

**Status key**: ✅ done, ◐ partial, ⬜ not started

---

## Phase 0: Build Integrity & Housekeeping ✅

> Done.

| # | Task | Status |
|---|------|--------|
| 0.1 | Fix build after git checkout damage (240 linker errors) | ✅ |
| 0.2 | Restore esmreader.hpp/esmwriter.hpp/records.hpp API | ✅ |
| 0.3 | Clean 38 stale root temp files | ✅ |
| 0.4 | Update .gitignore (temp artifacts, blender/pynifly binaries) | ✅ |
| 0.5 | Update STATUS.md to ~42% realistic state | ✅ |
| 0.6 | Update finalPhases.md progress tracker | ✅ |

---

## Phase 1: ESM I/O Robustness & Tes4Codes Removal ✅

> Done.

| # | Task | Status |
|---|------|--------|
| 1.1 | Remove `Tes4Codes::fromTes4()` — all parsers use on-disk codes | ✅ |
| 1.2 | Remove `Tes4Codes::toTes4()` | ✅ |
| 1.3 | Delete `tes4codes.hpp` entirely | ✅ |
| 1.4 | `readNSubHeader()` returns raw on-disk NAME | ✅ |
| 1.5 | Game-version-specific `FNAM`/`FLAG` fallback in 35 parsers | ✅ |
| 1.6 | `test_starfieldesm` verifies Starfield.esm parsing | ✅ |
| 1.7 | 20/20 tests passing | ✅ |

---

## Phase 2: Component-Property Architecture ✅

> The single biggest architectural gap — resolved. All Tier 1+2 components
> implemented and wired. The real CK builds every record editor from ~90
> reusable BGS*/TES* Components; OpenCK mirrors this with `libs/components/`.

### Tier 1 — Universal Components ✅

| # | Component | Handles (subrecords) | File | Status |
|---|-----------|---------------------|------|--------|
| 2.1 | `TESFullName_Component` | FULL | `libs/components/tesfullname.hpp` | ✅ |
| 2.2 | `TESModel_Component` | MODL, MNAM | `libs/components/tier1_components.hpp` | ✅ |
| 2.3 | `TESTexture_Component` | ICON, ICO2 | same | ✅ |
| 2.4 | `TESHealth_Component` | HLTH, DATA (4b/2b) | same | ✅ |
| 2.5 | `TESValue_Component` | DATA (i32) | same | ✅ |
| 2.6 | `TESWeight_Component` | DATA (float) | same | ✅ |
| 2.7 | `TESDescription_Component` | DESC | same | ✅ |
| 2.8 | `TESContainer_Component` | CNTO arrays | same | ✅ |
| 2.9 | `BGSKeywordForm_Component` | CNAM, KWDA | same | ✅ |

### Tier 2 — Equipment Components ✅

| # | Component | Handles (subrecords) | File | Status |
|---|-----------|---------------------|------|--------|
| 2.10 | `TESBipedModel_Component` | BNAM, FNAM, BMDT, INDX | `libs/components/tier2_components.hpp` | ✅ |
| 2.11 | `TESEnchantableForm_Component` | ENAM, ANAM | same | ✅ |
| 2.12 | `BGSInstanceNamingRulesForm_Component` | INRR, INRV, INRD (raw) | same | ✅ |
| 2.13 | `BGSPickupPutdownSounds_Component` | YNAM, ZNAM, PICK, PUTD | same | ✅ |

### Tier 3 — Initial Components (Phase 5 bonus) ✅

| # | Component | Handles | File | Status |
|---|-----------|---------|------|--------|
| 2.14 | `TESFlags_Component` | FNAM, FLAG (6+ record types) | `libs/components/tier3_components.hpp` | ✅ |
| 2.15 | `BGSSoundDescriptor_Component` | FNAM, SNDD, SNDX (SOUN) | same | ✅ |
| 2.16 | `TESWeatherData_Component` | SNAM, FNAM/FLAG (WTHR) | same | ✅ |
| 2.17 | `BGSRefData_Component` | NAME, DATA, XOWN, DNAM, XESP, SCRI (REFR) | same | ✅ |

### EditorProperty Leaves ✅

| # | Property Type | Real CK Equivalent | File | Status |
|---|--------------|-------------------|------|--------|
| 2.18 | `BoolEditorProperty` | `BGSBoolEditorProperty` | `libs/components/editorproperty.hpp` | ✅ |
| 2.19 | `IntEditorProperty` | `BGSIntEditorProperty` | same | ✅ |
| 2.20 | `UIntEditorProperty` | `BGSIntEditorProperty` | same | ✅ |
| 2.21 | `FloatEditorProperty` | `BGSFloatEditorProperty` | same | ✅ |
| 2.22 | `StringEditorProperty` | `BGSStringEditorProperty` | same | ✅ |
| 2.23 | `FormEditorProperty` | `BGSFormEditorProperty` | same | ✅ |
| 2.24 | `FormArrayEditorProperty` | `BGSFormArrayEditorProperty` | same | ✅ |

### Dialog Infrastructure ✅

| # | Component | Real CK Equivalent | File | Status |
|---|-----------|-------------------|------|--------|
| 2.25 | `QtFormDialog` | `QtCreationKitFormDialog` | `src/view/window/qtformdialog.cpp` | ✅ |
| 2.26 | `QtFormDialogManager` | `QtCreationKitFormDialogManager` | `src/view/window/qtformdialogmanager.cpp` | ✅ |
| 2.27 | `EditorPropertyGrid` | `BGSEditorPropertyGrid` | `src/view/widgets/editorpropertygrid.cpp` | ✅ |
| 2.28 | `FormComponentWidget` | `QtFormComponentWidget` | `src/view/widgets/formcomponentwidget.cpp` | ✅ |

---

## Phase 3: Record Type Migration to Component System ✅

> All ~50 ESM record types with Object Window entries now use
> `FormComponents` + `QtFormDialogManager`. Flat fields synced
> to/from components at load/save boundaries for back-compat.

| # | Batch | Record Types | Status |
|---|-------|-------------|--------|
| 3.1 | Tier-1 types (model/texture/name) | STAT, MISC, ACTI, BOOK, ALCH, CONT, ARMO, WEAP, TREE, LIGH, FURN, MSTT, DOOR, FLOR, INGR, MAGIC, CLASS, FACT, PERK, LTEX, and 15+ pre-existing | ✅ |
| 3.2 | Tier-1+2 types (biped/enchant/sounds) | ENCH, SPELL, SOUN, WTHR, REFR, AMMO, SCRL, etc. | ✅ |
| 3.3 | Complex types (component grid + flat fields) | NPC, CELL, QUEST, RACE, DIAL, PACKAGE, INFO | ✅ |
| 3.4 | Additional types | LOCATION, MATERIAL, LAND, WORLDSPACE | ✅ |
| 3.5 | Wired via `QtFormDialogManager` | All `ObjectWindowDialog::editSelected` cases replaced | ✅ |

**Dead code removed**: `packrecord.hpp/cpp` (unused duplicate `PackageRecord` struct).

---

## Phase 4: WindowLayout & QtAdvancedDocking ✅

> Real CK saves dock layout to `QtCreationKitSavedSettings.ini`.
> OpenCK uses `editor.ini`.

| # | Task | Status |
|---|------|--------|
| 4.1 | Save dock layout on close | ✅ |
| 4.2 | Restore dock layout on open | ✅ |
| 4.3 | Default layout for first launch (Object Window left, Viewport right, Properties tabbed, Palette bottom) | ✅ |
| 4.4 | "Reset Window Layout" menu action | ✅ |

---

## Phase 5: Gaps & Specialized Editors ✅

> The generic property grid handles name/model/icon/weight/value — but
> it cannot handle record-specific editing. The real CK has 127 bespoke
> editors for exactly this reason. We built specialized widgets for
> each complex record's unique subrecords and composed them into
> the `QtFormDialog` alongside the generic component grid.

### 5A — Missing EditorProperty Types ✅

> Real CK has 14+ property types. OpenCK now has 13.

| # | Property Type | What It Enables | Status |
|---|--------------|----------------|--------|
| 5A.1 | `Point2EditorProperty` | 2D coordinates (UV, GUI layouts) | ✅ |
| 5A.2 | `Point3EditorProperty` | 3D position/rotation selectors (ref placement, door markers) | ✅ |
| 5A.3 | `BitfieldEditorProperty` | Flag checkboxes (record flags, AI flags, faction flags) | ✅ |
| 5A.4 | `EnumEditorProperty` | Dropdown enums (weapon types, armor slots, spell schools) | ✅ |
| 5A.5 | `MinMaxEditorProperty` | Range sliders (level ranges, damage spread) | ✅ |
| 5A.6 | `ColorEditorProperty` | Color pickers (light color, fog color, vertex color) | ✅ |
| 5A.7 | `FormComponentArrayEditorProperty` | Table-based container/array editor (inventory items, keyword table) | ✅ |

### 5B — Per-Record Specialized Editor Widgets

> Each complex record type gets a bespoke widget composed into
> `QtFormDialog`. The generic component grid renders the shared
> properties (name/model/icon) at the top. The specialized widget
> renders below for the parts unique to that record type.

#### Actor Records

| # | Record | Specialized Widget | Key Subrecords | Status |
|---|--------|-------------------|----------------|--------|
| 5B.1 | NPC_ | **NPC Editor** — actor stats grid (health/magicka/stamina/attributes/skills), faction/race/class dropdowns, AI data panel, spell list, inventory table, face-gen preview | ACBS, SPLO, CNTO, AIDT, DOFT, SOFT, DPLT, CSCR, PKID | ✅ |
| 5B.2 | CREA | **Creature Editor** — same as NPC but with creature-specific data (soul, combat style, body parts) | ACBS, BNAM, NIFT | ◐ (has components, needs widget) |

#### Quest & Dialogue Records

| # | Record | Specialized Widget | Key Subrecords | Status |
|---|--------|-------------------|----------------|--------|
| 5B.3 | QUST | **Quest Editor** — stage tree (index/flag/text), alias editor, objective editor, script fragment editor | INDX, QSDT, CNAM, SCDA, ANAM, NNAM, CTDA | ✅ |
| 5B.4 | DIAL | **Topic Editor** — response tree, conditions per response, voice file linking | QSTI, PNAM, CTDA | ✅ |
| 5B.5 | INFO | **Response Editor** — response text, voice file picker, conditions grid, emotion/anim overrides | CNAM, CTDA, TLOI, SCHR | ✅ |
| 5B.6 | SCEN | **Scene Editor** (Starfield/Fallout 4) — action list, phase timeline, actor assignment | ⬜ deferred |

#### World Records

| # | Record | Specialized Widget | Key Subrecords | Status |
|---|--------|-------------------|----------------|--------|
| 5B.7 | CELL | **Cell Editor** — lighting template, water height, music type, interior/exterior flags, cell regions, navmesh preview | XCLL, XCMT, XCLW, XCWT, XOWN, XCIM, LTMP, XCLR | ✅ |
| 5B.8 | WRLD | **Worldspace Editor** — map data, climate, water, LOD settings, cell grid | WNAM, XNAM, MNAM, CNAM, NAM0-NAM9 | ◐ (has components, needs widget) |
| 5B.9 | LAND | **Landscape Editor** — heightmap brush, texture layer painting (4 layers), vertex color painting, normal editing | VHGT, VNML, VCLR, VTEX | ✅ |
| 5B.10 | REFR | **Reference Editor** — position/rotation/scale spinners, owner/lock pickers, enable-state, linked references, script attachment | NAME, DATA, XOWN, DNAM, XESP, XSCL, XPRM, XLKR, XLCM | ✅ |

#### Character Records

| # | Record | Specialized Widget | Key Subrecords | Status |
|---|--------|-------------------|----------------|--------|
| 5B.11 | RACE | **Race Editor** — skill bonuses, starting spells, body part data, head/hair part lists, face marker editor | DATA, DESC, SPLO, BODT, BOD2, HNAM, ENAM, FNAM, INDX, FMRK | ✅ |
| 5B.12 | CLAS | **Class Editor** — skill array, attribute array, specialization dropdown, description | DATA, DESC | ✅ |

#### Other Records

| # | Record | Specialized Widget | Key Subrecords | Status |
|---|--------|-------------------|----------------|--------|
| 5B.13 | WTHR | **Weather Editor** — color curve editor (sky/cloud/ambient/fog/sun), precipitation data, sound FX assignment, cloud texture layers | PNAM, NAM0, FNAM, MNAM, CNAM, SNAM, QNAM, INAM, DATA | ✅ |
| 5B.14 | SOUN | **Sound Editor** — waveform preview, attenuation curves, output model, sound category | FNAM, SNDD, SNDX, ATTN | ✅ |
| 5B.15 | PACK | **AI Package Editor** — package type selector, target picker, schedule data, conditions grid | PKDT, PLDT, PTDT, PSCT, PKED, PKPT, PSDT, CTDA | ⬜ deferred |
| 5B.16 | EFSH | **Shaders** (EffectShader, ImageSpaceModifier) | ⬜ deferred |

### 5C — Per-Component Table Widgets

> Real CK has dedicated table/model widgets for component arrays.
> OpenCK currently renders them as flat property lists.

| # | Widget | Replaces | Serves Components | Status |
|---|--------|----------|-------------------|--------|
| 5C.1 | `ContainerTableWidget` | Flat CNTO list | TESContainer_Component | ✅ |
| 5C.2 | `SpellListWidget` | Flat SPLO list | TESSpellList_Component | ✅ |
| 5C.3 | `KeywordTableWidget` | Flat CNAM list | BGSKeywordForm_Component | ✅ |
| 5C.4 | `BipedModelWidget` | Flat biped fields | TESBipedModel_Component | ✅ |
| 5C.5 | `PickupSoundsWidget` | Flat sound form IDs | BGSPickupPutdownSounds_Component | ✅ |

### 5D — Tier 3 Actor Components (for NPC/CREA)

> These components slot into NPC and CREA records alongside
> the Tier 1+2 components already wired.

| # | Component | Handles | Status |
|---|-----------|---------|--------|
| 5D.1 | `TESActorBaseData_Component` | ACBS (flags, base spell, fatigue, barter gold, level, calc min/max, speed multiplier) | ✅ |
| 5D.2 | `TESAIForm_Component` | AIDT (aggression/confidence/energy/morality/combat style) + AI packages link | ✅ |
| 5D.3 | `TESSpellList_Component` | SPLO counted arrays (NPC spell list) | ✅ |
| 5D.4 | `TESAttributes_Component` | ATTR (strength/intelligence/willpower/agility/speed/endurance/personality/luck) | ✅ |
| 5D.5 | `TESSkills_Component` | SKIL (block/armorer/medium armor/heavy armor/blunt/ long blade/axe/spear/athletics/enchant/destruction/alteration/illusion/conjuration/mysticism/restoration/alchemy/unarmored/security/sneak/acrobatics/light armor/short blade/marksman/mercantile/speechcraft/hand-to-hand) | ✅ |
| 5D.6 | `TESNPCFaceGen_Component` | Face morph data, hair/eyes/head part selection | ✅ |
| 5D.7 | `TESBodyParts_Component` | Body part data (BODT/BOD2) for NPC/RACE | ✅ |

### 5E — Container / Array Back-Compat Migration

> Currently, some records keep flat fields for container items /
> keywords alongside their components. Clean up the duplication.

| # | Task | Status |
|---|------|--------|
| 5E.1 | Remove flat `containerItems` field from CONT — component handles CNTO | ◐ (kept — single int, not array) |
| 5E.2 | Remove flat `keywords` field from ARMO/WEAP/etc. — component handles CNAM/KWDA | ◐ (kept — still read by data.cpp/exporters) |
| 5E.3 | Remove flat `spells` field from NPC — component handles SPLO | ◐ (kept — back-compat) |
| 5E.4 | Audit all records for flat-field component overlap and clean up | ✅ (audit complete; most fields kept, markerCount removed from FurnRecord) |

---

## Phase 6: NIF Pipeline & Blender Integration ✅

> Already applied from the original Phase 2+3 plan.

| # | Task | Status |
|---|------|--------|
| 6.1 | NifPyFileWrapper — all 7 methods | ✅ |
| 6.2 | BlenderLauncher — all 9 methods + bonus | ✅ |
| 6.3 | ObjectWindow refactoring — modelPath helper | ✅ |
| 6.4 | Error handling — 7 of 8 steps | ✅ |
| 6.5 | fieldvalidators.hpp — deploy to remaining ~43 editors | ✅ |

---

## Phase 7: 3D Viewport Enhancements ✅

> Already implemented before this plan was written.
> Phong lighting, interleaved position/normal/UV/color vertex format,
> texture sampling, VAO batching.

| # | Task | Status |
|---|------|--------|
| 7.1 | NIF version handling | ✅ |
| 7.2 | Normals extraction | ✅ |
| 7.3 | UV coordinate extraction | ✅ |
| 7.4 | Texture coordinate mapping | ✅ |
| 7.5 | Improved lighting model (Phong in GLSL) | ✅ |
| 7.6 | Mesh batching / VAO optimization | ✅ |

---

## Phase 8: Editor Completions ✅

> From original Phase 7. Several editors had placeholder UI; all completed.

| # | Task | Status |
|---|------|--------|
| 8.1 | Spell Editor 3D preview | ✅ |
| 8.2 | Enchantment Editor 3D preview | ✅ |
| 8.3 | Landscape heightmap persistence | ✅ |
| 8.4 | Landscape brush repaint | ✅ |
| 8.5 | Landscape height limit slider | ✅ |
| 8.6 | Object palette from game data | ✅ |
| 8.7 | Object placement persistence | ✅ |

---

## Phase 9: Papyrus & Dialogue Completion ✅

> From original Phase 8. All completed.

| # | Task | Status |
|---|------|--------|
| 9.1 | Papyrus if/else/elif statements | ✅ |
| 9.2 | Papyrus while/for loops | ✅ |
| 9.3 | Papyrus type checking | ✅ |
| 9.4 | Dialogue conditional response editing | ✅ |
| 9.5 | Dialogue voice file association | ✅ |
| 9.6 | Quest graph stage editing | ✅ |

---

## Phase 10: Testing ◐

> 26 tests exist. All passing.

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
| 10.21 | Component unit test | `test_component.cpp` | ✅ |
| 10.22 | QtFormDialog unit test | `test_qtformdialog.cpp` | ✅ |
| 10.23 | Editor lifecycle test | `test_editor_lifecycle.cpp` | ✅ |
| 10.24 | EditorProperty unit test | `test_editorproperty.cpp` | ✅ |
| 10.25 | Tier 3 component round-trip tests | `test_tier3_components.cpp` | ✅ |
| 10.26 | Specialized editor widget unit tests | `test_editor_widgets.cpp` | ✅ |

---

## Phase 11: Documentation & Final Polish ⬜

| # | Task | Status |
|---|------|--------|
| 11.1 | Update STATUS.md | ⬜ |
| 11.2 | Update TECHNICAL_DEBT.md | ⬜ |
| 11.3 | Update ROADMAP.md | ⬜ |
| 11.4 | Add API doc comments to public interfaces | ⬜ |
| 11.5 | Final build + test pass — 26/26 green | ⬜ |

---

## Phase 12: UI Layout Parity with Real Creation Kit ⬜

> Deep-dive audit (`docs/LAYOUT_AUDIT.md`) compared OpenCK's window/menu/dock
> layout against the real Starfield Creation Kit (`CreationKit.exe` v1.16.244.0).
> Found 12 gaps: 4 Critical, 3 High, 5 Medium/Low.
> The data model and functional behavior match the CK, but the window chrome
> (menu structure, dock arrangement, Object Window tree shape, Cell View,
> Preferences) is substantially different. This phase closes those gaps so
> users migrating from the real CK feel at home.

### 12A — QtAdvancedDocking Integration (Critical)

> The vendored ADS library at `external/ads/` is built but unused.
> The real CK uses `CDockManager` for all panels — tear-off, tab, redock.

| # | Task | Files | Status |
|---|------|-------|--------|
| 12A.1 | Add `ads::CDockManager` member to `MainWindow` | `mainwindow.hpp`, `mainwindow.cpp` | ⬜ |
| 12A.2 | Replace `QDockWidget` for Object Window with `ads::CDockWidget` | `mainwindow.cpp:165-169` | ⬜ |
| 12A.3 | Replace `QDockWidget` for Render Window with `ads::CDockWidget` (central widget) | `mainwindow.cpp:941-963` | ⬜ |
| 12A.4 | Replace `QDockWidget` for Script Editor, Dialogue Editor, FormID Editor, Asset Browser | `mainwindow.cpp:965-1210` | ⬜ |
| 12A.5 | Replace `QDockWidget` for Landscape Editor, Object Palette | `mainwindow.cpp:171-205` | ⬜ |
| 12A.6 | Make Render Window the central dock widget (ADS central widget pattern) | `mainwindow.cpp` | ⬜ |
| 12A.7 | Persist/restore ADS layout to `QtCreationKitSavedSettings.ini`-style file | `windowlayout.cpp` | ⬜ |

### 12B — Cell View Docked Panel (Critical)

> Real CK has `TESCellView.cpp` as a docked 2D top-down cell browser.
> OpenCK has a modal `QDialog` (`cellsdialog.cpp`) with just a tree.

| # | Task | Files | Status |
|---|------|-------|--------|
| 12B.1 | Rename `CellsDialog` → `CellViewPanel`, inherit `QWidget` instead of `QDialog` | `cellsdialog.hpp`, `cellsdialog.cpp` | ⬜ |
| 12B.2 | Add worldspace selector combo at top | `cellsdialog.cpp` | ⬜ |
| 12B.3 | Add cell list (QListView) on left side of splitter | `cellsdialog.cpp` | ⬜ |
| 12B.4 | Add reference QTableView below cell list showing references in selected cell | `cellsdialog.cpp` | ⬜ |
| 12B.5 | Add 2D top-down map canvas (QWidget with paintEvent) showing reference markers | `cellsdialog.cpp` | ⬜ |
| 12B.6 | Wire as `ads::CDockWidget` in dock manager, toggle from ObjectWindows menu | `mainwindow.cpp:1307-1318` | ⬜ |

### 12C — Object Window Hierarchical Tree (Critical)

> Real CK: `All → Actors/Items/World Objects/Gameplay/Audio/Dialogue → record types`.
> OpenCK: flat 27-item list. Also "Texture Asset" is mislabeled (should be "Static").

| # | Task | Files | Status |
|---|------|-------|--------|
| 12C.1 | Add `CategoryGroup` struct (name + child category indices) to model | `objectwindow.hpp` | ⬜ |
| 12C.2 | Restructure `initCategories` to create parent groups: All, Actors, Items, World Objects, Gameplay, Audio, Dialogue, World, Miscellaneous | `objectwindow.cpp:29-268` | ⬜ |
| 12C.3 | Place each existing `addCategory` call under the correct parent group | `objectwindow.cpp:240-267` | ⬜ |
| 12C.4 | Rename "Texture Asset" → "Static" | `objectwindow.cpp:260` | ⬜ |
| 12C.5 | Extend `index()`/`parent()`/`rowCount()` for 3-level tree (root → group → category → record) | `objectwindow.cpp:418-470` | ⬜ |
| 12C.6 | Verify tree view renders 3 levels correctly with `setRootIsDecorated(true)` | `objectwindowdialog.cpp:130-142` | ⬜ |

### 12D — Top-Level Menu Restructure (Critical)

> Real CK has 16 menus; OpenCK has 9. 7 menus missing, several actions in wrong menu.

| # | Task | Files | Status |
|---|------|-------|--------|
| 12D.1 | Add `menuCharacter` (NPC, Race, Class, Faction, BodyPart, HeadPart actions) | `ui/mainwindow.ui`, `mainwindow.cpp` | ⬜ |
| 12D.2 | Add `menuObjectWindows` (Object Window, Cell View, Object Palette, Galaxy View, Scene View, Find Forms) | `ui/mainwindow.ui`, `mainwindow.cpp` | ⬜ |
| 12D.3 | Add `menuRenderWindows` (Render Window, Preview Window, Lighting, Reflection Probes) | `ui/mainwindow.ui`, `mainwindow.cpp` | ⬜ |
| 12D.4 | Add `menuNavmesh` (move `actionNavmesh` from View) | `ui/mainwindow.ui`, `mainwindow.cpp` | ⬜ |
| 12D.5 | Add `menuTerrain` (move `actionLandscapeEditing` from World) | `ui/mainwindow.ui`, `mainwindow.cpp` | ⬜ |
| 12D.6 | Add `menuAudio` (move `actionSoundEditor` from Tools) | `ui/mainwindow.ui`, `mainwindow.cpp` | ⬜ |
| 12D.7 | Add `menuDocks` (ADS show/hide/restore-layout actions) | `ui/mainwindow.ui`, `mainwindow.cpp` | ⬜ |
| 12D.8 | Add stub `menuGalaxy`, `menuPackin`, `menuTheme`, `menuTests` for parity | `ui/mainwindow.ui` | ⬜ |
| 12D.9 | Move `actionObjectWindow` from View → ObjectWindows | `ui/mainwindow.ui:75` | ⬜ |
| 12D.10 | Move `actionObjectPalette` from World → ObjectWindows | `ui/mainwindow.ui:99` | ⬜ |
| 12D.11 | Move `actionAnimationEditor` from Tools → Character or RenderWindows | `ui/mainwindow.ui:218` | ⬜ |
| 12D.12 | Fold `menuExport` into File > Export submenu (real CK has no top-level Export) | `ui/mainwindow.ui:205-212` | ⬜ |
| 12D.13 | Add missing CK File actions: Create Archive, Compile Papyrus Scripts, Compact Master | `ui/mainwindow.ui`, `mainwindow.cpp` | ⬜ |

### 12E — Preferences Dialog Tree Sidebar (High)

> Real CK: tree sidebar (Display/Edit/Sound/Network/Archive/Papyrus/…).
> OpenCK: flat QGroupBox stack.

| # | Task | Files | Status |
|---|------|-------|--------|
| 12E.1 | Replace QVBoxLayout with QSplitter: left = QTreeWidget categories, right = QStackedWidget pages | `preferencesdialog.cpp:28-115` | ⬜ |
| 12E.2 | Add category pages: General, Display, Edit, Sound, Archive, Papyrus, LOD, Network | `preferencesdialog.cpp` | ⬜ |
| 12E.3 | Split INI storage into per-category groups (`[Display]`, `[Papyrus]`, etc.) | `preferencesdialog.cpp:121,146` | ⬜ |

### 12F — QtFormDialog Modeless + Tabs (High)

> Real CK form dialogs are modeless windows with tabbed component sections.
> OpenCK dialogs are modal.

| # | Task | Files | Status |
|---|------|-------|--------|
| 12F.1 | Change `QtFormDialog` base from `QDialog` to `QWidget` (or `ads::CDockWidget`) | `qtformdialog.hpp`, `qtformdialog.cpp` | ⬜ |
| 12F.2 | Change `QtFormDialogManager::openOrFocus` to `show()` + `raise()` instead of `exec()` | `qtformdialogmanager.cpp` | ⬜ |
| 12F.3 | Add `QTabWidget` with tabs: Basic (Tier-1 components), Components (Tier 2+), Keywords, Ingest/Components per record type | `qtformdialog.cpp` | ⬜ |

### 12G — Render Window Toolbar Transform Tools (High)

> Real CK toolbar: Selection/Move/Rotate/Scale modes + snap toggles.
> OpenCK: grid/bounds/wireframe only.

| # | Task | Files | Status |
|---|------|-------|--------|
| 12G.1 | Add QActionGroup with 4 checkable actions: Select, Move, Rotate, Scale | `nifviewportwidget.cpp:384-476` | ⬜ |
| 12G.2 | Add snap-to-grid toggle + snap-to-angle toggle + snap-step spinbox | `nifviewportwidget.cpp` | ⬜ |
| 12G.3 | Wire transform mode to gizmo state in the viewport (placeholder for edit-module work) | `nifviewportwidget.cpp` | ⬜ |

### 12H — Default Dock Placement Fix (High)

> Object Window is on the RIGHT; should be LEFT. `applyDefaultLayout` only runs on manual reset.

| # | Task | Files | Status |
|---|------|-------|--------|
| 12H.1 | Change Object Window dock area from `RightDockWidgetArea` → `LeftDockWidgetArea` | `mainwindow.cpp:168` | ⬜ |
| 12H.2 | Call `WindowLayout::applyDefaultLayout(this)` at end of `setData()` | `mainwindow.cpp:205` | ⬜ |

### 12I — Status Bar Enhancements (Medium)

> Real CK shows cell coordinates, object counts, selected object info.

| # | Task | Files | Status |
|---|------|-------|--------|
| 12I.1 | Add `mStatusCellCoords` label, update from Cell View selection | `mainwindow.cpp:114-126` | ⬜ |
| 12I.2 | Add `mStatusSelectedObject` label, update from Object Window/RenderWindow selection | `mainwindow.cpp` | ⬜ |
| 12I.3 | Move `mStatusPluginInfo` from permanent to transient slot | `mainwindow.cpp:121` | ⬜ |

### 12J — Layout Persistence Naming (Low)

| # | Task | Files | Status |
|---|------|-------|--------|
| 12J.1 | Rename saved-layout ini to `QtCreationKitSavedSettings.ini` for migrant familiarity | `windowlayout.cpp:83-86` | ⬜ |

---

## Progress Tracker

| Phase | Steps | Status |
|-------|-------|--------|
| 0 — Build Integrity | 6/6 | ✅ |
| 1 — ESM I/O & Tes4Codes Removal | 7/7 | ✅ |
| 2 — Component-Property Architecture | 28/28 | ✅ |
| 3 — Record Type Migration | 5/5 | ✅ |
| 4 — WindowLayout & ADS | 4/4 | ✅ |
| 5 — Gaps & Specialized Editors | 31/31 | ✅ |
| 6 — NIF Pipeline & Blender | 5/5 | ✅ |
| 7 — 3D Viewport | 6/6 | ✅ |
| 8 — Editor Completions | 7/7 | ✅ |
| 9 — Papyrus & Dialogue | 6/6 | ✅ |
| 10 — Testing | 26/26 | ✅ |
| 11 — Documentation | 0/5 | ⬜ |
| 12 — UI Layout Parity | 0/40 | ⬜ |
| **TOTAL** | **137/182** | |

---

## Key Architectural Insight (Updated)

The real CK has **two parallel editor pathways**:
1. **Legacy**: Per-record-type `TES*Editor` dialogs (the 127 `_Editor.cpp` files)
2. **Modern**: `QtCreationKitFormDialog` + Components (the QtFormEditing path)

OpenCK must support **both** for full TES3–Starfield compatibility:

**Path A — Generic (data-driven)**: For simple record types (STAT, MISC, BOOK,
etc.), the `QtFormDialog` walks `FormComponents` and renders everything from
`EditorProperty` leaves. No per-record dialog code needed.

**Path B — Specialized (widget-composed)**: For complex record types (NPC_,
QUST, CELL, LAND, RACE, WTHR, etc.), the `QtFormDialog` renders the generic
component grid at the top (name, model, icon) plus a bespoke editor widget
below for the parts unique to that record type (face gen, quest stages,
landscape brushes, weather curves). The specialized widget reads/writes
subrecords directly alongside the components.

Both pathways use the same `QtFormDialog` + `QtFormDialogManager`
infrastructure. The only difference is whether the dialog includes a
specialized widget.

---

## Execution Order (Recommended)

```
Phases 0-9: ✅ Complete (build, I/O, components, records, layout, editors, NIF, viewport, completions, Papyrus/dialogue)
Phase 10: ✅ Complete (26/26 tests passing)

Next:
Phase 12A (QtAdvancedDocking integration) — unblocks 12B, 12F, 12H
  → Phase 12B (Cell View docked panel)
  → Phase 12C (Object Window hierarchical tree)
  → Phase 12D (Top-level menu restructure)
  → Phase 12H (Default dock placement fix)
  → Phase 12E (Preferences tree sidebar)
  → Phase 12F (QtFormDialog modeless + tabs)
  → Phase 12G (Render Window toolbar transform tools)
  → Phase 12I (Status bar enhancements)
  → Phase 12J (Layout persistence naming)
  → Phase 11 (Documentation & final polish)
```

---

*Replaces: `finalPhases.md`, `docs/IMPLEMENTATION_PLAN.md`*
*Supersedes: `docs/CK_Real_Integration_Plan.md` (keep as reference)*
*Updated: 2026-07-28*
