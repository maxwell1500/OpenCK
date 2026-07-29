# OpenCK UI Layout Audit v2 — vs. Real Starfield Creation Kit

A deep-dive, line-referenced comparison of OpenCK's current UI against
the real Bethesda Starfield Creation Kit binary (Qt5 +
QtAdvancedDocking). Reference data for the real CK comes from binary
string extraction of `CreationKit.exe` (top-level menus, 524 action
names, dock window titles, Object Window tree categories, theming
assets, Render Window toolbar actions). OpenCK data comes from the
current tree under `src/` and `ui/`.

This document supersedes `docs/LAYOUT_AUDIT.md`. It is more specific
(exact action names from the binary, exact `file:line` references on
the OpenCK side) and is the work list for the next round of UI work.

Priority legend:

- **Critical** — blocks core usability; OpenCK cannot be used the way
  the CK is used.
- **High** — visually jarring or breaks a common workflow.
- **Medium** — parity gap that does not block work.
- **Low** — cosmetic.

---

## Summary table

| # | Area | Real CK has | OpenCK has | Gap severity |
|---|------|-------------|------------|-------------|
| 1 | Menu bar | 16 menus, 524 actions | 18 menus (incl. `Plugins`, `Tools` not in real CK), ~60 actions, mostly wrong-menu | **Critical** |
| 2 | Object Window tree | 9 groups, ~80+ record categories | 7 groups, 27 categories, wrong group names, missing ~60 categories | **Critical** |
| 3 | Dock layout | ADS, Object Window left, Cell View right, Render Window center (central widget), Inspector, Warnings | ADS, Object Window left, Cell View right, 3D Viewport right (not center), no Inspector, no Warnings | **Critical** |
| 4 | Theming | Theme menu (Default/Plastique/Plastique Dark), `DefaultDark.qss`, `DefaultQtAdvancedDocking.css` | Theme menu exists but stub (`actionNotImplementedTheme`); ThemeManager exists but Theme menu not wired | **High** |
| 5 | Render Window toolbar | Snap-to-Grid/Angle/Connect-Points, Lock-to-X/Y, Ground Plane, Wireframe, Collision, Lighting, Grid Lines, Space Grid, 3D Grid, Top-Down, Isometric, Preview, Preview Orbit, Reset Camera, Pan-to-Selection, Copy/Cut/Paste Render, Paste-in-Place | Select/Move/Rotate/Scale, Snap-to-Grid, Snap-to-Angle, Wireframe, Grid, Axis, Bounds, Collision, Cell Grid, Export NIF, Edit Mesh, Filter, Hierarchy, Anim toolbar, Particle toolbar | **Critical** |
| 6 | Form dialog / Inspector | "Inspector" dock with sections for components, keywords, scripts, etc. | `QtFormDialog` modal QDialog with "Properties" + "Data" tabs; not docked; not called "Inspector" | **High** |
| 7 | Status bar | Cell coordinates, object counts, selected object info (real CK), plus warnings dock | Records count, Cell coords, Selected object, Plugin info, Progress bar | **Medium** |
| 8 | Cell View panel | Docked "Cell View" (right side), worldspace combo + cell list + 2D map + reference table | `CellViewPanel` docked right with worldspace combo + cell list + 2D map canvas + refr table — structurally close | **Medium** |
| 9 | Main toolbar | New, Open, Save, Save All, Check Out/In, Undo, Redo | Open, Save, Undo, Redo (no New, no Save All, no Check Out/In) | **High** |
| 10 | Keyboard shortcuts | 524 actions, many with shortcuts | ~30 actions with shortcuts; missing most | **High** |

---

## 1. Menu bar — **Critical**

### Reference (real CK)

Top-level menus confirmed from the binary (16 menus):

```
menuFile, menuEdit, menuView, menuCharacter, menuGameplay, menuWorld,
menuObjectWindows, menuRenderWindows, menuNavmesh, menuTerrain,
menuAudio, menuGalaxy, menuPackin, menuDocks, menuTheme, menuTests,
menuHelp
```

Key actions per menu (from the 524 total confirmed in the binary):

- **File**: `actionNew`, `actionOpen`, `actionSave`, `actionSaveAs`,
  `actionSaveAll`, `actionSave_Plugin`, `actionData`, `actionExit`,
  `actionRevert_file`, `actionPreferences`, `actionCheckOut`,
  `actionCheckIn`, `actionCheckout_active_file`,
  `actionRevertCheckedOutFiles`, `actionSubmit_file_in_Perforce`,
  `actionSync_Files`, `actionCompactSmallMaster`,
  `actionCompactMediumMaster`, `actionConvertLargeMaster`,
  `actionConvertMediumMaster`, `actionConvertSmallMaster`,
  `actionCreate_Archive`, `actionCompile_Papyrus_Scripts`,
  `actionPapyrus_Script_Manager`, `actionUpload_Plugin_PC`,
  `actionUpload_Plugin_PS4`, `actionUpload_Plugin_XB1`,
  `actionLogin_to_Bethesda_net`, `actionLogout`
- **Edit**: `actionUndo`, `actionRedo`, `actionSelectAll`,
  `actionDuplicate`, `actionDeleteSelection`, `actionSearch_Replace`,
  `actionFind_Text`, `actionPaste_in_Place`, `actionCopy_Render`,
  `actionCut_Render`, `actionPaste_Render`
- **View**: `actionInspector`, `actionPreview_Window`, `actionWarnings`,
  `actionToggle_Wireframe`, `actionToggle_Collision`,
  `actionToggle_Lighting`, `actionToggle_Sky`, `actionToggle_Water`,
  `actionToggle_Grass`, `actionToggle_Wind`, `actionToggle_LOD`,
  `actionToggle_VertexColors`, `actionToggle_ViewMode`,
  `actionShow_Grid_Lines`, `actionSpace_Grid`, `actionToggle_3DGrid`,
  `actionStatusbar`, `actionToolbar`
- **Character**: `actionActor_Values`, `actionActors_Voice_Types`,
  `actionBody_Part_Data`, `actionCreature_Creator`,
  `actionFaction_Rank_Names`, `actionBooks`, `actionTrees`,
  `actionAmmo`
- **World**: `actionCells`, `actionWorld_Spaces`,
  `actionWorld_Heightmap_Editing`, `actionRegions`, `actionLayers`,
  `actionDefault_Layer_Manager`, `actionLight_Markers`,
  `actionSound_Markers`
- **ObjectWindows**: `actionObject_Window_Layouts`,
  `actionOpenNewObjectWindow`, `actionCell_View_Window`,
  `actionObject_Palette_Editing_Qt`, `actionFindFormsByCondition`,
  `actionReference_Batch_Action_Window`, `actionOpen_Windows`,
  `actionResetAllObjectWindowPositions`
- **RenderWindows**: `actionOpenNewRenderWindow`,
  `actionPreview_Window`, `actionRefresh_Render_Window`,
  `actionResetAllRenderWindowPositions`,
  `actionRender_Window_Hotkeys`,
  `actionRender_Window_Picking_Preferences`
- **Navmesh**: `actionCheck_NavMeshes`, `actionClean_NavMesh_Splines`,
  `actionConnectNavmeshes_All_Interiors`,
  `actionConnectNavmeshes_WorldSpace`,
  `actionFinalize_Cell_NavMeshes`, `actionToggle_Navmesh_Mode`,
  `actionToggle_NavMesh_Splines_Edit_Mode`
- **Terrain**: `actionLandscape_Editing`, `actionLandscape_Cutting`,
  `actionHeightmap_Import`, `actionHeightmap_Autopaint`,
  `actionSave_Landscape`, `actionTerrain_Blocks`,
  `actionWorld_Heightmap_Editing`
- **Audio**: `actionBuild_Soundbank_for_Active_File`,
  `actionProcess_Local_Voice_WAVs`, `actionRun_FaceFX_Compiler`,
  `actionReload_Wwise_Data`,
  `actionMaterial_Type_Override_Report`
- **Docks**: `actionManageLayouts`, `actionSaveCurrentLayout`,
  `actionSaveLayoutAs`, `actionApplyLayout`, `actionSnapWindows`,
  `actionResetAllObjectWindowPositions`,
  `actionResetAllRenderWindowPositions`, `actionShow_Hide_Window`
- **Theme**: `actionDefaultTheme`, `actionPlastique`,
  `actionPlastique_Dark`
- **Tests**: `actionTest_All_Cells`, `actionTest_Interior_Cells`,
  `actionTest_Models`, `actionTest_Icons_Textures`,
  `actionValidate_Forms`, `actionValidate_Loaded_Data`,
  `actionValidate_Room_Portal_Alignment`,
  `actionValidate_Water_Geometry`

### OpenCK current

`ui/mainwindow.ui:204-223` wires 18 menus into the menu bar (in
order):

```
File, Edit, View, Character, Gameplay, World, ObjectWindows,
RenderWindows, Navmesh, Terrain, Audio, Galaxy, Packin, Docks,
Theme, Tests, Help, Plugins, Tools
```

The last two (`Plugins`, `Tools`) do not exist in the real CK. Menu
title text in the UI file:

- `menuObjectWindows` (line 112): "ObjectWindows" (one word, no
  space) — real CK uses "Object Windows" (two words).
- `menuRenderWindows` (line 120): "RenderWindows" — real CK uses
  "Render Windows".
- `menuDocks` (line 160): "Docks" — real CK menu is named "Docks"
  but its actions are about window layouts, not about per-dock
  visibility (which OpenCK's `actionResetWindowLayout` approximates).
- `menuTheme` (line 167): exists but contains only a stub action
  `actionNotImplementedTheme` (`ui/mainwindow.ui:373-380`,
  `enabled=false`).
- `menuTests` (line 173): exists but contains only a stub action
  `actionNotImplementedTests` (`ui/mainwindow.ui:381-388`,
  `enabled=false`).
- `menuGalaxy` (line 146) and `menuPackin` (line 152): exist but each
  contain only a stub `actionNotImplementedGalaxy` /
  `actionNotImplementedPackin`.

Actions actually defined in `ui/mainwindow.ui`:

- File (lines 33-48): `actionNew_Plugin`, `actionData`, `actionSave`,
  `actionSaveAs`, `actionClose_Plugin`, `menuExport` (with
  `actionExportDialogue`, `actionExportScripts`,
  `actionExportTextures`), `actionCreateArchive`,
  `actionCompilePapyrusScripts`, `actionCompactSmallMaster`,
  `actionPreferences`, `actionValidate`, `actionExit`.
- Edit (lines 54-75): `actionUndo`, `actionRedo`, `actionCut`,
  `actionCopy`, `actionPaste`, `actionDelete`, `actionDuplicate`,
  `actionSelectAll`, `actionSearchAndReplace`, `actionFindNext`,
  `actionFindPrevious`, `actionNextRecord`, `actionPreviousRecord`,
  `actionFirstRecord`, `actionLastRecord`, `actionExpandAll`,
  `actionCollapseAll`.
- View (lines 81-90): `actionScriptEditor`, `actionDialogueEditor`,
  `actionDialogueTree`, `actionQuestGraph`, `actionAIPackages`,
  `actionWeatherLight`, `actionWater`, `actionCellTransitions`,
  `actionPapyrusDebugger`, `actionFormIdEditor`.
- Character (lines 103-108): `actionAnimationEditor`,
  `actionNpcEditor`, `actionRaceEditor`, `actionClassEditor`,
  `actionFactionEditor`.
- Gameplay (line 183): `actionSettings` only.
- World (lines 96-97): `actionWorldspaces`, `actionWorldView`.
- ObjectWindows (lines 114-116): `actionObjectWindow`,
  `actionCells`, `actionObjectPalette`.
- RenderWindows (lines 122-126): `action3DViewport`,
  `actionMaterialEditor`, `actionPreviewWindow`, `actionLighting`.
- Navmesh (line 132): `actionNavmesh` only.
- Terrain (line 138): `actionLandscapeEditing` only.
- Audio (line 144): `actionSoundEditor` only.
- Docks (lines 162-165): `actionResetWindowLayout`,
  `actionSaveLayout`, `actionLoadLayout`.
- Theme (line 171): `actionNotImplementedTheme` (stub).
- Tests (line 177): `actionNotImplementedTests` (stub).
- Help (line 202): `actionAbout` only.
- Plugins (lines 189-196): `actionLoadOrder`, `actionMasterFiles`,
  `actionConflictDetection`, `actionConflictResolution`,
  `actionPluginMerge`, `actionLoadOrderOptimizer`,
  `actionBashedPatch`, `actionExternalTools` — entire menu does not
  exist in real CK.
- Tools (lines 301-308): `actionParticleEffectsEditor`,
  `actionModManager` — entire menu does not exist in real CK.

### Gaps

1. **Menu title text wrong**: "ObjectWindows" and "RenderWindows"
   should be "Object Windows" and "Render Windows" to match real CK
   (`ui/mainwindow.ui:112,120`).
2. **File menu missing**: `actionNew` (we have `actionNew_Plugin`,
   not named the same), `actionOpen`, `actionSaveAll`,
   `actionSave_Plugin`, `actionRevert_file`, `actionCheckOut`,
   `actionCheckIn`, `actionCheckout_active_file`,
   `actionRevertCheckedOutFiles`, `actionSubmit_file_in_Perforce`,
   `actionSync_Files`, `actionCompactMediumMaster`,
   `actionConvertLargeMaster`, `actionConvertMediumMaster`,
   `actionConvertSmallMaster`, `actionPapyrus_Script_Manager`,
   `actionUpload_Plugin_PC/PS4/XB1`, `actionLogin_to_Bethesda_net`,
   `actionLogout`. We also have `actionClose_Plugin` and the entire
   `menuExport` submenu which do not exist in the real CK.
3. **Edit menu wrong**: real CK has `actionCopy_Render`,
   `actionCut_Render`, `actionPaste_Render`, `actionPaste_in_Place`
   — OpenCK has none. OpenCK has `actionCut`/`actionCopy`/
   `actionPaste` (generic, not "Render") plus navigation actions
   (`actionNextRecord`, `actionPreviousRecord`, `actionFirstRecord`,
   `actionLastRecord`, `actionExpandAll`, `actionCollapseAll`) that
   are not in the real CK's Edit menu.
4. **View menu wrong**: real CK is all about viewport toggles
   (`actionToggle_Wireframe`, `actionToggle_Collision`,
   `actionToggle_Lighting`, `actionToggle_Sky`,
   `actionToggle_Water`, `actionToggle_Grass`, `actionToggle_Wind`,
   `actionToggle_LOD`, `actionToggle_VertexColors`,
   `actionToggle_ViewMode`, `actionShow_Grid_Lines`,
   `actionSpace_Grid`, `actionToggle_3DGrid`, `actionStatusbar`,
   `actionToolbar`) plus `actionInspector`, `actionPreview_Window`,
   `actionWarnings`. OpenCK's View menu is full of editor launchers
   (`actionScriptEditor`, `actionDialogueEditor`,
   `actionDialogueTree`, `actionQuestGraph`, `actionAIPackages`,
   `actionWeatherLight`, `actionWater`, `actionCellTransitions`,
   `actionPapyrusDebugger`, `actionFormIdEditor`) — none of which
   are in the real CK's View menu. Those editor launchers belong in
   Character / World / Audio / etc.
5. **Character menu missing**: real CK Character menu has
   `actionActor_Values`, `actionActors_Voice_Types`,
   `actionBody_Part_Data`, `actionCreature_Creator`,
   `actionFaction_Rank_Names`, `actionBooks`, `actionTrees`,
   `actionAmmo`. OpenCK has `actionAnimationEditor`,
   `actionNpcEditor`, `actionRaceEditor`, `actionClassEditor`,
   `actionFactionEditor` — none of which match the real CK's
   Character menu actions.
6. **Gameplay menu empty**: OpenCK Gameplay menu has only
   `actionSettings`. The real CK Gameplay menu contents were not
   among the listed key actions, but the menu itself exists.
7. **World menu missing**: real CK World menu has `actionCells`,
   `actionWorld_Spaces`, `actionWorld_Heightmap_Editing`,
   `actionRegions`, `actionLayers`, `actionDefault_Layer_Manager`,
   `actionLight_Markers`, `actionSound_Markers`. OpenCK has only
   `actionWorldspaces` and `actionWorldView` (the latter is not a
   real CK action name).
8. **ObjectWindows menu missing**: real CK has
   `actionObject_Window_Layouts`, `actionOpenNewObjectWindow`,
   `actionCell_View_Window`, `actionObject_Palette_Editing_Qt`,
   `actionFindFormsByCondition`,
   `actionReference_Batch_Action_Window`, `actionOpen_Windows`,
   `actionResetAllObjectWindowPositions`. OpenCK has
   `actionObjectWindow`, `actionCells`, `actionObjectPalette` — none
   match.
9. **RenderWindows menu missing**: real CK has
   `actionOpenNewRenderWindow`, `actionPreview_Window`,
   `actionRefresh_Render_Window`,
   `actionResetAllRenderWindowPositions`,
   `actionRender_Window_Hotkeys`,
   `actionRender_Window_Picking_Preferences`. OpenCK has
   `action3DViewport`, `actionMaterialEditor`,
   `actionPreviewWindow`, `actionLighting` — only
   `actionPreviewWindow` is a near-match (real name is
   `actionPreview_Window`).
10. **Navmesh menu missing**: real CK has `actionCheck_NavMeshes`,
    `actionClean_NavMesh_Splines`,
    `actionConnectNavmeshes_All_Interiors`,
    `actionConnectNavmeshes_WorldSpace`,
    `actionFinalize_Cell_NavMeshes`, `actionToggle_Navmesh_Mode`,
    `actionToggle_NavMesh_Splines_Edit_Mode`. OpenCK has only
    `actionNavmesh`.
11. **Terrain menu missing**: real CK has `actionLandscape_Editing`,
    `actionLandscape_Cutting`, `actionHeightmap_Import`,
    `actionHeightmap_Autopaint`, `actionSave_Landscape`,
    `actionTerrain_Blocks`, `actionWorld_Heightmap_Editing`. OpenCK
    has only `actionLandscapeEditing`.
12. **Audio menu missing**: real CK has
    `actionBuild_Soundbank_for_Active_File`,
    `actionProcess_Local_Voice_WAVs`, `actionRun_FaceFX_Compiler`,
    `actionReload_Wwise_Data`,
    `actionMaterial_Type_Override_Report`. OpenCK has only
    `actionSoundEditor`.
13. **Docks menu wrong**: real CK Docks menu has
    `actionManageLayouts`, `actionSaveCurrentLayout`,
    `actionSaveLayoutAs`, `actionApplyLayout`, `actionSnapWindows`,
    `actionResetAllObjectWindowPositions`,
    `actionResetAllRenderWindowPositions`,
    `actionShow_Hide_Window`. OpenCK has `actionResetWindowLayout`,
    `actionSaveLayout`, `actionLoadLayout` — names don't match.
14. **Theme menu stub**: real CK Theme menu has
    `actionDefaultTheme`, `actionPlastique`,
    `actionPlastique_Dark`. OpenCK Theme menu has only
    `actionNotImplementedTheme` (`enabled=false`).
15. **Tests menu stub**: real CK Tests menu has
    `actionTest_All_Cells`, `actionTest_Interior_Cells`,
    `actionTest_Models`, `actionTest_Icons_Textures`,
    `actionValidate_Forms`, `actionValidate_Loaded_Data`,
    `actionValidate_Room_Portal_Alignment`,
    `actionValidate_Water_Geometry`. OpenCK Tests menu has only
    `actionNotImplementedTests` (`enabled=false`). (We do have
    `actionValidate` but it lives in File, not Tests.)
16. **`Plugins` menu does not exist in real CK**: OpenCK's entire
    Plugins menu (`ui/mainwindow.ui:185-197`) —
    `actionLoadOrder`, `actionMasterFiles`,
    `actionConflictDetection`, `actionConflictResolution`,
    `actionPluginMerge`, `actionLoadOrderOptimizer`,
    `actionBashedPatch`, `actionExternalTools` — has no
    counterpart. These are OpenCK-original tooling and should be
    relocated (e.g. into File or a dedicated submenu), not presented
    as a top-level menu.
17. **`Tools` menu does not exist in real CK**: OpenCK's Tools menu
    (`ui/mainwindow.ui:301-308`) plus runtime-added actions
    (`src/view/window/mainwindow.cpp:254-381`: Validate Assets,
    Check Asset Dependencies, Batch Export, Batch Rename Records,
    Reassign FormIds, Generate LOD Meshes) has no counterpart.
18. **Galaxy and Packin menus are stubs**: real CK has these menus
    (confirmed in the binary); OpenCK's are placeholder
    `actionNotImplementedGalaxy` /
    `actionNotImplementedPackin` (`ui/mainwindow.ui:146-156`).
    Starfield-specific — can stay as stubs but should track the
    real action names once extracted.
19. **Menu order differs**: real CK order is File, Edit, View,
    Character, Gameplay, World, ObjectWindows, RenderWindows,
    Navmesh, Terrain, Audio, Galaxy, Packin, Docks, Theme, Tests,
    Help. OpenCK appends `Plugins` and `Tools` after Help
    (`ui/mainwindow.ui:221-222`), and Gameplay appears in the right
    place but the order of the stub menus matches.

**Priority**: Critical.

---

## 2. Object Window tree — **Critical**

### Reference (real CK)

The Object Window tree uses **9 groups** (confirmed from binary
strings), in this order:

```
All Forms, Actors, Items, World Objects, Gameplay, Audio,
Dialogue, World, Miscellaneous
```

Each group expands to record-type categories. The binary confirms
these category names (partial list, ~80+ total):

- Static, Activator, Tree, Container, Door, Furniture, Ingredient,
  Alchemy, Armor, Ammo, Weapon, Book, Key, Misc, SoulGem, NPC,
  Creature, Leveled Actor, Leveled Item, Leveled NPC, Leveled Spell,
  Spell, Enchantment, Potion, Scroll, Power, Water Shader, Weather
  Shader, Effect Shader, Weather, Climate, Sound, Music Type, Voice
  Type, Quest, Package, Combat Style, Class, Faction, Global, Game
  Setting, Race, Body Part, Head Part, Encounter Zone, Location,
  Reference, Cell, Worldspace, Navmesh, Landscape, Static Collection,
  Movable Static, Idle Marker, Acoustic Space, Effect Shader, Image
  Space, Light, Perk, Keyword, Constructible Object, Outfit, Art
  Object, Note, Terminal, Message, Topic, Info, Scene, Speech
  Challenge, Camera Path, Camera Shot, Impact Data, Lens Flare,
  Flora, Grass, Debris, Hazard.

Note the group naming: the real CK uses **"Miscellaneous"** as the
9th group, not "World". "World" is a separate group containing Cell,
Worldspace, Navmesh, Landscape, etc.

### OpenCK current

`src/model/window/objectwindow.cpp:262-296` defines 27 categories
via `addCategory(...)`:

```
NPC, Armor, Weapon, Alchemy, Ingredient, Book, Miscellaneous,
Container, Enchantment, Spell, Magic Effect, Static, Activator,
Tree Node, Quest, Package, Global Variable, Game Settings, Perk,
Class, Faction, Race, Sound, Dialogue, Information, Weather,
Land Texture, Location Reference Type
```

`src/model/window/objectwindow.cpp:314-323` defines **7 groups**
via `addGroup(...)`:

```
Actors, Items, World Objects, Gameplay, Audio, Dialogue, World
```

### Gaps

1. **Missing groups**: real CK has 9 groups; OpenCK has 7. Missing:
   - **"All Forms"** — the first group in the real CK that lists
     every record type flat. OpenCK has no equivalent
     (`src/model/window/objectwindow.cpp:314-323`).
   - **"Miscellaneous"** — the real CK's 9th group. OpenCK has no
     Miscellaneous group. The category "Miscellaneous" exists at
     line 269 but it is a *category* (mapped to `CkId::Type_Misc_`,
     i.e. the MISC record), not a group.
2. **Wrong group name**: OpenCK's last group is named "World"
   (`src/model/window/objectwindow.cpp:323`), containing Weather,
   Land Texture, and Location Reference Type. The real CK has a
   "World" group too, but it contains Cell, Worldspace, Navmesh,
   Landscape — *not* Weather/Land Texture/Location Ref Type. OpenCK
   is conflating two different concepts.
3. **Missing record categories (~60 missing)**. The real CK has
   ~80+ categories; OpenCK has 27. Categories confirmed in the binary
   but absent from OpenCK's `addCategory` calls
   (`src/model/window/objectwindow.cpp:262-296`):
   - Door, Furniture, Key, SoulGem, Creature, Leveled Actor, Leveled
     Item, Leveled NPC, Leveled Spell, Scroll, Power, Water Shader,
     Weather Shader, Effect Shader, Climate, Music Type, Voice Type,
     Combat Style, Body Part, Head Part, Encounter Zone, Location,
     Reference, Cell, Worldspace, Navmesh, Landscape, Static
     Collection, Movable Static, Idle Marker, Acoustic Space, Image
     Space, Light, Keyword, Constructible Object, Outfit, Art
     Object, Note, Terminal, Message, Topic, Info (we have
     "Information" — real name is "Info"), Scene, Speech Challenge,
     Camera Path, Camera Shot, Impact Data, Lens Flare, Flora,
     Grass, Debris, Hazard.
4. **Wrong category names**:
   - "Tree Node" (`objectwindow.cpp:277`) — real CK calls it
     "Tree".
   - "Magic Effect" (`objectwindow.cpp:273`) — real CK uses "Magic
     Effect" but the record sigil in the binary is `MGEF`; verify.
   - "Miscellaneous" (`objectwindow.cpp:269`) — this is the *MISC*
     item record, not the "Miscellaneous" group. Real CK lists
     "Misc" as the category.
   - "Global Variable" (`objectwindow.cpp:281`) — real CK uses
     "Global".
   - "Game Settings" (`objectwindow.cpp:282`) — real CK uses "Game
     Setting" (singular).
   - "Land Texture" (`objectwindow.cpp:294`) — real CK uses "Land
     Texture" (this one matches).
   - "Location Reference Type" (`objectwindow.cpp:295`) — real CK
     uses "Location" and "Reference" as separate categories.
   - "Information" (`objectwindow.cpp:291`) — real CK uses "Info".
5. **No "All Forms" virtual group**: real CK's first group is a
   flat alphabetical list of every record type. OpenCK's tree starts
   directly with "Actors". This is a major navigation difference
   (`objectwindow.cpp:314`).
6. **Category-to-group mapping wrong for "World" group**: OpenCK
   puts Weather, Land Texture, Location Reference Type under "World"
   (`objectwindow.cpp:323`). In the real CK, "World" contains Cell,
   Worldspace, Navmesh, Landscape, Reference, etc. Weather belongs
   elsewhere (Climate/Weather are their own categories under a
   different group).
7. **No keyword/filter file support**: real CK ships
   `Data\DataViews\ObjectWindow\_common\*.filter` files that
   define which record types appear in which group. OpenCK hard-
   codes the groups in C++ (`objectwindow.cpp:297-323`). A data-
   driven approach would let users customize.
8. **Cell/Worldspace/Navmesh/Landscape/Reference categories
   missing from model**: OpenCK's `ObjectWindowModel::initCategories`
   switch (`objectwindow.cpp:38-126`) has no cases for
   `CkId::Type_Cel_`, `CkId::Type_WRLD_`, `CkId::Type_Refr_`, etc.
   The editor dialog (`objectwindowdialog.cpp:539-562`) *can* edit
   Cell/Worldspace/Refr/Location records, but the model never
   creates those categories, so they never appear in the tree.

**Priority**: Critical.

---

## 3. Dock layout — **Critical**

### Reference (real CK)

The real CK uses **QtAdvancedDocking (ADS)** with these confirmed
dock window titles:

- **"Object Window"** — docked left. Contains the record tree.
- **"Cell View"** — docked right. Contains the cell browser + 2D
  map + reference table.
- **"Render Window"** — the **central widget** of the main window
  (not a dock). This is the 3D viewport.
- **"Object palette"** — note lowercase 'p'. A dock, typically
  bottom.
- **"Inspector"** — the properties/record-editor window. Docked.
  **Not** called "Properties".
- **"Warnings"** — a separate dock listing build/validation
  warnings.

ADS layout: Object Window left, Cell View right, Render Window
center (as the QMainWindow central widget, with docks around it),
Inspector and Warnings docked as tabs or split sides.

### OpenCK current

`src/view/window/mainwindow.cpp:117` creates an ADS
`CDockManager`. Docks are created in `setData()`:

- **Object Window** — `ads::CDockWidget("Object Window")` added to
  `ads::LeftDockWidgetArea` (`mainwindow.cpp:181-184`).
- **Landscape Editor** — `ads::CDockWidget("Landscape Editor")`
  added to `ads::BottomDockWidgetArea` (`mainwindow.cpp:192-194`).
- **Object Palette** — `ads::CDockWidget("Object Palette")` added
  to `ads::BottomDockWidgetArea` (`mainwindow.cpp:207-209`). Note:
  capitalized "Palette"; real CK is "Object palette" (lowercase p).
- **Cell View** — `ads::CDockWidget("Cell View")` added to
  `ads::RightDockWidgetArea` (`mainwindow.cpp:1331-1333`). Created
  on-demand from `on_actionCells_triggered`.
- **3D Viewport** — `ads::CDockWidget("3D Viewport")` added to
  `ads::RightDockWidgetArea` (`mainwindow.cpp:964-967`). Created
  on-demand from `on_actionNifViewport_triggered`.

`src/view/window/windowlayout.cpp:7-17` `applyDefaultLayout`
simply toggles all docks visible — it does not arrange them into
the real CK's left/center/right arrangement.

### Gaps

1. **No central Render Window**: the real CK's Render Window is
   the QMainWindow central widget, not a dock. OpenCK makes the 3D
   viewport a right-docked ADS widget
   (`mainwindow.cpp:964-967`), so it cannot be the spacious center
   the CK provides. The `centralWidget` in `ui/mainwindow.ui:19` is
   an empty `QWidget`.
2. **3D Viewport is on-demand, not always present**: real CK opens
   with the Render Window visible. OpenCK requires the user to
   invoke `action3DViewport` (`mainwindow.cpp:957-978`) to create
   it. Should be created in `setData()` like Object Window.
3. **No Inspector dock**: real CK has an "Inspector" dock for
   editing the selected record's properties. OpenCK's record editor
   is `QtFormDialog`, a **modal `QDialog`** (`qtformdialog.cpp:14`)
   opened via `QtFormDialogManager::openOrFocus` — not a dock, and
   not named "Inspector". Each edit pops a separate floating
   window. Critical workflow gap.
4. **No Warnings dock**: real CK has a "Warnings" dock. OpenCK
   has none — validation results are shown in a `QMessageBox`
   (`mainwindow.cpp:1092-1099`).
5. **Cell View is on-demand, not default**: real CK opens with
   Cell View docked right. OpenCK creates it lazily
   (`mainwindow.cpp:1320-1339`). Should be created in `setData()`.
6. **Object Palette capitalization**: OpenCK uses "Object Palette"
   (`mainwindow.cpp:207`). Real CK uses "Object palette"
   (lowercase p). Minor but worth matching.
7. **"Landscape Editor" dock not in real CK**: the real CK
   landscape editing is a mode of the Render Window, not a
   separate bottom dock. OpenCK's `landscapeDock`
   (`mainwindow.cpp:192-194`) is an OpenCK-original.
8. **No default ADS geometry**: `windowlayout.cpp:7-17` only
   toggles visibility. It does not set Object Window to
   `LeftDockWidgetArea` explicitly, Cell View to
   `RightDockWidgetArea`, etc. The arrangement relies on insertion
   order, which ADS does not guarantee across sessions.
9. **Empty central widget**: `ui/mainwindow.ui:19` declares a
   `QWidget` centralWidget that is never used. This is where the
   Render Window belongs.

**Priority**: Critical.

---

## 4. Theming — **High**

### Reference (real CK)

The real CK ships two QSS stylesheets:

- `DefaultDark.qss` — the dark theme applied by default.
- `:/StyleSheets/DefaultQtAdvancedDocking.css` — ADS-specific
  styling.

The **Theme menu** has three actions:

- `actionDefaultTheme`
- `actionPlastique`
- `actionPlastique_Dark`

The CK is dark-themed by default (dark background, light text).

### OpenCK current

- `ui/mainwindow.ui:167-172` defines `menuTheme` with a single
  stub action `actionNotImplementedTheme` (`enabled=false`).
- `src/view/window/preferencesdialog.cpp:148-155` has an
  "Appearance" group with a `mThemeCombo` offering "Dark", "Light",
  "System".
- `src/view/window/preferencesdialog.cpp:483-488` calls
  `ThemeManager::applyTheme(*app, theme)` on save — so a
  `ThemeManager` class exists and works via Preferences.
- The Theme **menu** is not wired to anything.

### Gaps

1. **Theme menu is a stub**: `actionNotImplementedTheme`
   (`ui/mainwindow.ui:373-380`) does nothing. Should be replaced
   with three actions: Default, Plastique, Plastique Dark (or our
   equivalents: System, Light, Dark — see legal note below).
2. **No QSS shipped**: OpenCK has no `.qss` file in the repo. The
   dark theme is applied programmatically via `ThemeManager`
   (`preferencesdialog.cpp:484-487`). Should ship a
   `resources/dark.qss` to match the CK's appearance and to make
   the ADS docks theme consistently.
3. **No ADS-specific stylesheet**: real CK ships
   `DefaultQtAdvancedDocking.css` to style dock tabs, splitters,
   etc. OpenCK's ADS uses default Qt styling, which looks out of
   place next to a dark theme.
4. **Theme choice lives in Preferences, not Theme menu**: the real
   CK exposes theme switching as a top-level Theme menu for quick
   switching. OpenCK buries it in Preferences → General →
   Appearance (`preferencesdialog.cpp:148-155`). Both should exist.
5. **Default theme not applied on startup**: `mainwindow.cpp`
   constructor (`mainwindow.cpp:89-154`) does not call
   `ThemeManager::applyTheme`. Theme is only applied when the user
   saves Preferences (`preferencesdialog.cpp:483-488`). First run
   is un-themed.

**Legal note** (per `AGENTS.md`): "Plastique" is a Qt style name
(not Bethesda's), so `actionPlastique` / `actionPlastique_Dark` as
*concept names* are fine, but the *text strings* should be our own
wording. Use "Dark", "Light", "System" or similar.

**Priority**: High.

---

## 5. Render Window toolbar — **Critical**

### Reference (real CK)

The Render Window toolbar (confirmed action names from the binary):

- `actionSnap_to_Grid`, `actionSnap_to_Angle`,
  `actionSnap_to_Connect_Points`
- `actionLockToXAxis`, `actionLockToYAxis`
- `actionGround_Plane`, `actionWireframe`,
  `actionToggleCollision`, `actionToggleLighting`
- `actionShow_Grid_Lines`, `actionSpace_Grid`,
  `actionToggle_3DGrid`
- `actionTopDownView`, `actionIsometric`, `actionPreview`,
  `actionPreviewOrbit`
- `actionResetCamera`, `actionPanToSelection`, `actionTop`
- `actionCopy_Render`, `actionCut_Render`, `actionPaste_Render`,
  `actionPaste_in_Place`

### OpenCK current

`src/view/window/nifviewportwidget.cpp:387-531` builds the toolbar:

- Edit-mode group (`nifviewportwidget.cpp:390-404`): Select, Move,
  Rotate, Scale (checkable, exclusive).
- `actionSnapGrid` ("Snap to Grid", `nifviewportwidget.cpp:413`),
  `actionSnapAngle` ("Snap to Angle", line 418), with
  `mSnapAngleSpin` and `mSnapGridSpin` spinboxes (lines 424-438).
- "Export NIF..." (`nifviewportwidget.cpp:442`), "Edit Mesh..."
  (line 445).
- Texture filter combo (lines 456-467).
- Toggle buttons (`nifviewportwidget.cpp:470-523`): Wireframe,
  Grid, Axis, Bounds, Collision, Cell Grid.
- "Hierarchy" toggle (`nifviewportwidget.cpp:526-530`).
- Animation toolbar (`setupAnimToolbar`, line 535) and particle
  toolbar (`setupParticleToolbar`, line 536) — OpenCK-original.

### Gaps

1. **Missing snap actions**: `actionSnap_to_Connect_Points`
   (real CK) — OpenCK has none.
2. **Missing axis-lock actions**: `actionLockToXAxis`,
   `actionLockToYAxis` — OpenCK has no axis-lock toggles. Move is
   unconstrained.
3. **Missing `actionGround_Plane`**: real CK has a ground-plane
   toggle. OpenCK's "Grid" button (`nifviewportwidget.cpp:479`) is
   the closest but draws a full 3D grid, not a ground plane.
4. **Missing `actionToggleLighting`**: real CK toggles lighting
   in the render window. OpenCK has no lighting toggle (lighting is
   always on in the shader, `nifviewportwidget.cpp:855-941`).
5. **Missing grid variants**: real CK has three grid actions —
   `actionShow_Grid_Lines`, `actionSpace_Grid`,
   `actionToggle_3DGrid`. OpenCK has one "Grid" button
   (`nifviewportwidget.cpp:479`).
6. **Missing view presets**: `actionTopDownView`,
   `actionIsometric`, `actionPreview`, `actionPreviewOrbit`,
   `actionTop` — OpenCK has none. Camera is free-orbit only.
7. **Missing camera actions**: `actionResetCamera`,
   `actionPanToSelection` — OpenCK has neither. There is no
   "frame selection" / "reset view" button.
8. **Missing render clipboard**: `actionCopy_Render`,
   `actionCut_Render`, `actionPaste_Render`,
   `actionPaste_in_Place` — OpenCK has no in-viewport
   copy/cut/paste of references.
9. **"Export NIF..." and "Edit Mesh..." not in real CK toolbar**:
   these are OpenCK-original toolbar items
   (`nifviewportwidget.cpp:442,445`). Real CK does not have them in
   the Render Window toolbar.
10. **Texture filter combo not in real CK toolbar**:
    `nifviewportwidget.cpp:460-464` adds a Linear/Nearest/Mipmap
    combo — OpenCK-original.
11. **"Axis", "Cell Grid", "Hierarchy" not in real CK toolbar**:
    `nifviewportwidget.cpp:488,516,526` — OpenCK-original buttons.
12. **Toolbar is in the dock widget, not the main window**: real
    CK's Render Window toolbar is part of the central render
    area. OpenCK's toolbar is inside the `NifViewportWidget`
    (`nifviewportwidget.cpp:387`), which is itself inside an ADS
    dock on the right (`mainwindow.cpp:964-967`). When the 3D
    Viewport is moved, the toolbar moves with it — but it should
    be the central, always-visible toolbar.

**Priority**: Critical.

---

## 6. Form dialog / Inspector — **High**

### Reference (real CK)

The real CK calls the properties window **"Inspector"** (confirmed
dock title). It is a docked panel (not a modal dialog) with
sections for: components, keywords, scripts, references, and the
per-record property grid. Selecting a record in the Object Window
populates the Inspector in-place.

### OpenCK current

`src/view/window/qtformdialog.cpp:14-68` defines `QtFormDialog`:

- It is a `QDialog` (line 16), `setModal(false)` (line 22).
- Title: `"Form — <formIdKey>"` (line 20) — **not** "Inspector".
- Two tabs (`m_tabs`, line 26): "Properties" (line 38) and "Data"
  (line 43).
- Properties tab: a scrollable `EditorPropertyGrid`
  (`qtformdialog.cpp:32-36`) populated from `FormComponents`
  (lines 47-56).
- Data tab: a custom widget set via `setCustomWidget`
  (`qtformdialog.cpp:72-84`). Per-record widgets
  (`NpcRecordDataWidget`, `RaceDataWidget`, etc.) are registered
  with `QtFormDialogManager` (`objectwindowdialog.cpp:63-108`).
- Buttons: Apply, OK, Cancel (`qtformdialog.cpp:58-63`).

Each record open creates a **new floating dialog** via
`QtFormDialogManager::openOrFocus` — there is no single docked
Inspector.

### Gaps

1. **Wrong name**: "Form — 0x..." vs "Inspector". The dock title
   should be "Inspector" (`qtformdialog.cpp:20`).
2. **Not docked**: `QtFormDialog` is a `QDialog`, not an ADS
   `CDockWidget`. The real CK's Inspector is a dock. OpenCK pops
   one dialog per record (`objectwindowdialog.cpp:269`, `openOrFocus`
   call), leading to window sprawl.
3. **No "single Inspector" mode**: real CK has one Inspector that
   updates as selection changes. OpenCK opens a new dialog per
   formId. Should add an option to host the property grid inside a
   docked Inspector that reuses one widget and re-binds on
   selection.
4. **Missing sections**: real CK Inspector has sections for
   Keywords, Scripts, Components, References. OpenCK's
   `EditorPropertyGrid` only renders the component property leaves
   — no dedicated Keywords/Scripts panels
   (`qtformdialog.cpp:34-36`).
5. **Tabs vs sections**: real CK uses collapsible sections, not
   tabs. OpenCK's "Properties" / "Data" tab split
   (`qtformdialog.cpp:38,43`) is an OpenCK pattern.
6. **Apply/OK/Cancel inappropriate for a dock**: a docked Inspector
   applies changes live (or on focus loss). The button row
   (`qtformdialog.cpp:58-63`) implies a modal edit session that
   doesn't match the dock model.

**Priority**: High.

---

## 7. Status bar — **Medium**

### Reference (real CK)

The real CK status bar shows: cell coordinates, object counts,
selected object info, and the Warnings dock feeds off the same
data. There is a `actionStatusbar` toggle (confirmed in binary).

### OpenCK current

`src/view/window/mainwindow.cpp:123-143` sets up the status bar:

- `mStatusRecordCount` — "Records: 0" (`mainwindow.cpp:124-126`).
- `mStatusCellCoords` — "Cell: -, -" (`mainwindow.cpp:128-130`).
- `mStatusSelectedObject` — "No selection" (`mainwindow.cpp:132-134`).
- `mStatusPluginInfo` — "No plugin loaded" (`mainwindow.cpp:136-138`).
- `mStatusProgressBar` — permanent, hidden until needed
  (`mainwindow.cpp:140-143`).

`updateStatus` (`mainwindow.cpp:673-676`) shows a transient
message via `statusBar()->showMessage`.

### Gaps

1. **`mStatusCellCoords` never updated**: nothing writes to
   `mStatusCellCoords` after construction
   (`mainwindow.cpp:128-130`). It always reads "Cell: -, -". The
   Cell View panel selection (`cellsdialog.cpp:416-434`) does not
   push coordinates back to the main window.
2. **`mStatusSelectedObject` never updated**: same — never
   written after init (`mainwindow.cpp:132-134`). The Object
   Window selection (`objectwindowdialog.cpp:744-750`) does not
   update the status bar.
3. **`mStatusRecordCount` only set once**: set to "Records: 0" at
   construction (`mainwindow.cpp:125`) and updated only via
   `updateStatus("Data loaded")` (line 176) which shows a message,
   not a count. Should reflect `mData->totalRecordCount()`.
4. **No `actionStatusbar` toggle**: real CK View menu has
   `actionStatusbar` to show/hide the status bar. OpenCK has no
   such toggle.
5. **Warnings not in status area**: real CK surfaces warnings in
   a dock; OpenCK pops a `QMessageBox` (`mainwindow.cpp:1092-1099`).

**Priority**: Medium.

---

## 8. Cell View panel — **Medium**

### Reference (real CK)

The real CK "Cell View" dock (confirmed title) is a docked panel on
the right with: worldspace selector, cell list, 2D top-down map,
and a reference table for the selected cell. Selecting a cell
populates the map and reference table; selecting a reference
focuses the Render Window on it.

### OpenCK current

`src/view/window/cellsdialog.cpp:307-397` defines `CellViewPanel`:

- Worldspace combo (`cellsdialog.cpp:319-334`) — first entry
  "(All cells)".
- Horizontal splitter (`cellsdialog.cpp:337`): cell `QListView`
  (line 340) on the left, vertical splitter (line 346) with
  `CellMapCanvas` (line 349) and `RefrTableModel` `QTableView`
  (line 352) on the right.
- Bottom toolbar (`cellsdialog.cpp:365-373`): "Go to cell",
  "Filter", "Refresh" actions — all placeholder.
- `CellMapCanvas` (`cellsdialog.cpp:32-118`) paints a black
  background with a 32px grid, center crosshair, and green dots
  for references. Header text shows "Cell (x, y)  Refs: N".
- Docked via `ads::CDockWidget("Cell View")` to
  `RightDockWidgetArea` (`mainwindow.cpp:1331-1333`).

### Gaps

1. **On-demand creation**: real CK opens with Cell View visible.
   OpenCK creates it only when `actionCells` is triggered
   (`mainwindow.cpp:1320-1339`). Should be created in `setData()`.
2. **Bottom toolbar actions are placeholders**: "Go to cell",
   "Filter", "Refresh" (`cellsdialog.cpp:365-373`) — only
   "Refresh" is wired (line 382-386, and it just resets the
   worldspace combo). "Filter" logs a placeholder message (line
   393-396). "Go to cell" only scrolls the list (line 387-392).
3. **No Render Window linkage**: selecting a reference in
   `mRefrTable` does nothing — there is no
   `connect(mRefrTable->selectionModel(), ...)` to focus the 3D
   viewport on the reference.
4. **Map canvas is minimal**: `CellMapCanvas::paintEvent`
   (`cellsdialog.cpp:57-112`) draws dots but no cell boundaries, no
   north indicator, no zoom/pan. Real CK's cell map is
   interactive.
5. **No "Go to worldspace" entry**: real CK lets you type a
   worldspace editor ID. OpenCK only has the combo
   (`cellsdialog.cpp:319`).
6. **No "Active cell" highlight**: real CK highlights the
   currently-rendered cell. OpenCK's map does not track the
   render window's active cell.
7. **Reference table columns limited**: `RefrTableModel`
   (`cellsdialog.cpp:212-305`) has 4 columns: Editor ID, Form ID,
   Position, Base Object. Real CK shows more (scale, rotation,
   flags, etc.).

**Priority**: Medium (structurally close, but several workflow
gaps).

---

## 9. Main toolbar — **High**

### Reference (real CK)

The real CK main window toolbar has: New, Open, Save, Save All,
Check Out, Check In, Undo, Redo.

### OpenCK current

`ui/mainwindow.ui:224-236` defines `mainToolBar` with:

- `actionOpenButton` (Open)
- `actionSaveButton` (Save)
- separator
- `actionUndoButton` (Undo)
- `actionRedoButton` (Redo)

`mainwindow.cpp:146-147` connects the Undo/Redo buttons to the
Edit-menu handlers.

### Gaps

1. **No "New" button**: real CK toolbar starts with New.
   OpenCK's `actionNew_Plugin` exists in the File menu
   (`ui/mainwindow.ui:264`) but is not on the toolbar.
2. **No "Save All" button**: real CK has `actionSaveAll`. OpenCK
   has no `actionSaveAll` at all (File menu or toolbar).
3. **No "Check Out" / "Check In" buttons**: real CK has these for
   Perforce integration. OpenCK has no version-control actions
   (Preferences notes VC is "not yet implemented",
   `preferencesdialog.cpp:344-355`).
4. **No "Data" button**: real CK's `actionData` (open plugin) is
   the primary entry. OpenCK has `actionOpenButton` which emits
   `actionData_triggered` (`mainwindow.cpp:1031-1034`) —
   functionally correct but the tooltip says "Load Master/Plugin
   Files (Ctrl + O)" (`ui/mainwindow.ui:733`), not "Data".
5. **Toolbar is not customizable at runtime**: OpenCK has a
   `ToolbarCustomizationDialog` (`mainwindow.cpp:646-651`) which
   is OpenCK-original and not in the real CK, but useful.
6. **Button-only, no icons**: `ui/mainwindow.ui:728-759` defines
   the toolbar actions as text-only (`actionOpenButton` etc.).
   Real CK toolbar uses icons.

**Priority**: High.

---

## 10. Keyboard shortcuts — **High**

### Reference (real CK)

The 524 actions include many with shortcuts (the binary lists
shortcut strings alongside action names). Key examples: Save
(Ctrl+S), Open (Ctrl+O), Undo (Ctrl+Z), Redo (Ctrl+Y), Save All
(Ctrl+Shift+S), Find Text, etc.

### OpenCK current

`src/view/window/mainwindow.cpp:454-658` (`setupShortcuts` /
`applyShortcuts`) wires shortcuts via a `ShortcutManager`
(`src/model/world/shortcutmanager.hpp`). Shortcuts are applied
to:

- File: NewPlugin, OpenPlugin, SavePlugin, SaveAsPlugin,
  ClosePlugin, Exit (`mainwindow.cpp:484-495`).
- Edit: Undo, Redo, Cut, Copy, Paste, Duplicate, SelectAll,
  SearchAndReplace, FindNext, FindPrevious
  (`mainwindow.cpp:500-519`).
- View: ObjectWindow, NifViewport, ScriptEditor, DialogueEditor,
  DialogueTree, QuestGraph, AIPackages, WeatherLight, Navmesh,
  WaterEditor, CellTransitions, MaterialEditor, PapyrusDebugger,
  FormIdEditor, Refresh (`mainwindow.cpp:532-570`).
- World: Worldspaces, Cells, LandscapeEditing, ObjectPalette
  (`mainwindow.cpp:575-581`).
- Plugins: LoadOrder, MasterFiles, ConflictDetection,
  ConflictResolution, PluginMerge, LoadOrderOptimizer,
  BashedPatch, ExternalTools (`mainwindow.cpp:586-598`).
- Misc: Preferences, Validate, About (`mainwindow.cpp:603-607`).
- Navigation: NextRecord, PreviousRecord, FirstRecord, LastRecord,
  ExpandAll, CollapseAll (`mainwindow.cpp:612-617`).
- Render: ToggleGrid, ToggleBoundingBoxes, ToggleWireframe
  (`mainwindow.cpp:627-643`).

UI-file-defined shortcuts (`ui/mainwindow.ui`): Ctrl+O (Data,
line 250), Ctrl+S (Save, 261), Ctrl+N (New Plugin, 269),
Ctrl+Shift+S (SaveAs, 277), Ctrl+W (Close Plugin, 285), Ctrl+Z
(Undo, 444), Ctrl+Y (Redo, 452), Ctrl+C (Copy, 460), Ctrl+X (Cut,
468), Ctrl+V (Paste, 476), Ctrl+D (Duplicate, 484), Delete
(Delete, 492), Ctrl+A (SelectAll, 500), F3 (FindNext, 508),
Shift+F3 (FindPrevious, 516), Ctrl+F (SearchAndReplace, 524),
Down/Up/Home/End (record navigation, 532-556), Ctrl+Plus/Ctrl+Minus
(expand/collapse, 564/572), F6 (3DViewport, 585), F7 (ScriptEditor,
593), F8 (DialogueEditor, 601), F9 (DialogueTree, 609), F10
(QuestGraph **and** AIPackages — conflict, lines 617/625), F11
(WeatherLight, 633), F12 (Navmesh, 641), Ctrl+F11 (Water, 649),
Ctrl+F12 (CellTransitions, 657), F13 (MaterialEditor, 665),
Ctrl+Shift+W (Worldspaces **and** WorldView — conflict,
lines 683/707), Ctrl+Shift+C (Cells, 691), Ctrl+L (LandscapeEditing,
699), Ctrl+P (ObjectPalette, 715).

### Gaps

1. **Shortcut conflict**: F10 is bound to both `actionQuestGraph`
   (`ui/mainwindow.ui:617`) and `actionAIPackages` (line 625).
   `applyShortcuts` (`mainwindow.cpp:541,543`) re-assigns via
   `ShortcutManager`, but the UI file has two actions claiming F10.
2. **Shortcut conflict**: Ctrl+Shift+W is bound to both
   `actionWorldspaces` (`ui/mainwindow.ui:683`) and
   `actionWorldView` (line 707).
3. **Missing real-CK shortcuts**: Save All (Ctrl+Shift+S in real
   CK) — OpenCK uses Ctrl+Shift+S for SaveAs (`ui/mainwindow.ui:277`)
   and has no Save All. Find Text (real CK `actionFind_Text`) —
   OpenCK's Find is bound to Ctrl+F via `SearchAndReplace`
   (`mainwindow.cpp:514-515`), close but the action name differs.
4. **Render-window shortcuts missing**: real CK has many
   viewport-specific shortcuts (snap toggles, axis locks, view
   presets). OpenCK only has ToggleGrid, ToggleBoundingBoxes,
   ToggleWireframe (`mainwindow.cpp:627-643`).
5. **No customizable shortcut UI exposed in main menu**: a
   `ShortcutEditorDialog` is imported (`mainwindow.cpp:61`) but
   not wired to a menu action.
6. **F13+ may not work on all keyboards**: `actionMaterialEditor`
   uses F13 (`ui/mainwindow.ui:665`). Real CK avoids F13+.
7. **No shortcut for `actionInspector` / `actionWarnings`**: real
   CK View menu has these with shortcuts. OpenCK has no Inspector
   dock action at all.

**Priority**: High.

---

## Prioritized fix list

Ordered by severity, then by leverage (fixes that unblock other
fixes come first).

### Critical

1. **Make the Render Window the central widget.** Replace the
   empty `centralWidget` (`ui/mainwindow.ui:19`) with a
   `NifViewportWidget` (or a thin wrapper). Move the 3D Viewport
   out of the ADS right dock (`mainwindow.cpp:964-967`) into the
   central area. Create it in `setData()`, not on-demand.
   *Unblocks: Render Window toolbar (§5), dock layout (§3).*

2. **Add an "Inspector" dock** that hosts the record editor as an
   ADS `CDockWidget("Inspector")` on the right, reusing
   `QtFormDialog`'s `EditorPropertyGrid`. Add a single-Inspector
   mode that re-binds on Object Window selection instead of
   opening one dialog per record. Rename the dialog title to
   "Inspector" (`qtformdialog.cpp:20`). *Unblocks: Form dialog
   (§6).*

3. **Add a "Warnings" dock** (`ads::CDockWidget("Warnings")`)
   that receives validation messages instead of popping
   `QMessageBox` (`mainwindow.cpp:1092-1099`). Wire
   `actionWarnings` in the View menu to toggle it.

4. **Create Cell View in `setData()`** instead of on-demand
   (`mainwindow.cpp:1320-1339`). Dock it right. *Unblocks: Cell
   View (§8).*

5. **Rewrite the Object Window tree to 9 groups / ~80 categories.**
   Add the "All Forms" group first, add the "Miscellaneous" group
   last, rename "World" group, add the ~60 missing record
   categories with their `CkId::Type` cases in
   `ObjectWindowModel::initCategories` (`objectwindow.cpp:30-323`).
   Add Cell/Worldspace/Refr/Navmesh/Landscape categories to the
   model switch. *Unblocks: Object Window tree (§2).*

6. **Realign the menu bar to the real CK's 16 menus.** Rename
   "ObjectWindows"→"Object Windows", "RenderWindows"→"Render
   Windows" (`ui/mainwindow.ui:112,120`). Relocate the `Plugins`
   and `Tools` menus into File submenus or a dedicated area.
   Populate Character, World, ObjectWindows, RenderWindows,
   Navmesh, Terrain, Audio, Docks, Theme, Tests with the real CK
   action names. *Unblocks: Menu bar (§1).*

7. **Rebuild the Render Window toolbar** to include the real CK
   actions: `Snap_to_Connect_Points`, `LockToXAxis`, `LockToYAxis`,
   `Ground_Plane`, `ToggleLighting`, `Show_Grid_Lines`,
   `Space_Grid`, `Toggle_3DGrid`, `TopDownView`, `Isometric`,
   `Preview`, `PreviewOrbit`, `ResetCamera`, `PanToSelection`,
   `Top`, `Copy_Render`, `Cut_Render`, `Paste_Render`,
   `Paste_in_Place` (`nifviewportwidget.cpp:387-531`). *Unblocks:
   Render Window toolbar (§5).*

### High

8. **Wire the Theme menu** (`ui/mainwindow.ui:167-172`) to three
   actions (Default/Plastique/Plastique Dark or our
   System/Light/Dark) that call `ThemeManager::applyTheme`. Apply
   the default dark theme in the `MainWindow` constructor
   (`mainwindow.cpp:89-154`). Ship a `resources/dark.qss` and an
   ADS-specific stylesheet. *Unblocks: Theming (§4).*

9. **Expand the main toolbar** (`ui/mainwindow.ui:224-236`) to
   include New, Save All. Add `actionSaveAll` to the File menu and
   wire it. Add Check Out/Check In placeholder actions (disabled
   until VC is implemented). *Unblocks: Main toolbar (§9).*

10. **Fix keyboard shortcut conflicts** (F10, Ctrl+Shift+W)
    (`ui/mainwindow.ui:617,625,683,707`). Add a menu action for
    `ShortcutEditorDialog` (`mainwindow.cpp:61`). Add viewport
    shortcuts for snap/axis-lock/view presets. *Unblocks: Shortcuts
    (§10).*

11. **Add Editor ID / Form ID / Position / Scale / Rotation columns
    to the Cell View reference table** (`cellsdialog.cpp:212-305`).
    Wire reference selection to focus the Render Window. Make the
    map canvas interactive (zoom/pan, cell boundary highlight).

### Medium

12. **Wire status bar updates.** Make `mStatusCellCoords` and
    `mStatusSelectedObject` update on Cell View / Object Window
    selection (`mainwindow.cpp:128-134`). Update
    `mStatusRecordCount` from `mData` after load. Add
    `actionStatusbar` toggle to the View menu.

13. **Remove OpenCK-original View-menu editor launchers**
    (`ui/mainwindow.ui:81-90`) and relocate them to the
    appropriate Character/World/Audio menus (or a dedicated
    "Editors" submenu), to match the real CK's View menu which is
    purely viewport toggles.

14. **Rename "Object Palette" to "Object palette"**
    (`mainwindow.cpp:207`) to match the real CK's lowercase 'p'.

15. **Add `actionSaveAll`** and `actionRevert_file` to the File
    menu (`ui/mainwindow.ui:29-49`). Add the compact/convert master
    actions (`actionCompactMediumMaster`,
    `actionConvertLargeMaster`, etc.) as a "Master Files" submenu
    of File.

16. **Add `actionManageLayouts`, `actionSaveCurrentLayout`,
    `actionSaveLayoutAs`, `actionApplyLayout`, `actionSnapWindows`**
    to the Docks menu (`ui/mainwindow.ui:158-166`), replacing the
    OpenCK-original `actionSaveLayout`/`actionLoadLayout` names.
    Wire `windowlayout.cpp` to a layouts directory.

### Low

17. **Fix category display names** in
    `ObjectWindowModel::initCategories`: "Tree Node"→"Tree",
    "Global Variable"→"Global", "Game Settings"→"Game Setting",
    "Information"→"Info" (`objectwindow.cpp:262-296`).

18. **Add icons to the main toolbar** (`ui/mainwindow.ui:728-759`).

19. **Remove the stub `actionNotImplementedGalaxy` /
    `actionNotImplementedPackin`** and replace with the real CK
    action names once extracted from the binary, or leave the
    menus hidden until implemented.

20. **Move "Export NIF..." and "Edit Mesh..." out of the Render
    Window toolbar** (`nifviewportwidget.cpp:442,445`) into a
    context menu or a separate "Tools" submenu, to match the real
    CK toolbar which has only viewport actions.

---

## Appendix: file-to-area cross-reference

| Area | Primary files |
|------|---------------|
| Menu bar | `ui/mainwindow.ui:20-223` (menu definitions), `src/view/window/mainwindow.cpp:228-451` (runtime wiring) |
| Object Window tree | `src/model/window/objectwindow.cpp:30-323` (categories + groups), `src/view/window/objectwindowdialog.cpp:48-180` (UI) |
| Dock layout | `src/view/window/mainwindow.cpp:89-226` (dock creation), `src/view/window/windowlayout.cpp:1-51` (layout persistence) |
| Theming | `ui/mainwindow.ui:167-172` (Theme menu stub), `src/view/window/preferencesdialog.cpp:148-155,483-488` (Theme combo), `src/view/window/thememanager.hpp` (manager) |
| Render Window toolbar | `src/view/window/nifviewportwidget.cpp:387-531` (toolbar build) |
| Form dialog / Inspector | `src/view/window/qtformdialog.cpp:14-97`, `src/view/window/objectwindowdialog.cpp:63-108` (factory registration) |
| Status bar | `src/view/window/mainwindow.cpp:123-143,673-676` |
| Cell View panel | `src/view/window/cellsdialog.cpp:307-435` |
| Main toolbar | `ui/mainwindow.ui:224-236`, `src/view/window/mainwindow.cpp:146-147` |
| Keyboard shortcuts | `src/view/window/mainwindow.cpp:454-658`, `src/model/world/shortcutmanager.hpp` |