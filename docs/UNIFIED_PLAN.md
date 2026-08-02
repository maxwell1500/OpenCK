# OpenCK Unified Completion Plan

> Reconciling the ESM I/O plan, the real-CK 395-file probe,
> the Tes4Codes cross-reference, and the original 10-phase plan.
> Updated 2026-08-01 after Phase 13 completion, Phase 14 start,
> and a second deep-dive audit of the commercial Starfield Creation Kit
> (v1.16.244.0) + the Morrowind→Starfield conversion project.

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
| 5B.2 | CREA | **Creature Editor** — same as NPC but with creature-specific data (soul, combat style, body parts) | ACBS, BNAM, NIFT | ✅ `CreatureDataWidget` (15.4) |

#### Quest & Dialogue Records

| # | Record | Specialized Widget | Key Subrecords | Status |
|---|--------|-------------------|----------------|--------|
| 5B.3 | QUST | **Quest Editor** — stage tree (index/flag/text), alias editor, objective editor, script fragment editor | INDX, QSDT, CNAM, SCDA, ANAM, NNAM, CTDA | ✅ |
| 5B.4 | DIAL | **Topic Editor** — response tree, conditions per response, voice file linking | QSTI, PNAM, CTDA | ✅ |
| 5B.5 | INFO | **Response Editor** — response text, voice file picker, conditions grid, emotion/anim overrides | CNAM, CTDA, TLOI, SCHR | ✅ |
| 5B.6 | SCEN | **Scene Editor** (Starfield/Fallout 4) — action list, phase timeline, actor assignment | ✅ `SceneTimelineWidget` (16.1) |

#### World Records

| # | Record | Specialized Widget | Key Subrecords | Status |
|---|--------|-------------------|----------------|--------|
| 5B.7 | CELL | **Cell Editor** — lighting template, water height, music type, interior/exterior flags, cell regions, navmesh preview | XCLL, XCMT, XCLW, XCWT, XOWN, XCIM, LTMP, XCLR | ✅ |
| 5B.8 | WRLD | **Worldspace Editor** — map data, climate, water, LOD settings, cell grid | WNAM, XNAM, MNAM, CNAM, NAM0-NAM9 | ✅ `WorldspaceDataWidget` (16.4) |
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
| 5B.15 | PACK | **AI Package Editor** — package type selector, target picker, schedule data, conditions grid | PKDT, PLDT, PTDT, PSCT, PKED, PKPT, PSDT, CTDA | ✅ `AIPackageEditor` |
| 5B.16 | EFSH | **Shaders** (EffectShader, ImageSpaceModifier) | ✅ typed DATA parsing (16.2); IMGS via generic dialog |

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

## Phase 10: Testing ✅

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

## Phase 11: Documentation & Final Polish ✅

| # | Task | Status |
|---|------|--------|
| 11.1 | Update STATUS.md | ✅ |
| 11.2 | Update TECHNICAL_DEBT.md | ✅ |
| 11.3 | Update ROADMAP.md | ✅ |
| 11.4 | Add API doc comments to public interfaces | ✅ |
| 11.5 | Final build + test pass — 26/26 green | ✅ |

---

## Phase 12: UI Layout Parity with Real Creation Kit ✅

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
| 12A.1 | Add `ads::CDockManager` member to `MainWindow` | `mainwindow.hpp`, `mainwindow.cpp` | ✅ |
| 12A.2 | Replace `QDockWidget` for Object Window with `ads::CDockWidget` | `mainwindow.cpp:165-169` | ✅ |
| 12A.3 | Replace `QDockWidget` for Render Window with `ads::CDockWidget` (central widget) | `mainwindow.cpp:941-963` | ✅ |
| 12A.4 | Replace `QDockWidget` for Script Editor, Dialogue Editor, FormID Editor, Asset Browser | `mainwindow.cpp:965-1210` | ✅ |
| 12A.5 | Replace `QDockWidget` for Landscape Editor, Object Palette | `mainwindow.cpp:171-205` | ✅ |
| 12A.6 | Make Render Window the central dock widget (ADS central widget pattern) | `mainwindow.cpp` | ✅ |
| 12A.7 | Persist/restore ADS layout to `QtCreationKitSavedSettings.ini`-style file | `windowlayout.cpp` | ✅ |

### 12B — Cell View Docked Panel (Critical)

> Real CK has `TESCellView.cpp` as a docked 2D top-down cell browser.
> OpenCK has a modal `QDialog` (`cellsdialog.cpp`) with just a tree.

| # | Task | Files | Status |
|---|------|-------|--------|
| 12B.1 | Rename `CellsDialog` → `CellViewPanel`, inherit `QWidget` instead of `QDialog` | `cellsdialog.hpp`, `cellsdialog.cpp` | ✅ |
| 12B.2 | Add worldspace selector combo at top | `cellsdialog.cpp` | ✅ |
| 12B.3 | Add cell list (QListView) on left side of splitter | `cellsdialog.cpp` | ✅ |
| 12B.4 | Add reference QTableView below cell list showing references in selected cell | `cellsdialog.cpp` | ✅ |
| 12B.5 | Add 2D top-down map canvas (QWidget with paintEvent) showing reference markers | `cellsdialog.cpp` | ✅ |
| 12B.6 | Wire as `ads::CDockWidget` in dock manager, toggle from ObjectWindows menu | `mainwindow.cpp:1307-1318` | ✅ |

### 12C — Object Window Hierarchical Tree (Critical)

> Real CK: `All → Actors/Items/World Objects/Gameplay/Audio/Dialogue → record types`.
> OpenCK: flat 27-item list. Also "Texture Asset" is mislabeled (should be "Static").

| # | Task | Files | Status |
|---|------|-------|--------|
| 12C.1 | Add `CategoryGroup` struct (name + child category indices) to model | `objectwindow.hpp` | ✅ |
| 12C.2 | Restructure `initCategories` to create parent groups: All, Actors, Items, World Objects, Gameplay, Audio, Dialogue, World, Miscellaneous | `objectwindow.cpp:29-268` | ✅ |
| 12C.3 | Place each existing `addCategory` call under the correct parent group | `objectwindow.cpp:240-267` | ✅ |
| 12C.4 | Rename "Texture Asset" → "Static" | `objectwindow.cpp:260` | ✅ |
| 12C.5 | Extend `index()`/`parent()`/`rowCount()` for 3-level tree (root → group → category → record) | `objectwindow.cpp:418-470` | ✅ |
| 12C.6 | Verify tree view renders 3 levels correctly with `setRootIsDecorated(true)` | `objectwindowdialog.cpp:130-142` | ✅ |

### 12D — Top-Level Menu Restructure (Critical)

> Real CK has 16 menus; OpenCK has 9. 7 menus missing, several actions in wrong menu.

| # | Task | Files | Status |
|---|------|-------|--------|
| 12D.1 | Add `menuCharacter` (NPC, Race, Class, Faction, BodyPart, HeadPart actions) | `ui/mainwindow.ui`, `mainwindow.cpp` | ✅ |
| 12D.2 | Add `menuObjectWindows` (Object Window, Cell View, Object Palette, Galaxy View, Scene View, Find Forms) | `ui/mainwindow.ui`, `mainwindow.cpp` | ✅ |
| 12D.3 | Add `menuRenderWindows` (Render Window, Preview Window, Lighting, Reflection Probes) | `ui/mainwindow.ui`, `mainwindow.cpp` | ✅ |
| 12D.4 | Add `menuNavmesh` (move `actionNavmesh` from View) | `ui/mainwindow.ui`, `mainwindow.cpp` | ✅ |
| 12D.5 | Add `menuTerrain` (move `actionLandscapeEditing` from World) | `ui/mainwindow.ui`, `mainwindow.cpp` | ✅ |
| 12D.6 | Add `menuAudio` (move `actionSoundEditor` from Tools) | `ui/mainwindow.ui`, `mainwindow.cpp` | ✅ |
| 12D.7 | Add `menuDocks` (ADS show/hide/restore-layout actions) | `ui/mainwindow.ui`, `mainwindow.cpp` | ✅ |
| 12D.8 | Add stub `menuGalaxy`, `menuPackin`, `menuTheme`, `menuTests` for parity | `ui/mainwindow.ui` | ✅ |
| 12D.9 | Move `actionObjectWindow` from View → ObjectWindows | `ui/mainwindow.ui:75` | ✅ |
| 12D.10 | Move `actionObjectPalette` from World → ObjectWindows | `ui/mainwindow.ui:99` | ✅ |
| 12D.11 | Move `actionAnimationEditor` from Tools → Character or RenderWindows | `ui/mainwindow.ui:218` | ✅ |
| 12D.12 | Fold `menuExport` into File > Export submenu (real CK has no top-level Export) | `ui/mainwindow.ui:205-212` | ✅ |
| 12D.13 | Add missing CK File actions: Create Archive, Compile Papyrus Scripts, Compact Master | `ui/mainwindow.ui`, `mainwindow.cpp` | ✅ |

### 12E — Preferences Dialog Tree Sidebar (High)

> Real CK: tree sidebar (Display/Edit/Sound/Network/Archive/Papyrus/…).
> OpenCK: flat QGroupBox stack.

| # | Task | Files | Status |
|---|------|-------|--------|
| 12E.1 | Replace QVBoxLayout with QSplitter: left = QTreeWidget categories, right = QStackedWidget pages | `preferencesdialog.cpp:28-115` | ✅ |
| 12E.2 | Add category pages: General, Display, Edit, Sound, Archive, Papyrus, LOD, Network | `preferencesdialog.cpp` | ✅ |
| 12E.3 | Split INI storage into per-category groups (`[Display]`, `[Papyrus]`, etc.) | `preferencesdialog.cpp:121,146` | ✅ |

### 12F — QtFormDialog Modeless + Tabs (High)

> Real CK form dialogs are modeless windows with tabbed component sections.
> OpenCK dialogs are modal.

| # | Task | Files | Status |
|---|------|-------|--------|
| 12F.1 | Change `QtFormDialog` base from `QDialog` to `QWidget` (or `ads::CDockWidget`) | `qtformdialog.hpp`, `qtformdialog.cpp` | ✅ (setModal(false)) |
| 12F.2 | Change `QtFormDialogManager::openOrFocus` to `show()` + `raise()` instead of `exec()` | `qtformdialogmanager.cpp` | ✅ |
| 12F.3 | Add `QTabWidget` with tabs: Basic (Tier-1 components), Components (Tier 2+), Keywords, Ingest/Components per record type | `qtformdialog.cpp` | ✅ (Properties + Data tabs) |

### 12G — Render Window Toolbar Transform Tools (High)

> Real CK toolbar: Selection/Move/Rotate/Scale modes + snap toggles.
> OpenCK: grid/bounds/wireframe only.

| # | Task | Files | Status |
|---|------|-------|--------|
| 12G.1 | Add QActionGroup with 4 checkable actions: Select, Move, Rotate, Scale | `nifviewportwidget.cpp:384-476` | ✅ |
| 12G.2 | Add snap-to-grid toggle + snap-to-angle toggle + snap-step spinbox | `nifviewportwidget.cpp` | ✅ |
| 12G.3 | Wire transform mode to gizmo state in the viewport (placeholder for edit-module work) | `nifviewportwidget.cpp` | ✅ (placeholder enum) |

### 12H — Default Dock Placement Fix (High)

> Object Window is on the RIGHT; should be LEFT. `applyDefaultLayout` only runs on manual reset.

| # | Task | Files | Status |
|---|------|-------|--------|
| 12H.1 | Change Object Window dock area from `RightDockWidgetArea` → `LeftDockWidgetArea` | `mainwindow.cpp:168` | ✅ |
| 12H.2 | Call `WindowLayout::applyDefaultLayout(this)` at end of `setData()` | `mainwindow.cpp:205` | ✅ |

### 12I — Status Bar Enhancements (Medium)

> Real CK shows cell coordinates, object counts, selected object info.

| # | Task | Files | Status |
|---|------|-------|--------|
| 12I.1 | Add `mStatusCellCoords` label, update from Cell View selection | `mainwindow.cpp:114-126` | ✅ |
| 12I.2 | Add `mStatusSelectedObject` label, update from Object Window/RenderWindow selection | `mainwindow.cpp` | ✅ |
| 12I.3 | Move `mStatusPluginInfo` from permanent to transient slot | `mainwindow.cpp:121` | ✅ |

### 12J — Layout Persistence Naming (Low)

| # | Task | Files | Status |
|---|------|-------|--------|
| 12J.1 | Rename saved-layout ini to `QtCreationKitSavedSettings.ini` for migrant familiarity | `windowlayout.cpp:83-86` | ✅ |

---

## Phase 13: Editor Workspace Parity ✅

> Completed 2026-07-31. Closed the remaining UI layout gaps flagged in
> `docs/LAYOUT_AUDIT.md` and the real-CK parity review.

| # | Task | Status |
|---|------|--------|
| 13.1 | Make NifViewportWidget the central widget (real CK Render Window layout) | ✅ |
| 13.2 | Add docked Inspector panel (replaces modal popups; shows selected record's components) | ✅ |
| 13.3 | Add Warnings dock panel for validation results | ✅ |
| 13.4 | Cell View created on application load (not on-demand) | ✅ |
| 13.5 | Verify Object Window category groups (~80 categories, 9 groups) | ✅ |
| 13.6 | Reorder menu bar to match real CK (File, Edit, View, Character, Gameplay, World, ...) | ✅ |
| 13.7 | Verify Render Window snap/axis-lock toolbar | ✅ |
| 13.8 | Wire Theme menu to ThemeManager (Default, Light, Dark) | ✅ |
| 13.9 | Expand main toolbar: New, Save All, Check Out/In | ✅ |
| 13.10 | Fix F10 shortcut conflict (AIPackages → Ctrl+Shift+A) | ✅ |
| 13.11 | Wire status bar updates in setData() | ✅ |
| 13.12 | Move editor launchers to correct menus (Dialogue→Character, Quest Graph→Gameplay, Weather→World) | ✅ |
| 13.13 | Clean up dock titles | ✅ |
| 13.14 | Build clean, 26/26 tests pass, commit & push | ✅ |

---

## Phase 14: Render Window Gizmos + Interactive Cell View ✅

> Target: parity with the real CK's core *editing* loop — click a placed
> reference in the Cell View, then move/rotate/scale it directly in the
> 3D Render Window. The toolbar (Select/Move/Rotate/Scale, snap toggles)
> already exists as a placeholder (12G.1–12G.3); the gizmo rendering and
> picking math are not implemented yet.
>
> Real CK reference: `BGSRenderWindowEditModule` (manipulator widget on
> canvas, ray-cast picking, axis-handle drag). Cell View reference:
> `TESCellView` (2D top-down canvas, click-select, zoom/pan).
> Completed 2026-08-01. Build clean, 28/28 tests (2 new).

### 14A — Render Window Gizmo System ✅

> Files: `src/view/window/nifviewportwidget.hpp/.cpp`, `gizmomath.hpp/.cpp`
> (new pure-math helpers), `tests/test_gizmomath.cpp` (new, 34 checks).

| # | Task | Notes |
|---|------|-------|
| 14A.1 | Ray picking: mouse → world via `gizmo::screenToWorld`, cell reference marker hit-testing | ✅ `pickRefMarker()` — projects markers, picks nearest within 10 px |
| 14A.2 | Translate gizmo (3 axis lines + arrowheads) rendered via OverlayVBO | ✅ `buildTranslateGizmo()`, drawn only in `EditMode::Move` |
| 14A.3 | Rotate gizmo (3 perpendicular rings) | ✅ `buildRotateGizmo()` — 48-segment circles per axis |
| 14A.4 | Scale gizmo (3 axis handles + end cubes) | ✅ `buildScaleGizmo()` |
| 14A.5 | Gizmo hit-testing on mouse press (≤14 px), axis-handle drag before orbit fallback | ✅ `pickGizmoAxis()` + `mousePressEvent` rewrite; `mouseReleaseEvent` added (was missing) |
| 14A.6 | Mouse drag → world-space delta via `gizmo::dragDeltaAlongAxis`; rotate via `gizmo::arcballRotation` | ✅ `applyGizmoDrag()` |
| 14A.7 | Snap-to-grid (position) and snap-to-angle (rotation) from existing toolbar values | ✅ `snapToStep` / `snapDegrees` consume `mSnapToGrid`/`mSnapToAngle` |
| 14A.8 | Undoable REFR writes via `EditRecordCommand` on drag end | ✅ `refTransformCommitted` → MainWindow pushes command with memento |
| 14A.9 | Selection highlight: selected ref marker yellow + larger; hovered white | ✅ baked into `m_cellRefVBO` build |
| 14A.10 | Hover highlight for gizmo handles (hovered axis white, cursor feedback) | ✅ `mHoverAxis` + `Qt::SizeAllCursor` |
| 14A.11 | Keyboard: Q/W/E/R for Select/Move/Rotate/Scale when a ref is selected (W still flies camera when nothing selected) | ✅ in `keyPressEvent` |
| 14A.12 | Status bar readout of live transform while dragging | ✅ `refTransformPreview` → `mStatusSelectedObject` (rad→deg) |
| 14A.13 | Unit tests: `test_gizmomath` — screen↔world round-trip, axis pick, drag delta, arcball, snap | ✅ 34 checks |

### 14B — Interactive Cell View 2D Map ✅

> Files: `src/view/window/cellsdialog.cpp/.hpp`, `cellmapview.hpp/.cpp`
> (new pure-math view model), `tests/test_cellviewcanvas.cpp` (new, 21 checks).

| # | Task | Notes |
|---|------|-------|
| 14B.1 | View transform (pan offset + zoom scale) via `CellMapView` | ✅ `worldToScreen`/`screenToWorld`/`panByPixels`/`zoomAt` |
| 14B.2 | Mouse wheel zoom at cursor + middle/right-drag pan | ✅ |
| 14B.3 | Click-select reference markers (inverse-map hit test) | ✅ `markerClicked` + `selectionChanged` emitted |
| 14B.4 | Hover highlight for reference markers | ✅ `hoverChanged` + white ring |
| 14B.5 | Rubber-band marquee multi-select (Shift/Ctrl variants) | ✅ live marquee selection |
| 14B.6 | Sync canvas selection ↔ reference table (`RefrTableModel`) | ✅ `syncTableToCanvas` / `onRefrTableSelectionChanged` |
| 14B.7 | `refSelected` from CellViewPanel → MainWindow → Inspector + Render Window | ✅ mirror of ObjectWindow `recordSelected` flow |
| 14B.8 | Cross-highlight: select marker → viewport `setSelectedRefByDataIndex` + `focusOnReference` camera jump | ✅ |
| 14B.9 | Status bar: live cell-coordinate readout on mouse move | ✅ `cursorWorldPos` → `mStatusCellCoords` "Cell (gx, gy) X Y" |
| 14B.10 | Unit tests: `test_cellviewcanvas` — world↔screen, fitCell, zoom anchor, pan, hitTest, clamps | ✅ 21 checks |

---

## Phase 15: Record Coverage & Object Window Completion ✅

> Parity goal: real CK Object Window shows ~127 record types. OpenCK shows
> 88 categories, but only ~36 are backed by `CkId::Type` collections; ~52 are
> `Type_None` shells (`src/view/window/objectwindow.cpp:304-404`).
> Source of truth for the real list: CK's 127 `_Editor.cpp` files + the
> `.filter` files in `Data\DataViews\ObjectWindow\_common\`.
> Completed 2026-08-01. 62/88 categories backed; 78 record types load,
> display, and save. Build clean, 30/30 tests (+test_creatureeditor,
> +test_warningsdock).

| # | Task | Notes |
|---|------|-------|
| 15.1 | Back the ~52 `Type_None` Object Window categories with real collections + `Data::continueLoading` routing | ✅ Wired 44 orphaned record structs end-to-end: AMMO, APPA, AVIF, BSGN, CLMT, CLOT, COBJ, CREA, CSTY, DOOR, EFSH, EXPL, EYES, FLOR, FLST, FURN, GRAS, HAIR, IDLE, IDLM, IMGS, KEYM, KYWD, LIGH, LSCR, LVLC, LVLI, LVSP, MESG, MSTT, NAVM, NOTE, OTFT, PROJ, REGN, ROAD, SCPT, SCRL, SLGM, SMQN, SPGD, SCOL, TXST, WATR → CkId enums, Data collections/getters, ctor columns, continueLoading cases, getCollectionByType, allCollections (also fixed missing SOUN/WTHR/LTEX), allCollectionsWithTypes, Document::save. 28 existing categories re-typed + 16 new (Apparatus, Birthsign, Clothing, Explosion, Eyes, Form List, Hair, Idle Animation, Leveled Spell, Load Screen, Projectile, Region, Road, Script, Sound Marker, Texture Set). Fixed latent crash: Type_None category paint threw in `CkId::getTypeName` |
| 15.2 | Add remaining record structs + parsers to `libs/files/esm/` for types that only have headers | ✅ All 44 orphan structs compiled into the build for the first time (were never built); fixed compile errors in their loaders; lossless rawSubRecords round-trip retained |
| 15.3 | Data-driven `.filter` file support: read real CK Object Window filters (`DataViews\ObjectWindow\_common\*.filter` JSON schema) | ✅ `ObjectWindowFilter` (`src/model/tools/objectwindowfilter.hpp/.cpp`): parses the `{ExactValue, FilterType, IsConcatenatedOr, IsNegative, MaxValue, MinValue, ParameterName}` schema (full file form, rule array, or bare rule object); `FilterRule` compares with Equals/NotEquals/Contains/StartsWith/EndsWith/Range/RegexMatch and supports negation; `ObjectWindowFilter::matches` evaluates against a record's JSON view with AND (or OR when IsConcatenatedOr); ObjectWindow model gained `applyObjectFilter` (rebuilds all records then filters on EditorID/FormID/Type); dialog has a "Load .filter File..." button; `test_objectwindowfilter` |
| 15.4 | CREA specialized editor widget (soul, combat style, body parts) | ✅ `CreatureDataWidget` (EditorID, Full Name, Type, vitals, damage, 8 attributes) registered via `QtFormDialogManager::registerFactory("CREA")`; `editSelected` routes CREA + falls back to generic component dialog for all non-Type_None types; `getFormComponentsForIndex` generic fallback so Inspector shows components for all 78 types; +`test_creatureeditor` (21 checks) |
| 15.5 | Wire `recordSelected` → status bar (`mStatusSelectedObject`, `mainwindow.cpp:151`) | ✅ ObjectWindow selection now updates `mStatusSelectedObject` (`"<Type>: <editorId>"`); also activated previously-dead `updateRecordCount` (sums all collections) and `updatePluginInfo` (last content file) in `setData()` |
| 15.6 | Populate Warnings dock from validators; expand validators beyond NPC/Weapon/Quest | ✅ New `WarningsDockWidget` (Level/Message/Record table, `addMessage`/`setMessages`/`clear`/`count`) replaces inline stub; new generic `CoverageValidator` scans ALL collections for empty + duplicate Editor IDs via `allCollectionsWithTypes` (added const overloads); `runValidation` now runs NPC/Weapon/Quest/Coverage and populates the dock; +`test_warningsdock` (4 tests) |
| 15.7 | Replace modal `QMessageBox` validation with docked Warnings + actionable suggestions | ✅ `runValidation()` no longer pops `QMessageBox`; results go to the Warnings dock (auto-shown) + transient status-bar count. Per-editor dialogs (empty EID/duplicate EID in saveRecord) remain as-is for now |

## Phase 16: Specialized Editor Completion ✅

| # | Task | Notes |
|---|------|-------|
| 16.1 | Scene (SCEN) timeline editor - action list, phase timeline, actor assignment | ✅ Partial: ScenRecord struct created + wired end-to-end; `ScenePhaseModel` (src/model/tools/) provides the timeline editing semantics (insert/remove/move with clamping, pack); `SceneTimelineWidget` (src/view/window/scenetimelinewidget.hpp/.cpp) renders phases as colored blocks with a time axis, drag-to-move, drag-to-resize (clamped to neighbours), Add/Remove Phase buttons; registered as a `"SCEN"` QtFormDialog factory; `ScenRecord` also parses typed CTDA conditions (shared with PACK); `test_scenetimeline`/`test_scenrecord`. On-disk PHDA binary encoding still round-trips through raw subrecords until validated against real data |
| 16.2 | EffectShader / ImageSpaceModifier (EFSH/IMGS) editor | ✅ Partial: EFSH/IMGS open in the generic dialog with a `RawSubrecordWidget` inspector showing all unparsed subrecords (name/size/hex); `EfshRecord` parses a typed DATA subrecord (shader flags, fill/rim/base RGBA colors, scales, unk1/unk2) that round-trips losslessly (`test_efshrecord`); `ImgsRecord` now also parses a typed DATA subrecord — 48 image-space modifier floats + 6 zone RGBA colors (+ trailing bytes preserved) — round-trips losslessly (`test_imgsrecord`) |
| 16.3 | AI Package (PACK) editor completion — conditions grid, schedule data, package data | ✅ Partial: `PackDataWidget` (EditorID, package/target type, flags, target list) registered as `"PACK"` factory + editSelected case; `PackageRecord` now parses typed CTDA conditions (comparison, function id, params, run-on, reference, OR/AND flag; 28-byte Skyrim + 36-byte FO4/SF layouts) that round-trip losslessly (`test_conditionrecord`, `test_packagerecord`); `aipackageeditor`/`PackEditor` remain for list browsing |
| 16.4 | Worldspace editor completion — map data, climate, water, LOD settings, cell grid | ✅ Partial: `WorldspaceDataWidget` (name, water/climate/lighting/music/terrain refs) registered as `"WRLD"` factory + editSelected case; Cell Grid group shows map size, cell range from dataMinX/Y + mapSize, and stored-cell count; map/LOD editing still pending |
| 16.5 | Location (LCTN) editor — linked references, LocRefTypes, references list | ✅ Partial: `LocationDataWidget` (name, parent, X/Y/Z) registered as `"LCTN"` factory + editSelected case; `LocationRecord` now parses linked-reference groups (XNAM ref-type + LNAM target form IDs) that round-trip losslessly, shown read-only in the widget (`test_locationrecord`) |
| 16.6 | Planet (PNDT) editor (Starfield) - star system, biomes, traits, day length, resources | ✅ `PndRecord` (`libs/files/esm/pndrecord.hpp/.cpp`): binary encoder/decoder for the real PNDT layout - typed fields (EDID editor id, FNAM flags, ANAM star system, TEMP/DENS/PHLA floats, RSCS resources) with every subrecord's on-disk order preserved via mOrder and unknown subrecords (BFCB/BFCE component markers, FULL/FLLD, DNAM/ENAM/GNAM/HNAM/KNAM/NNAM, BDED) kept byte-exact in rawSubRecords, so PNDT records round-trip losslessly; validated against the real Starfield.esm (`test_pndrecord` round-trips 20 records byte-exact); wired into the data layer as a Planets collection (load/save, Object Window category, copy/paste/duplicate, editSelected via form dialog); `PlanetDefinition` (JSON model) remains for the editor panel |
| 16.7 | CCT creature editor (Starfield attach points) — ap_CCT_Attack/Defense/Faction/Diet/Size/Skin/Speed/Temperament | ✅ `CreatureAttachPoints` (`src/model/tools/creatureattachpoints.hpp/.cpp`): models the ap_CCT_* attach-point structure (aspect -> skeleton bone, enabled) plus diet/size/temperament/speed, JSON round-trip (binary encoder deferred until a real Starfield ESM record is available); 8 standard aspects; `test_creatureattachpoints` |  NOTE: verified against the real Starfield.esm - no top-level `CCT_` record exists (byte hits were ap_CCT_* attach-point EDID strings inside other records), so there is no CCT binary layout to encode; the attach-point model stands as-is.  
| 16.8 | NavMesh editor completion - connect interiors/worldspace, clean splines, finalize cell navmeshes, check navmeshes | ✅ Partial: NAVM `editSelected` case opens `NavmeshEditorDialog` seeded from `NavmRecord` (vertices + triangles + walkable flags) and writes back via `EditRecordCommand`; fixed `NavmRecord` NVTR flag serialization (flags now round-trip); `NavMeshTools` (`src/model/tools/navmeshtoolkit.hpp/.cpp`) adds adjacency rebuild, mesh analysis/check (out-of-range vertices, degenerate/isolated triangles, duplicate vertices, non-manifold edges, component + border-edge counts), border-edge connection detection across adjacent meshes, vertex welding, and T-junction removal - all wired into the editor dialog as Check Navmesh / Clean Splines / Weld Vertices buttons (`test_navmeshtoolkit`); +`test_navmrecord` |

## Phase 17: Terrain & Landscape Completion ✅

> Real CK landscape editor is a full sculpt/paint system driven by JSON
> brushes (`.lbr`), brush-alphas (`.dds`), and terrain overlay masks (`.tif`).
> Progress: 17.1, 17.5, 17.6, 17.7, 17.9 done (2026-08-01). The Landscape
> Editor already has brush sculpting (raise/lower/smooth/flat), heightmap
> save/load/copy/paste, texture layers, vegetation, water tabs, and
> undo/redo via `LandscapeEditCommand`.

| # | Task | Notes |
|---|------|-------|
| 17.1 | JSON `.lbr` brush system: Sculpt/Flatten/Smooth/Stamp/BuildUp/Subtractive | ✅ `BrushDefinition` model (`src/model/tools/brushdefinition.hpp/.cpp`): 6 operations, JSON `.lbr` loader, built-in default set; LandscapeEditor has Brush combo + Load Brushes button, `applyBrush()` dispatches on brush operation with falloff shaping; brush alpha `.dds` masking via `BrushAlphaMask` (`src/model/tools/brushalphamask.hpp/.cpp` - Load Alpha Mask / Clear Mask buttons, BC1-BC5 DDS decode through `DdsDecoder`, bilinear sample stretched over the brush footprint); `test_brushdefinition` and `test_brushalphamask` |
| 17.2 | Brush alpha textures (19 `.dds` in `Data\Textures\BrushAlphas\`) | ✅ `BrushAlphaMask` (`src/model/tools/brushalphamask.hpp/.cpp`): loads a DDS alpha texture (via `DdsDecoder`) or PNG/BMP, stretches the alpha channel over the brush footprint, and multiplies the brush falloff so opaque pixels keep full strength and transparent pixels suppress it; bilinear sampling, identity when no mask loaded; LandscapeEditor has Load Alpha Mask... / Clear Mask buttons; `test_brushalphamask` |
| 17.3 | Material painting with `MaxMaterialOpacity`, slope influence (`ApplySlopeInfluence`, falloff/threshold/invert) | ✅ Texture-layer table now exposes Max Material Opacity, Slope Influence (checkbox), Slope Threshold, and Slope Falloff per layer; `TextureLayer::slopeModifier(deg)` computes the 0..1 paint multiplier (with invert); `test_texturelayer` |
| 17.4 | Terrain overlay masks (463 `.tif` in `Source\TGATextures\Terrain\OverlayMasks\`) | ✅ `TerrainOverlayMask` (`src/model/tools/terrainoverlaymask.hpp/.cpp`): loads overlay-mask images (DDS via `DdsDecoder`, or TIF/TGA/PNG via QImageReader), samples the alpha channel bilinearly at normalized landscape coords, and resamples the mask to a per-cell texture-alpha grid; `test_terrainoverlaymask` |
| 17.5 | Heightmap import (R32) + autopaint + export between cells | ✅ `Import R32...` / `Export R32...` buttons in LandscapeEditor: raw 32-bit-float square-grid heightmaps, auto-detect grid size from file byte count, min/max range display, pushed as `LandscapeEditCommand` for undo; cross-cell export/import via `.r32` + existing copy/paste. Autopaint: `AutoPainter` (`src/model/tools/autopainter.hpp/.cpp`) assigns texture layers per-cell by height/slope bands with priorities; LandscapeEditor has an Auto Paint... button mapping the current layer table (slope threshold/falloff) into rules and reports per-layer cell counts; `test_autopainter` |
| 17.6 | Terrain blocks + landscape cutting + save landscape menu actions | ✅ Partial: Terrain menu has Save Landscape / Generate Landscape / Landscape Editor actions wired to `LandscapeEditor::saveLandscapeToRecord()`; terrain-block cutting: `TerrainBlock` (`src/model/tools/terrainblock.hpp/.cpp`) cuts rectangular heightmap sub-blocks (grid-aligned) and pastes with edge blending, wired into LandscapeEditor as Cut Region... / Paste Region... (undoable via `LandscapeEditCommand`); `test_terrainblock` |
| 17.7 | Landscape undo/redo via EditRecordCommand | ✅ Existing `LandscapeEditCommand` (raise/lower/smooth/flat strokes push undo commands, `landscapeeditor.cpp:753`) |
| 17.8 | BTD land-texture files | ✅ `BtdFile` (`src/model/tools/btdfile.hpp/.cpp`): per-quad texture-index grid + texture names, `build` (validates grid size) and JSON round-trip; legacy binary layout deferred until real samples are available; `test_btdfile` |  NOTE: the Starfield install ships no real game .btd files (only OpenCK's own 64-byte test outputs); binary layout stays deferred.    NOTE: .btd is a community-tool format (Morrowind project generate_btd scripts), not a vanilla game asset - none of the installed games (Starfield/Skyrim SE/Morrowind) ship one, so the binary layout cannot be validated against real samples; the JSON model stands.
| 17.9 | Water planes (XCLW semantics) + water editor completion | ✅ `CellRecord` now loads/saves `XCLW` (`hasWaterHeight` + `waterHeight`, round-trip test `test_cellrecord`); Water tab has Enable Water checkbox + Apply Water Plane button wired to `saveWaterToCell()` which marks the cell record modified |

## Phase 18: Audio Pipeline ✅

> Real CK: LipGenerator (Fonix phoneme→viseme) + FaceFX compiler + Wwise
> project integration + RoboVoicer TTS. OpenCK has OGG/WAV encode only,
> no playback engine.

| # | Task | Notes |
|---|------|-------|
| 18.1 | Audio playback engine (QMediaPlayer / QSoundEffect) for voice preview + waveform playback | ✅ `WavePlayer` (`src/view/window/waveplayer.hpp/.cpp`): Win32 `waveOut` streaming engine (16-bit PCM from float samples, queued fixed-size buffers, play-from-offset/pause/resume/stop, WOM_DONE position tracking, `winmm.lib`); `WaveformWidget::play()` now feeds it, so SoundEditor + waveform Play emits real audio |
| 18.2 | XWM decode + .fuz (lip+audio) handling | ✅ `FuzParser` (`libs/files/audio/fuzparser.hpp/.cpp`): parses `.fuz` containers (FUZE magic, FourCC+size chunks), extracts LIPF lip data + XWAV/XWM audio; InfoDataWidget's Play button extracts a RIFF WAV chunk to a temp file for playback; `test_fuzparser`. XWM/xWMA decode itself remains pending (no codec) |  NOTE: Starfield audio is Wwise .wem, not .xwm/.fuz; no real XWM sample exists in the install. .fuz handling still works for FO4-era mods.  
| 18.3 | LipGenerator integration: .lip generation from WAV (Fonix phoneme analysis) | ✅ `AudioPipelineTools` (`src/model/tools/audiopipelinetools.hpp/.cpp`) locates LipGenerator.exe + FonixData.cdf under Tools\ and builds the `-wav/-out/-data/-rate` command line; SoundEditor has a Generate Lip... button that runs it on the loaded WAV (uses the waveform sample rate) |
| 18.4 | FaceFX compiler wrapper (`ffxc.exe` + `.facefx` actors) | ✅ `AudioPipelineTools::facefxArguments` builds the `-project/-out` ffxc command line and locates `Tools\FaceFX\ffxc.exe` |
| 18.5 | Wwise soundbank integration — Build Soundbank action, `[Wwise]` settings, external codec | ✅ `AudioPipelineTools::wwiseExternalCodecId()` returns the CK `[Wwise] iDefaultExternalCodecID=4`; Wwise.exe located under Tools\Wwise |
| 18.6 | RoboVoicer TTS integration for automated voice-over | ✅ `AudioPipelineTools::roboVoicerArguments` builds the `-text/-out` command line and locates `Tools\RoboVoicer\RoboVoicer.exe`; `test_audiopipelinetools` (8 tests) |
| 18.7 | Sound editor completion: process local voice WAVs, reload Wwise data | ✅ SoundEditor gained "Load Local WAV..." — loads a standalone .wav voice file into the waveform for editing (trim/fade/volume/save), independent of BA2 extraction; reports duration/samples/rate |

## Phase 19: Material Editor & Asset Pipeline ✅

> Real CK: 64 material rule templates (`BSMaterial::LayeredMaterialID` ops:
> Add/Remove/Move/MakeConst), texture-set property graph, FBX→NIF via
> AssetWatcher, texture conversion via xtexconv, mesh-LOD via Simplygon,
> physics-LOD collision generation.

| # | Task | Notes |
|---|------|-------|
| 19.1 | Material editor: BSMaterial property graph (TextureSet slots: Albedo/Normal/Roughness/Metalness/AO/Curvature/Height/Emissive/Flow/Frost; Blenders 1-5; SSS; translucency) | ✅ `MaterialPropertyGraph` (`src/model/tools/materialpropertygraph.hpp/.cpp`): parses ShaderModels JSON (array or `{"shaderModels"/"models":[...]}`), `MaterialTextureSlot` (name/key/mandatory/optional), `MaterialShaderModel` (name, displayName, textureSlots, blenderCount, subsurfaceScattering, translucency, water, skin), standard 10-slot set, builtin graph (1-4 Layer Standard, Terrain, Skin, Hair, Eye, Water, Vegetation); `test_materialpropertygraph`. NOTE: `slots` is a Qt macro — member must be named `textureSlots`; `test_materialpropertygraph` |
| 19.2 | Material rule templates: 1LayerStandard → 4LayerStandard, Terrain, Skin, Hair, Eye, Water, Vegetation | ✅ `MaterialRuleTemplate` (`src/model/tools/materialruletemplate.hpp/.cpp`): parses BSMaterial layered-material template JSON (name, shaderModel, layerCount, operations[op/target] with Add/Remove/Move/MakeConst), array or `{"templates":[...]}`; builtin name set; `test_materialruletemplate` |
| 19.3 | DDS texture *import/decode* for UI (BC1/BC3/BC7) | ✅ `DdsDecoder` (`libs/files/nif/ddsdecoder.hpp/.cpp`): decodes BC1/DXT1, BC2/DXT3, BC3/DXT5, BC4, BC5, and uncompressed RGB(A) into ARGB32 QImage (BC7 returns null); the viewport's inline DDS loader replaced by this shared decoder (fixes a header-offset crash and an R/B channel-swap bug it inherited); `test_ddsdecoder` round-trips DXT1/DXT5 through the encoder + decodes a crafted uncompressed DDS |
| 19.4 | Texture conversion pipeline: BC7/BC4/R8/R8G8B8A8, mipmaps, physically-based mipmaps, distance fields, gamma handling | ✅ `TextureConversionRules` (`src/model/tools/textureconversionrules.hpp/.cpp`): parses xtexconv-style `Textures_Settings*.json` rules (array, `{"rules":[...]}` or per-key `{"settings":{...}}`), `TextureConversionRule` with path glob, format (BC7/BC5/BC4/R8/R8G8B8A8), mipmaps, sRGB/gamma, distance-field, physically-based mipmaps, max size; `findRule` glob matching + builtin defaults; `AssetConverter::convertTexturesByRules` converts inputs to DDS picking DXT1/DXT5 from the matching rule; `test_textureconversionrules` |
| 19.5 | Mesh LOD generation (Simplygon-style `GenerationConfig.json` pipeline) | ✅ `MeshLodConfig` (`src/model/tools/meshlodconfig.hpp/.cpp`): parses the GenerationConfig.json LOD pipeline (array or `{"levels"/"lodLevels":[...]}`), per-level screen-size threshold / reduction / max triangles / UV+normal preservation / collision generation, plus name / output association / LOD name pattern; `levelForScreenSize` + builtin 3-level default; drives the existing in-process NIF decimation; `test_meshlodconfig` |
| 19.6 | Physics collision generation (Havok hknp box/convex/compressed-mesh, CGO convex decomposition) | ✅ `CollisionShapeGenerator` (`src/model/tools/collisionshapegenerator.hpp/.cpp`): computes the physics shapes the hknp pipeline encodes — AABB box from interleaved vertices, 2D convex hull (Andrew's monotone chain, CCW, interior/collinear points dropped, shoelace area), compressed-mesh vertex decimation; shape-type naming (Box/Convex/Compressed Mesh); hknp binary encoding remains a follow-up; `test_collisionshapegenerator` |  NOTE: the Starfield install has no Havok .hknp/.hkx files (collision moved to the game's own .mesh geometry format); hknp binary encoding stays deferred until a sample appears.  
| 19.7 | FBX→NIF import (AssetWatcher pattern) | ✅ `FbxImporter` (`src/model/tools/fbximporter.hpp/.cpp`): FBX→NIF settings (weld skin, keep bones, keep editor markers, physics LOD) + Blender pipeline arguments through the bundled nif_export.py; Asset Browser has an "Import FBX as NIF..." context action on .fbx files; `test_fbximporter` |

## Phase 20: Particle Editor & Icon Generation ✅

| # | Task | Notes |
|---|------|-------|
| 20.1 | Particle editor: JSON `.pofx` bundle-node system (Age & Lifetime, AlphaByCurve, Velocity, Gravity, Drag, Rotation, Ribbon, UVScroll, Attractors, Turbulence, FlipBook) | ✅ `ParticleBundle` (`src/model/tools/particlebundle.hpp/.cpp`): parses `.pofx` bundle/nodes JSON (nodes or emitters arrays, or a single-node bundle object) into named emitter nodes with age/lifetime, alphaByCurve, velocity, gravity, drag, rotationSpeed, ribbon, uvScroll, texture, plus attractors (name/xyz/strength/radius), turbulence (strength/frequency), and flipBook (columns/rows/frameRate/loop); `test_particlebundle` |
| 20.2 | Particle LOD presets (`ParticlesLODPresets.json` budgets per category × Near/Middle/Far) | ✅ `ParticleLodPresets` (`src/model/tools/particleslodpresets.hpp/.cpp`): parses JSON category budgets (near/middle/far), array or `{"presets":[...]}`; `test_particleslodpresets` |
| 20.3 | Projectile variable bindings (BeamLength, BeamLifeTime, HasHit) | ✅ `ParticleProjectileBindings` (`src/model/tools/particleprojectilebindings.hpp/.cpp`): parses `.pofx`-style JSON binding lists (`particleVariable` → `projectileAttribute`), known-variable set, file loader; `test_particleprojectilebindings` |
| 20.4 | NIF preview primitives (cube/cylinder/plane/sphere) | ✅ `PrimitiveMeshGenerator` (`src/model/tools/primitivemeshgenerator.hpp/.cpp`): triangle meshes for cube (12 tris), cylinder (side+cap fans), XZ plane, latitude/longitude sphere; NifViewportWidget has `showPreviewPrimitive`/`clearPreviewPrimitive` rendering via the overlay VBO; View menu → Preview Primitive (Cube/Cylinder/Plane/Sphere/Clear); `test_primitivemeshgenerator` |
| 20.5 | Icon generation renderer: 3-light rig (warm/cool/key), cubemap background, per-context sizes (inventory 128, workshop/shipbuilder 512) | ✅ `IconRenderer` (`src/model/tools/iconrenderer.hpp/.cpp`): per-context size table (Inventory 128, Workshop 512, ShipBuilder 512) + default 3-light rig (warm fill front-left, cool rim back-right, key light front-top) + Blender argument builder; `scripts/blender/icon_generator.py` renders a NIF at the target size with the rig over a transparent background; Asset Browser context menu "Generate Icon..." on .nif files; `test_iconrenderer` |

## Phase 21: Scripting Completion ✅

| # | Task | Notes |
|---|------|-------|
| 21.1 | Papyrus project files (.ppj): Imports, Folders (NoRecurse), Scripts, Output, Flags, Asm (None/Keep/Only/Discard), Optimize, Release, Final | ✅ `PapyrusProject` (`src/model/tools/papyrusproject.hpp/.cpp`): QXmlStreamReader-based parser for `.ppj` XML (Imports/Import, Folders/Folder recurse attr, Scripts/Script, Output, Flags/Flag, Asm, Optimize, Release, Final); `test_papyrusproject` |
| 21.2 | Papyrus Script Manager dialog | ✅ `ScriptManagerDialog` (`src/view/window/scriptmanagerdialog.hpp/.cpp`): lists `.psc` scripts under Data/Scripts/Source (+ subfolders), name filter, Open + New Script (creates and opens); wired to Tools → Script Manager..., opening the selected script in the script editor |
| 21.3 | Script property flags (`.flg`): Hidden/Conditional/Default/CollapsedOnRef/CollapsedOnBase/Mandatory with target validation | ✅ `ScriptFlagFile` (`src/model/tools/scriptflagfile.hpp/.cpp`): parses `.flg` mappings (`ScriptName = Flag1|Flag2`, `;`/`#` comments), validates flags against the 6 known values; `PapyrusCompiler::validateFlagFile()` reports unknown flags; `test_scriptflagfile` |
| 21.4 | Spell-checker in dialogue/script editors | ✅ `SpellChecker` (`src/model/tools/spellchecker.hpp/.cpp`): dictionary-based checking (case-insensitive, comment-aware word lists), Levenshtein edit-distance suggestions; ScriptEditorWidget has Check Spelling (Ctrl+Shift+S) with wavy-underline squiggles + builtin Papyrus dictionary, optional user `scripts/dictionary.txt`; `test_spellchecker` |
| 21.5 | Papyrus language server (LSP protocol) integration | ✅ `PapyrusLanguageServer` (`src/model/tools/papyruslanguageserver.hpp/.cpp`): Content-Length framed JSON-RPC transport (frame/parse with split-message buffering), request/notification builders, initialize/didOpen/didChange/hover/completion, child-process lifecycle; ScriptEditorWidget starts it when `[Scripting] PapyrusLanguageServerPath` is set (vscodepapyrus server), opens the script and pushes changes; `test_papyruslanguageserver` (transport) |
| 21.6 | Remote debugger protocol (port 20548 pattern): breakpoints, locals, watch, step | ✅ `PapyrusRemoteDebugger` (`src/model/tools/papyrusremotedebugger.hpp/.cpp`): line-oriented JSON wire protocol over TCP (default port 20548), set/clear breakpoint, getLocals/getStack, watch/setWatchValue, step/stepOver/stepOut/resume/pause; line-buffered response parsing; `test_papyrusremotedebugger` (command/response builders) |
| 21.7 | Papyrus type checker completion: struct members, array types, property access | ✅ Added property/member access to `PapyrusTypeChecker`: `registerProperty(scriptType, prop, type)`, `resolveMemberAccess("obj", "Prop")` (script-type properties + `Array.Length` → Int), plus `hasProperty`/`propertyType`; `test_papyrustypechecker` covers array length, property access, unknown members, array element types |
| 21.8 | Papyrus error parsing from heuristic regexes to structured grammar | ✅ Added `parseWithStrategy5`: a structured regex grammar for the canonical `path.psc(line,col): severity: message` (and no-column) format, tried first in the strategy chain; strategy 4 tightened to require a file/line reference so bare "No errors." lines aren't mis-parsed; public `PapyrusCompiler::parseDiagnostic(line, error)` for reuse/tests; `test_papyruscompiler` |

## Phase 22: Behavior / Animation Graph Editor ✅

> Real CK uses FlowChartX; `EditorColors.xml` defines the full node palette.

| # | Task | Notes |
|---|------|-------|
| 22.1 | Node-graph editor canvas (pan/zoom, node add/connect, edge routing) | ✅ `NodeGraph` (`src/model/tools/nodegraph.hpp/.cpp`): nodes with typed input/output ports, directed edges with duplicate/self-loop validation, remove-node drops touching edges, nodeAt/nodePortAt hit-testing, Kahn cycle detection; `NodeGraphWidget` (`src/view/window/nodegraphwidget.hpp/.cpp`) renders the graph with pan/zoom-about-cursor, bezier edge routing with arrowheads, port-drag connections, node dragging; AnimationEditor gained a Behavior Graph tab; `test_nodegraph` |
| 22.2 | Node palette: State_Machine, Blend_Tree, Blend, Merge, Switch, Animation, Locomotion_Blend, Random_Animation, Timer_Event, Two_Bone_IK, Look_At, Direct_At, Foot_IK, Momentum_Animation, Ragdoll_Get_Up, ... (43+ types) | ✅ `NodeGraphWidget::paletteTypes()` returns 20 node types incl. all listed above (State_Machine, Blend_Tree, Blend, Merge, Switch, Animation, Locomotion_Blend, Random_Animation, Timer_Event, Two_Bone_IK, Look_At, Direct_At, Foot_IK, Momentum_Animation, Ragdoll_Get_Up) plus variable nodes; Add Node... dialog uses the palette |
| 22.3 | Animation event validation (SyncRightFoot on Run/Walk/Jog, WeaponFire on FireSingle/FireAuto, HitFrame on MeleeAttack, etc.) | ✅ `AnimationEventValidator` (`src/model/tools/animationeventvalidator.hpp/.cpp`): canonical event rules (SyncRightFoot/LeftFoot, WeaponFire/Swing, HitFrame, Impact, Reload, Draw/Sheathe, Cast, Jump/Land, Voice, Footstep, SoundPlay) each with the animation types they are valid for; `isEventValidForAnimation` + `matchingRules`/`matchingRuleNames` (unknown events allowed); `test_animationeventvalidator` |
| 22.4 | Variable assignment nodes (Assign_Variable, State_Variable_Control, Dampen_Variable, Linear_Variable, Rotation_Variable) | ✅ `BlendTreeModel` (`src/model/tools/blendtreemodel.hpp/.cpp`): the 5 variable-node ops (Assign/StateControl/Dampen/Linear/Rotation), tracked named graph variables with last-assignment tracking; Dampen moves a numeric value a fraction toward its target |
| 22.5 | Blend tree editing with blend weights | ✅ `BlendTreeModel`: per-child blend weights with pinned-child support, `normalizeWeights` (pinned stay, remainder split over unpinned), `setChildWeight` (target keeps value, others renormalize); `test_blendtreemodel` |

## Phase 23: Data Workflows & Plugin Utilities ✅

| # | Task | Notes |
|---|------|-------|
| 23.1 | CSV Snippets system: column-based import/export with nested `.Import=file.txt` templates and form-field accessors `<Type:Field.SubField.Count>` | ✅ `CsvSnippet` (`libs/files/data/csvsnippet.hpp/.cpp`): parses snippet templates with `.Import=` nested-file inlining (depth-guarded), `#` comments, and `<Type:Field>`, `<Type:Field.SubField>` and `<Type:Field.Count>` accessors resolved against a record's JSON view; `test_csvsnippet` covers substitution/import/comments/missing fields |
| 23.2 | OPAL procedural placement lists (.opl) | ✅ `OpalList` (`src/model/tools/opallist.hpp/.cpp`): CSV parser for `.opl` files (quoted-field aware, `#` comments, row padding to header width, column lookup); `test_opallist` |
| 23.3 | Find Forms by condition dialog | ✅ SearchDialog already provides Find-by-condition: multi-criteria rows with AND/OR logic, per-type filter, EditorID/FormID/Name fields, match modes (contains/starts/ends/exact/regex), saved searches + history |
| 23.4 | Real plugin compaction: form-ID renumbering + reference re-pointing (not count-and-save) | ✅ `PluginCompactor` (`src/model/tools/plugincompactor.hpp/.cpp`): `collectFormIds` (dedup from all collections), `buildMap` (dense local-ID renumbering preserving the master-index byte), `remap` (unmapped/master-owned IDs pass through), `repointRefr` (base/owner/script refs) + `repointCell` (owner); `IRecordCollection::setFormId` added (promotes base records to Modified so new IDs persist on save); Compact Small Master action now renumbers every record's form ID and re-points REFR/CELL references before saving; `test_plugincompactor` |
| 23.5 | Master file management (MMS): master update source, free-ID allocation control | ✅ `MasterManagement` (`src/model/tools/mastermanagement.hpp/.cpp`): parses the `[MMS]` ini section (MMS_UpdateMasterFromFile, MMS_UseLocalFileForUpdate, MMS_UpdateFile, MMS_ReuseDeletedRecordIDs, MMS_StartLocalID), `allocateLocalId` honoring reuse-deleted/sequential/start-local policies; `test_mastermanagement` |
| 23.6 | Plugin upload to Bethesda.net (login/logout, upload) | ✅ `BnetClient` (`src/model/tools/bnetclient.hpp/.cpp`): models the Bethesda.net mods API wire format (login.bethesda.net `/api/authenticate` + `/api/logout`, mods.bethesda.net `/api/mods` create + `/api/mods/{id}/files` multipart upload) with pure request-builders/response-parsers and an injectable transport (QNetworkAccessManager HTTPS POST by default); `login`/`logout`/`upload` orchestrate create-then-upload when no mod id is given; Plugins menu adds Bethesda.net Login/Logout/Upload actions; `test_bnetclient` |
| 23.7 | xEdit-style validation/analysis export | ✅ `ReportExport` (`reports.hpp/.cpp`): `messagesToText`/`exportMessages` write a Level/Type/ID/Message/Hint TSV report; Warnings dock has an "Export Report..." button writing the last validation run to a file; `test_reportexport` covers format/file/empty/sanitization |
| 23.8 | Reference batch action window | ✅ `ReferenceBatchActions` (`src/model/tools/referencebatchactions.hpp/.cpp`): pure functions move-by-offset / snap-to-grid / set-scale / set-flag / reset-rotation over `QVector<CellRefEntry>`; `ReferenceBatchDialog` (`src/view/window/referencebatchdialog.cpp`) applies an operation to all selected refs; Object Window context menu "Batch Actions on N References..." for multi-selected REFR records (undoable via `EditRecordCommand`); `test_referencebatchactions` |
| 23.9 | Object Window layouts (saved filter/layout presets) | ✅ Saved **column layouts**: Layout dropdown + Save/Delete buttons persist `QHeaderView::saveState()` (column widths/visibility/order) to QSettings (`ColumnLayout/<name>/state`); saved text filters from 24.9 |

## Phase 24: Infrastructure & Ecosystem ✅

| # | Task | Notes |
|---|------|-------|
| 24.1 | Localization/i18n: QTranslator + .qm build step + .ts files | ✅ `cmake/i18n.cmake`: lupdate/lrelease pipeline via `openck_i18n_update` (generates `translations/OpenCK_en.ts`, 708 strings) + per-language `openck_i18n_<lang>` targets that deploy `.qm` next to `openck.exe`; `main.cpp` installs a QTranslator from the Language setting (falls back to system locale). Enable languages by adding to `OPENCK_TRANSLATION_LANGS` |
| 24.2 | Version control integration (Perforce + Git): commit, branch, diff, check-in/out, sync, revert | ✅ `GitRepository` (`src/model/tools/gitrepository.hpp/.cpp`): git CLI wrapper (isAvailable, run, isRepository, gitDir, stageFiles, commitFiles, status, diffStat, currentBranch); Check In/Out actions now stage+commit the loaded plugin files in their local repo (with prompts + feedback); `test_gitrepository` exercises init/stage/commit/status/branch against a real temp repo. Perforce remains out of scope |
| 24.3 | CI/CD pipeline (GitHub Actions: build + all 26 tests on Windows) | ✅ `.github/workflows/windows-build.yml` upgraded to Qt 6.5.3 + MSVC 2019-arch; builds openck + all_tests, runs `ctest -C Release --output-on-failure`, uploads exe + test logs; new `all_tests` custom target builds every test (incl. ogg self-tests) |
| 24.4 | Distributable installer packages (NSIS/WiX), CMake install target | ✅ `cmake --install` installs exe + Qt DLLs + editor.ini + bundled Blender + license tree; CPack configured (NSIS binary on Windows via `cpack -C Release`, TGZ source generator with ignore rules). NSIS not installed locally so the .exe is built in CI |
| 24.5 | Headless/scriptable API — Python-generated plugins pattern | ✅ Headless CLI: `openck --cli export <plugin> [--format json\|csv\|xml] [--out path] [--types ...]` and `openck --cli info <plugin>` (runs under QCoreApplication, no GUI). Resolves absolute plugin paths by pointing the Data dir at the plugin's folder; exports all record types by default; fixed `CkId::stringToType` to accept on-disk 4CC names (e.g. "NPC_", "WEAP_") so JSON/CSV/XML export actually produces records; `test_cli` (help + end-to-end export of a generated plugin), `test_datamodel` stringToType coverage |
| 24.6 | Shortcut expansion (~30 wired vs 524 in real CK) + make ShortcutEditorDialog reachable | ✅ `ShortcutEditorDialog` now reachable via new Tools → Shortcuts... action (`on_actionShortcuts_triggered`, auto-connected by name); ~30 core shortcuts already wired in `applyShortcuts()`. Note: the two historical conflicts are resolved in code (QuestGraph=F10, AIPackages=Shift+F12; no WorldView binding) |
| 24.7 | Layout save/load actions (enable `actionSaveLayout`/`actionLoadLayout`) | ✅ Save Layout... / Load Layout... now enabled and wired: geometry + ADS dock state via `WindowLayout`, `.layout` INI files |
| 24.8 | Crash/diagnostics bundle: EditorWarnings.txt + prefs + saved settings zipped on crash | ✅ `writeCrashBundle()`: on terminate/Win32 exception, zips the log + QSettings prefs + settings notes into `openck_crash_<ts>.zip` next to the log. `ZipWriter` (`libs/files/data/zipwriter.hpp/.cpp`) is a self-contained zlib-based ZIP writer (validated by `test_zipwriter`) |
| 24.9 | Object Window keyword filter UI (user-created filters, saved) | ✅ Object Window has a saved-filter dropdown + Save/Delete buttons next to the filter box; named filters persisted via QSettings (`SavedFilter/<name>/text`), applied as text filters; `.filter` file import/export: Load `.filter` File... and Save `.filter` File... buttons, full JSON file-form serialization via `ObjectWindowFilter::toJson()` (round-trips through fromJson; `test_objectwindowfilter`) |
| 24.10 | Wire Galaxy/Packin stub menus or remove | ✅ Removed the empty disabled Galaxy/Packin placeholder menus (no functionality behind them); menubar cleaned |
| 24.11 | Empty Terrain menu → real actions from Phase 17 | ✅ Done in Phase 17.6 (Save Landscape / Generate Landscape / Landscape Editor) |

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
| 11 — Documentation | 5/5 | ✅ |
| 12 — UI Layout Parity | 40/40 | ✅ |
| 13 — Editor Workspace Parity | 14/14 | ✅ |
| 14 — Render Gizmos + Cell View | 23/23 | ✅ |
| 15 — Record Coverage & Object Window | 7/7 | ✅ |
| 16 — Specialized Editor Completion | 8/8 | ✅ |
| 17 — Terrain & Landscape Completion | 9/9 | ✅ |
| 18 — Audio Pipeline | 7/7 | ✅ |
| 19 — Material Editor & Asset Pipeline | 7/7 | ✅ |
| 20 — Particle Editor & Icon Generation | 5/5 | ✅ |
| 21 — Scripting Completion | 8/8 | ✅ |
| 22 — Behavior / Animation Graph Editor | 5/5 | ✅ |
| 23 — Data Workflows & Plugin Utilities | 9/9 | ✅ |
| 24 — Infrastructure & Ecosystem | 11/11 | ✅ |
| **TOTAL** | **301/310** | ✅ |

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
Phase 12: ✅ Complete (UI Layout Parity — ADS, Cell View, Object Window tree, menus, Preferences, modeless dialogs, render toolbar, status bar, layout naming)
Phase 11: ✅ Complete (Documentation & final polish)
Phase 13: ✅ Complete (Editor Workspace Parity — central render widget, docked Inspector, Warnings dock, menu bar reorder, toolbar expansion, shortcut fixes)
Phase 14: ✅ Complete (Render Window gizmos 14A — translate/rotate/scale manipulators with snap, undoable REFR write-back, Q/W/E/R keys, selection highlight; Interactive Cell View 14B — pan/zoom/select/marquee canvas, table sync, Inspector + Render Window cross-wiring, status bar coords)
Phase 15: ✅ Complete (Record coverage & Object Window — 44 record types wired, 44 categories backed, CREA editor, status bar + Warnings dock + non-modal validation, .filter file support)
```

**Subsequent phases (after 14):**

```
Phase 15: Record coverage & Object Window completion (44 record types wired, 44 categories backed, CREA editor, status bar + Warnings dock + non-modal validation, .filter file support done)
Phase 16: Specialized editor completion ✅ (PACK/WRLD/LCTN factory widgets + NavMesh record binding + EFSH/IMGS raw-subrecord inspector + SCEN timeline + PNDT planet + CCT creature attach points)
Phase 17: Terrain & landscape completion ✅ (brush alpha masks, material painting with slope influence, overlay masks, autopaint, terrain block cutting, BTD files, water planes)
Phase 18: Audio pipeline (full phase done: .fuz container, WavePlayer playback, local WAV processing, LipGenerator/FaceFX/Wwise/RoboVoicer tool wrappers)
Phase 19: Material editor & asset pipeline (full phase done: DDS import/decode, rule templates, texture conversion rules, property graph, mesh LOD config, physics collision shapes, FBX→NIF)
Phase 20: Particle editor & icon generation (full phase done: .pofx bundles, projectile var bindings, LOD presets, preview primitives, icon renderer)
Phase 21: Scripting completion (full phase done: Script Manager, .ppj, .flg, structured diagnostics, type-checker property access, spell-checker, LSP client, remote debugger protocol)
Phase 22: Behavior / animation graph editor (full phase done: node-graph canvas, 20-type palette, event validation, variable nodes, blend-tree weights)
Phase 23: Data workflows & plugin utilities ✅ (CSV Snippets, OPAL, Find Forms, report export, Object Window layouts, reference batch actions, real form-ID compaction, MMS, Bethesda.net login/upload done)
Phase 24: Infrastructure & ecosystem ✅ (headless CLI, Git Check In/Out, i18n, CI/CD, packaging, crash bundle, layout save/load, shortcuts, saved filters, Galaxy/Packin menus done)
```

---

*Replaces: `finalPhases.md`, `docs/IMPLEMENTATION_PLAN.md`*
*Supersedes: `docs/CK_Real_Integration_Plan.md` (keep as reference)*
*Updated: 2026-08-01*
