# OpenCK UI Layout Audit — vs. Real Starfield Creation Kit

A systematic comparison of OpenCK's current UI layout against the real
Bethesda Starfield Creation Kit (`CreationKit.exe` v1.16.244.0, Qt5 +
QtAdvancedDocking). Source for the reference side is
`docs/CK_Real_Integration_Plan.md` (recovered source-file map and 524
actions) plus the shipped `Data\DataViews\ObjectWindow\_common\*.filter`
files and `CreationKit.ini`. Source for the OpenCK side is the current
tree under `src/` and `ui/`.

This document drives the next phase of UI work. For every gap it names
the file(s) that must change and what to do. Priorities:

- **Critical** — blocks core usability (the editor cannot be used like the CK)
- **High** — visually jarring or breaks a common workflow
- **Medium** — nice-to-have, parity gap that doesn't block work
- **Low** — cosmetic

---

## Summary table

| # | Area | Real CK | OpenCK | Gap severity |
|---|------|---------|--------|-------------|
| 1 | Top-level menu bar | 16 menus (File, Edit, View, Character, Gameplay, World, ObjectWindows, RenderWindows, Navmesh, Terrain, Audio, Galaxy, Packin, Docks, Theme, Tests, Help) | 9 menus (File, Edit, View, World, Plugins, Export, Tools, Gameplay, Help) | **Critical** |
| 2 | Object Window tree | Hierarchical: All → Actors / Items / World Objects / Gameplay / Audio / Dialogue / … each expanding to record types | Flat list of 27 categories, no parent groups | **Critical** |
| 3 | Dock layout / window arrangement | QtAdvancedDocking (`CDockManager`); Object Window left, Cell View top-right, Render Window center; tear-off/tab/redock | `QDockWidget`; Object Window on **right**, no Cell View dock, no ADS | **Critical** |
| 4 | Cell View panel | `TESCellView.cpp` — docked 2D top-down cell browser with cell list + references | `cellsdialog.cpp` — modal `QDialog` opened from World > Cells; no dock, no reference list | **Critical** |
| 5 | Render Window toolbar | Selection/Move/Rotate/Scale mode toggles, snap toggles (angle/grid), grid toggles, landscape tools | `nifviewportwidget.cpp` — grid/axis/bounds/wireframe/collision/cell-grid toggles + NIF export/filter; no transform-mode tools, no snap toggles | **High** |
| 6 | Property editor (`QtFormDialog`) | `QtCreationKitFormDialog` — property grid on top, per-form data widgets below, OK/Cancel at bottom | `qtformdialog.cpp` — scrollable `EditorPropertyGrid` then custom widget inserted before button box; Apply/OK/Cancel at bottom | **Medium** |
| 7 | Preferences dialog | `TESPreferences.cpp` — tree-based category sidebar (Display, Edit, Sound, Network, …) | `preferencesdialog.cpp` — flat `QGroupBox` stack (General / Appearance / Miscellaneous), no tree sidebar | **High** |
| 8 | Status bar | Cell coordinates, object counts, selected object info | Record count label + plugin info label + progress bar | **Medium** |

---

## 1. Top-level menu bar — **Critical**

### Reference (real CK)

`docs/CK_Real_Integration_Plan.md:63-69` lists the 16 top-level menus
recovered from the binary:

```
File, Edit, View, Character, Gameplay, World, ObjectWindows,
RenderWindows, Navmesh, Terrain, Audio, Galaxy, Packin, Docks,
Theme, Tests, Help
```

524 actions are wired across those menus (ibid. §"Top-level menus").

### OpenCK current

`ui/mainwindow.ui:126-134` wires 9 menus into the menu bar:

```
File, Edit, View, World, Plugins, Export, Tools, Gameplay, Help
```

`src/view/window/mainwindow.cpp:212-436` (`setupEditMenu`) also injects
actions into `menuExport`, `menuTools`, and `menuView` at runtime.

### Gaps

#### 1.1 Missing top-level menus — **Critical**

7 of the 16 real-CK menus have no OpenCK equivalent at all:

| Real CK menu | OpenCK status | Where to add |
|---|---|---|
| Character | absent | new `QMenu menuCharacter` in `ui/mainwindow.ui`; wire NPC/Race/Class/Faction/BodyPart/HeadPart actions |
| ObjectWindows | absent (the View menu shows a single Object Window toggle instead) | new `QMenu menuObjectWindows` listing Object Window / Cell View / Object Palette / Galaxy View / Scene View / Find Forms |
| RenderWindows | absent | new `QMenu menuRenderWindows` listing Render Window / Preview Window / Lighting / Reflection Probes toggle |
| Navmesh | absent | new `QMenu menuNavmesh`; the existing `actionNavmesh` (World-adjacent) moves here |
| Terrain | absent | new `QMenu menuTerrain` for landscape tools |
| Audio | absent | new `QMenu menuAudio`; the existing `actionSoundEditor` (currently in Tools) moves here |
| Galaxy | absent | defer (Starfield-only subsystem; stub menu for parity) |
| Packin | absent | defer (BGS packin subsystem; stub menu) |
| Docks | absent | new `QMenu menuDocks` exposing the ADS show/hide/restore-layout actions |
| Theme | absent | new `QMenu menuTheme` (real CK has QSS theme swatches) |
| Tests | absent | new `QMenu menuTests` for internal validation toggles |

File: `ui/mainwindow.ui:20` (the `<widget class="QMenuBar">` block).
Each new menu is a `<widget class="QMenu" name="menuX">` plus an
`<addaction name="menuX"/>` entry in the menubar action list
(`ui/mainwindow.ui:126-134`).

#### 1.2 Actions in the wrong menu — **High**

| OpenCK action | OpenCK location | Real CK location | Fix |
|---|---|---|---|
| `actionObjectWindow` | View (`ui/mainwindow.ui:75`) | ObjectWindows | move to new `menuObjectWindows` |
| `actionNavmesh` | View (`ui/mainwindow.ui:83`) | Navmesh | move to new `menuNavmesh` |
| `actionMaterialEditor` | View (`ui/mainwindow.ui:86`) | (real CK has `actionOpen_Material_Editor` in a Tools/File-adjacent spot; integration plan lumps it with deferred editors) | keep in View until a dedicated menu exists |
| `actionSoundEditor` | Tools (`ui/mainwindow.ui:217`) | Audio | move to new `menuAudio` |
| `actionParticleEffectsEditor` | Tools (`ui/mainwindow.ui:219`) | (real CK has `actionOpen_Particle_Editor`) | keep in Tools or move to a Render/Effects menu |
| `actionAnimationEditor` | Tools (`ui/mainwindow.ui:218`) | Character/RenderWindow | move to `menuCharacter` or `menuRenderWindows` |
| `actionLandscapeEditing` | World (`ui/mainwindow.ui:97`) | Terrain | move to new `menuTerrain` |
| `actionWorldspaces`, `actionCells` | World (`ui/mainwindow.ui:94-95`) | World (correct) | keep |
| `actionObjectPalette` | World (`ui/mainwindow.ui:99`) | ObjectWindows | move to `menuObjectWindows` |
| `actionPluginMerge`, `actionBashedPatch`, `actionLoadOrderOptimizer`, `actionExternalTools` | Plugins | (no direct CK analog; these are xEdit-style tools) | keep — but consider a Tools submenu |
| `actionExportDialogue`, `actionExportScripts`, `actionExportTextures` | Export | File > Export in real CK | either fold into File or keep as a separate menu — but the real CK does not have a top-level Export menu |

File: `ui/mainwindow.ui` (reorder `<addaction>` entries inside each
`<widget class="QMenu">` block). Runtime additions in
`src/view/window/mainwindow.cpp:212-436` (`setupEditMenu`) must move
their `ui->menuTools->addAction(...)` calls to the new menus.

#### 1.3 Actions present in OpenCK but not in real CK — **Medium**

The entire **Plugins** menu (`ui/mainwindow.ui:107-119`) is an OpenCK
addition (load-order optimizer, bashed patch, external tools). These
are useful but not CK-native. Keep them but document that they are
OpenCK extensions so the audit reflects the divergence.

#### 1.4 Actions in real CK that OpenCK lacks entirely — **High**

From `docs/CK_Real_Integration_Plan.md:71-82`, key missing actions:

- `actionCell_View_Window` (Cell View toggle)
- `actionGalaxy_View`
- `actionCompile_Papyrus_Scripts` (OpenCK has a debugger, not a compile action)
- `actionCreate_Archive` (BA2 creation — `Tools/Archive2/` exists but not wired)
- `actionCompactSmallMaster`, `actionCompactMediumMaster`
- `actionConvertLargeMaster`
- `actionCheckIn`, `actionCheckOut` (version control — defer)
- `actionClean_NavMesh_Splines`, `actionConnectNavmeshes_All_Interiors` (defer)

Add the non-deferred ones under the new menus above.

---

## 2. Object Window category tree — **Critical**

### Reference (real CK)

`docs/CK_Real_Integration_Plan.md:86-96` shows the real CK has a
hierarchical tree (`BGSObjectWindowTree.cpp`,
`BGSGenericCategoryLayout.cpp`). The top-level nodes expand into
record-type leaves. The shipped `.filter` files in
`Data\DataViews\ObjectWindow\_common\` (e.g. `Planets-Life.filter`)
show filter definitions keyed by `ParameterName` like
`"Keyword(s)"`, indicating the tree also exposes record properties as
filterable parameters.

The real CK hierarchy (observed by running it) is roughly:

```
All
├── Actors
│   ├── Creature
│   ├── NPC_             (TESNPC)
│   └── ...
├── Items
│   ├── Armor            (TESObjectARMO)
│   ├── Book             (TESObjectBOOK)
│   ├── Weapon           (TESObjectWEAP)
│   ├── Alchemy          (AlchemyItem)
│   ├── Ingredient       (TESObjectMISC / Ingr)
│   ├── Misc             (TESObjectMISC)
│   ├── Container        (TESObjectCONT)
│   └── ...
├── World Objects
│   ├── Static           (TESObjectSTAT)
│   ├── Activator        (TESObjectACTI)
│   ├── Movable Static   (BGSMovableStatic)
│   ├── Tree             (TESObjectTREE)
│   └── ...
├── Gameplay
│   ├── Quest            (TESQuest)
│   ├── Package          (TESPackage)
│   ├── Global           (TESGlobal)
│   ├── Game Setting     (TESGameSettings)
│   ├── Perk             (BGSPerk)
│   ├── Class            (TESClass)
│   ├── Faction          (TESFaction)
│   └── ...
├── Audio
│   ├── Sound           (TESSound)
│   ├── Music Type      (BGSMusicType)
│   └── ...
├── Dialogue
│   ├── Dialogue        (TESTopic)
│   └── Info            (TESTopicInfo)
└── ...
```

### OpenCK current

`src/model/window/objectwindow.cpp:240-267` builds a **flat** list of
27 categories via a chain of `addCategory("Name", CkId::Type_X)`
calls:

```
Game Settings, NPC, Weapon, Armor, Spell, Magic Effect, Quest,
Dialogue, Information, Global Variable, Location Reference Type,
Package, Tree Node, Alchemy, Ingredient, Container, Enchantment,
Book, Miscellaneous, Activator, Texture Asset (mislabeled STAT),
Race, Class, Faction, Perk, Sound, Weather, Land Texture
```

Notable problems beyond the flat-vs-hierarchical shape:

1. The STAT category is labeled **"Texture Asset"** (`objectwindow.cpp:260`)
   — the real CK calls it "Static". This is a naming bug.
2. There is no "All" root node.
3. There are no parent groups (Actors, Items, World Objects, Gameplay,
   Audio, Dialogue).
4. The record-type coverage is 27 of the real CK's 127
   `_Editor.cpp` record types (see integration plan §"TESForms").
5. The model in `objectwindow.cpp` is a 2-level tree only because
   `parent()` returns the category index for a record leaf
   (`objectwindow.cpp:441-455`) — there is no 3rd level for groups.

### What to change

| File | Change |
|---|---|
| `src/model/window/objectwindow.hpp` | Add a `CategoryGroup` struct (name + list of child `Category` indices); change `mCategories` to a 2-level structure: `QList<CategoryGroup> mGroups` where each group holds `QList<Category>`. |
| `src/model/window/objectwindow.cpp:29-268` | Replace `initCategories` with a version that first creates the parent groups (All, Actors, Items, World Objects, Gameplay, Audio, Dialogue, …) then adds each existing `addCategory` call under the correct group. Rename "Texture Asset" → "Static". |
| `src/model/window/objectwindow.cpp:418-470` | Extend `index()`/`parent()`/`rowCount()` for 3 levels: root → group → category → record. The current `internalId` scheme (`0` for top-level, `categoryId+1` for leaves) needs a third tier (e.g. `internalId == groupIndex+1` for category nodes, `internalId == (groupIndex+1)*1000 + categoryId+1` for leaves — or use a small `QModelIndex`-internal pointer struct). |
| `src/view/window/objectwindowdialog.cpp:130-142` | Tree view already has `setRootIsDecorated(true)`; once the model is 3-level the UI will show the groups. No UI change needed beyond model work. |

Priority: **Critical** — the flat list is the single biggest "this
doesn't look like the CK" issue after the menu bar.

---

## 3. Dock layout / window arrangement — **Critical**

### Reference (real CK)

`docs/CK_Real_Integration_Plan.md:11-16`: the real CK links
**QtAdvancedDocking** (`QtAdvancedDocking.dll`, MIT/LGPL). A
`CDockManager` owns all panels. Default arrangement (observed):

```
┌──────────────┬───────────────────────────────┐
│ Object Window│          Render Window         │
│  (left)      │       (center, largest)        │
│              ├───────────────────────────────┤
│              │           Cell View            │
│              │         (top-right)            │
└──────────────┴───────────────────────────────┘
```

Panels are tear-off, tab together, and persist to
`QtCreationKitSavedSettings.ini`
(`docs/CK_Real_Integration_Plan.md:56`).

### OpenCK current

- Uses **`QDockWidget`**, not ADS. `ui/mainwindow.ui:16-17` sets
  `dockOptions` to `AllowTabbedDocks|AnimatedDocks`. The vendored ADS
  library exists at `external/ads/` (per `AGENTS.md`) but is not used
  by the main window.
- `src/view/window/mainwindow.cpp:166-169` adds the Object Window to
  **`Qt::RightDockWidgetArea`** — the wrong side. The real CK puts it
  on the left.
- `src/view/window/windowlayout.cpp:8-81` (`applyDefaultLayout`) does
  try to place Object Window left and Viewport right, but this only
  runs on the "Reset Window Layout" action
  (`mainwindow.cpp:132-139`). The initial `setData()` call does not
  invoke it, so the default placement is wrong until the user manually
  resets.
- There is **no Cell View dock**. See §4.
- Render Window (`nifviewportwidget.cpp`) is added to
  `Qt::RightDockWidgetArea` (`mainwindow.cpp:951`) but only when the
  user toggles View > 3D Viewport; it is not a persistent central
  widget.

### Gaps

| Gap | Severity | Fix |
|---|---|---|
| No ADS / `CDockManager` integration | **Critical** | Workstream B in the integration plan. Replace `QDockWidget` usages in `mainwindow.cpp` (lines 165-205, 947-1009, 1170-1210) with `ads::CDockWidget` managed by a single `ads::CDockManager` member in `mainwindow.hpp`. The `external/ads/` CMake target already builds the shared lib. |
| Object Window on right by default | **High** | `mainwindow.cpp:168`: change `Qt::RightDockWidgetArea` → `Qt::LeftDockWidgetArea`, or better, call `WindowLayout::applyDefaultLayout(this)` at the end of `setData()`. |
| `applyDefaultLayout` only runs on manual reset | **High** | `mainwindow.cpp:112` calls `restoreUiState()` in the ctor but `setData()` (line 155) does not call `WindowLayout::applyDefaultLayout` after creating docks. Add a call at the end of `setData()` (line 205) so a freshly loaded document gets the correct layout. |
| No central Render Window | **High** | The real CK's Render Window is the central widget, not a dock. `mainwindow.cpp:941-963` creates it as a dock. Once ADS is in, the Render Window should be the `CDockManager`'s central dock widget (ADS supports a central widget that other docks attach to). |
| No layout persistence to a CK-style ini | **Medium** | `windowlayout.cpp:83-86` already saves `saveState()` to a `QSettings`. Rename the key to match the CK's `QtCreationKitSavedSettings.ini` convention so users migrating have a familiar filename (we keep our own contents). |

---

## 4. Cell View panel — **Critical**

### Reference (real CK)

`docs/CK_Real_Integration_Plan.md:99-102`:
`Genesis\Construction Set\Misc\TESCellView.cpp` is a **docked** 2D
top-down cell browser. It shows:

- A list of cells (interior + exterior) for the loaded worldspace.
- A list of references placed in the selected cell.
- A 2D top-down map area with selectable reference markers.

It is opened via `actionCell_View_Window` (integration plan §"Top-level
menus") and lives as a dock, not a modal dialog.

### OpenCK current

`src/view/window/cellsdialog.cpp:82-109`:

```cpp
CellsDialog::CellsDialog(Data* data, QWidget* parent)
    : QDialog(parent), mData(data), mTreeView(nullptr), mModel(nullptr)
{
    setWindowTitle("Cells");
    setMinimumSize(600, 400);
    ...
    auto* closeBtn = new QPushButton("Close");
    ...
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}
```

It is a **modal `QDialog`** with a single-column tree of cell editor IDs
(`CellTableModel::data` returns only `editorId`,
`cellsdialog.cpp:30-34`). There is:

- No reference list per cell.
- No 2D top-down map.
- No dock hosting — it is opened modally from
  `MainWindow::on_actionCells_triggered` (`mainwindow.cpp:1307-1318`).
- No worldspace selector.

### What to change

| File | Change |
|---|---|
| `src/view/window/cellsdialog.hpp` / `.cpp` | Rename to `CellViewPanel`; inherit `QWidget` (or `ads::CDockWidget`) instead of `QDialog`. Replace the single tree with a splitter: left = worldspace combo + cell list (`QListView`), right = 2D `QWidget` canvas (paint reference markers) + reference `QTableView` below. |
| `src/view/window/mainwindow.cpp:1307-1318` | Replace `CellsDialog dialog(mData, this); dialog.exec();` with dock creation (the same pattern used for `objectWindowDock` at line 165) and a toggle action in the new `menuObjectWindows`. |
| `ui/mainwindow.ui` | Add `actionCellView` under the new `menuObjectWindows`. |
| `src/view/window/windowlayout.cpp:61-68` | The `cellDock` branch already exists but is dead because no cell dock is ever created. Wire it once `CellViewPanel` exists. |

Priority: **Critical** — without a docked Cell View, the
Object-Window → Render-Window → Cell-View editing triangle the CK is
built around does not exist in OpenCK.

---

## 5. Render Window toolbar — **High**

### Reference (real CK)

`docs/CK_Real_Integration_Plan.md:104-124` lists the render-window
source files (`BGSRenderWindow*.cpp`,
`BGSRenderWindowEditModule.cpp`, `BGSRenderWindowGizmo.cpp`,
`BGSRenderWindowReferenceEditModule.cpp`, etc.). The render window
toolbar (observed by running the CK) has:

- **Selection** mode (default)
- **Move** (translate gizmo)
- **Rotate** (rotate gizmo)
- **Scale** (scale gizmo)
- **Snap to grid** toggle
- **Snap to angle** toggle (with angle increment)
- **Grid visibility** toggle
- **Bounds visibility** toggle
- **Landscape** tools (raise/lower/smooth/paint)
- **Navmesh** tools (deferred)
- **Falloff / brush size** for landscape

### OpenCK current

`src/view/window/nifviewportwidget.cpp:384-476` builds a `QToolBar`
with these buttons:

| Button | Line | Real-CK equivalent |
|---|---|---|
| Export NIF... | 386 | (no equivalent — OpenCK-only) |
| Edit Mesh... | 389 | (no equivalent — OpenCK-only) |
| Filter combo (Linear/Nearest/Mipmap) | 400-411 | (no equivalent — texture filter is an OpenCK extra) |
| Wireframe | 414-421 | (real CK has wireframe under a View menu, not toolbar) |
| Grid | 423-430 | Grid visibility toggle ✓ |
| Axis | 432-439 | (OpenCK-only convenience) |
| Bounds | 441-449 | Bounds visibility ✓ |
| Collision | 451-458 | (OpenCK-only debug view) |
| Cell Grid | 460-467 | (OpenCK-only debug view) |
| Hierarchy | 469-474 | (OpenCK-only node-tree panel) |

### Gaps

| Missing tool | Severity | Where to add |
|---|---|---|
| Selection / Move / Rotate / Scale mode toggles | **Critical** (for render-window editing) | `nifviewportwidget.cpp` toolbar — add a `QActionGroup` of 4 checkable actions. Wire them to a new `EditModule`-style state in the widget (the real CK's `BGSRenderWindowEditModuleManager.cpp` pattern). Deferred per integration plan but the toolbar placeholders should exist. |
| Snap to grid toggle + snap-to-angle toggle | **High** | `nifviewportwidget.cpp` toolbar — two checkable buttons + a snap-step spinbox. |
| Landscape tools (raise/lower/smooth/paint, brush size/falloff) | **Medium** (landscape is deferred) | A second toolbar row or a `menuTerrain`-driven palette. The `landscapeeditor.cpp` widget exists separately; wire its tools into the render toolbar when landscape mode is active. |
| Reference placement / drag tools | **Critical** (for cell editing) | Requires the Render Window to actually render cell references, not just a single NIF. This is the multi-week Render Window workstream (integration plan §"Render Window"). |

The existing grid/bounds/wireframe toggles are fine to keep — they
map to real-CK features. The transform-mode tools are the big gap.

---

## 6. Property editor (`QtFormDialog`) — **Medium**

### Reference (real CK)

`docs/CK_Real_Integration_Plan.md:127-149`: the real CK uses
`QtCreationKitFormDialog` (line 142) which composes:

1. A property **grid** on top (`BGSEditorPropertyGrid.cpp`, line 132)
   rendering the common `EditorProperty` leaves.
2. Per-form **data widgets** below (`QtTESFormWidget.cpp`,
   `QtFormDataWidget.cpp`, lines 137-139) for record-specific UI
   (e.g. the NPC actor-base-data widget, the container items table).
3. OK / Cancel buttons at the bottom.

The dialog is modeless and managed by
`QtCreationKitFormDialogManager.cpp` (line 143) so each record opens in
its own window and is focused if reopened.

### OpenCK current

`src/view/window/qtformdialog.cpp:13-51`:

```cpp
QtFormDialog::QtFormDialog(...)
{
    ...
    m_grid = new EditorPropertyGrid(scroll);
    ...
    m_grid->setComponents(componentPtrs);
    ...
    auto* buttons = new QDialogButtonBox(this);
    auto* applyBtn = buttons->addButton("Apply", ApplyRole);
    auto* okBtn = buttons->addButton(Ok);
    auto* cancelBtn = buttons->addButton(Cancel);
    ...
}
```

`setCustomWidget` (`qtformdialog.cpp:55-67`) inserts a per-form
widget **above** the button box (line 65:
`m_layout->insertWidget(m_layout->count() - 1, widget)`). This is the
correct CK pattern: grid on top, custom data widget below, buttons at
bottom.

`qtformdialogmanager.cpp` (referenced throughout
`objectwindowdialog.cpp:68-108`) handles the open-or-focus
singleton pattern, matching the real CK's
`QtCreationKitFormDialogManager`.

### Gaps

| Gap | Severity | Fix |
|---|---|---|
| Dialog is **modal** (`QDialog::exec`-style accept/reject) — the real CK's form dialogs are modeless windows | **Medium** | `qtformdialog.cpp`: change base class from `QDialog` to `QWidget` (or make it a `CDockWidget` under ADS). `QtFormDialogManager::openOrFocus` should `show()` + `raise()` instead of relying on `exec()`. |
| Tab structure for multi-section forms (the real CK groups components into tabs: "Basic" / "Components" / "Keywords" / etc.) | **Medium** | `qtformdialog.cpp`: wrap the grid in a `QTabWidget` with a "Basic" tab (Tier-1 components) and per-component-group tabs. The `EditorPropertyGrid` would need a `setComponentsForTab` method. |
| OK/Cancel button placement | Low | Already at the bottom via `QDialogButtonBox` — matches CK. |
| Field ordering inside the grid | Low | Determined by `EditorPropertyGrid` component iteration order; ensure Tier-1 components (FullName, Model, etc.) are registered first in each record's `load()` so they appear at the top. |

The structural pattern is correct; the main gap is modal-vs-modeless
and the missing tab grouping.

---

## 7. Preferences dialog — **High**

### Reference (real CK)

`docs/CK_Real_Integration_Plan.md:58`:
`Genesis\Construction Set\Misc\TESPreferences.cpp` uses a **tree-based
category sidebar** (observed by running the CK):

```
Preferences
├── Display
├── Edit
├── Sound
├── Network
├── ...
```

Each category switches the right-hand pane to a different settings
page. The real `CreationKit.ini` (read from the install dir) has
sections confirming the categories: `[General]`, `[Display]`,
`[Archive]`, `[LODManager]`, `[Wwise]`, `[RenderGraph]`, `[Debug]`,
`[Particles]`, `[LOD]`, `[Morph]`, `[Papyrus]`, `[Localization]`,
`[MMS]`.

### OpenCK current

`src/view/window/preferencesdialog.cpp:28-115` builds a **flat stack
of `QGroupBox`es** in a single `QVBoxLayout`:

- General Settings (data dir, game, language)
- Appearance (theme)
- Miscellaneous (auto-save, skip cell load)

There is no category tree, no sidebar, and no way to navigate between
top-level preference groups. All settings are on one scrolling page.

### Gaps

| Gap | Severity | Fix |
|---|---|---|
| No tree/sidebar category navigation | **High** | `preferencesdialog.cpp`: replace the `QVBoxLayout` with a `QSplitter` — left = `QTreeWidget` with categories (General, Display, Edit, Sound, Network, Archive, Papyrus, LOD, …), right = a `QStackedWidget` with one page per category. |
| Missing categories that the real CK has | **Medium** | Add pages for Display (fov, camera speed, render distance — map to `CreationKit.ini [Display]`), Edit (auto-save, undo depth, default snap settings), Sound (volume, Wwise codec — `[Wwise]`), Network (version control — defer), Archive (BA2 list — `[Archive]`), Papyrus (compiler folder — `[Papyrus]`, already partially present in the codebase via `papyruscompiler.cpp`). |
| Settings stored under a single `[OpenCK]` INI group | **Low** | `preferencesdialog.cpp:121,146` read/write only `conf.beginGroup("OpenCK")`. Split into per-category groups (`[Display]`, `[Papyrus]`, etc.) to mirror the CK's ini layout and make the file hand-editable. |

---

## 8. Status bar — **Medium**

### Reference (real CK)

The real CK's status bar (observed) shows, left to right:

- Current **cell coordinates** (e.g. `Cell: 5,-3`).
- **Object count** in the current cell.
- **Selected object info** (editor ID + form ID of the picked
  reference).
- A progress area for long operations (load, save, compile).

### OpenCK current

`src/view/window/mainwindow.cpp:114-126`:

```cpp
mStatusRecordCount = new QLabel(ui->statusBar);
mStatusRecordCount->setText("Records: 0");
ui->statusBar->addWidget(mStatusRecordCount);

mStatusPluginInfo = new QLabel(ui->statusBar);
mStatusPluginInfo->setText("No plugin loaded");
ui->statusBar->addPermanentWidget(mStatusPluginInfo);

mStatusProgressBar = new QProgressBar(ui->statusBar);
ui->statusBar->addPermanentWidget(mStatusProgressBar);
```

So OpenCK shows: total record count (transient, left), plugin name
(permanent, right), progress bar (permanent, right).

### Gaps

| Gap | Severity | Fix |
|---|---|---|
| No cell-coordinate display | **Medium** | `mainwindow.cpp`: add a `mStatusCellCoords` label, update it when the (future) Cell View selection changes. |
| No selected-object info | **Medium** | Add a `mStatusSelectedObject` label; update from the Object Window tree's `currentIndex()` change signal and (future) Render Window pick events. |
| Record count is global, not per-cell | **Low** | The real CK shows the count of references *in the current cell*, not the total plugin record count. Repurpose `mStatusRecordCount` once Cell View exists. |
| Plugin info is fine but should be left-side, not permanent-right | Low | Cosmetic — the CK puts the plugin name on the left. Move `mStatusPluginInfo` to `addWidget` instead of `addPermanentWidget` (`mainwindow.cpp:121`). |

---

## Prioritized fix list

Ordered by what unblocks the most subsequent work.

1. **[Critical]** Integrate QtAdvancedDocking (`external/ads/`) into
   `MainWindow`; replace `QDockWidget` with `ads::CDockWidget`. Affects
   `src/view/window/mainwindow.cpp`, `src/view/window/windowlayout.cpp`,
   `ui/mainwindow.ui`. (Workstream B of the integration plan.)

2. **[Critical]** Build the Cell View as a docked panel
   (`CellViewPanel`) replacing the modal `CellsDialog`. Affects
   `src/view/window/cellsdialog.{hpp,cpp}`,
   `src/view/window/mainwindow.cpp:1307-1318`.

3. **[Critical]** Restructure the Object Window model into a 3-level
   hierarchical tree (Group → Category → Record) matching the CK's
   Actors/Items/World Objects/Gameplay/Audio/Dialogue grouping. Affects
   `src/model/window/objectwindow.{hpp,cpp}`. Rename "Texture Asset" →
   "Static" at `objectwindow.cpp:260`.

4. **[Critical]** Add the 7 missing top-level menus (Character,
   ObjectWindows, RenderWindows, Navmesh, Terrain, Audio, Docks — plus
   stubs for Galaxy/Packin/Theme/Tests) and move misfiled actions into
   them. Affects `ui/mainwindow.ui`, `src/view/window/mainwindow.cpp`.

5. **[High]** Make `QtFormDialog` modeless (base `QWidget` under ADS,
   not `QDialog`) and add tab grouping for component sections. Affects
   `src/view/window/qtformdialog.{hpp,cpp}`,
   `src/view/window/qtformdialogmanager.cpp`.

6. **[High]** Rebuild `PreferencesDialog` with a tree sidebar +
   `QStackedWidget` pages mirroring the CK's category set. Affects
   `src/view/window/preferencesdialog.{hpp,cpp}`.

7. **[High]** Add Render Window transform-mode toolbar buttons
   (Select/Move/Rotate/Scale) and snap toggles as placeholders for the
   deferred edit-module work. Affects
   `src/view/window/nifviewportwidget.cpp:384-476`.

8. **[High]** Fix default dock placement: call
   `WindowLayout::applyDefaultLayout(this)` at the end of
   `MainWindow::setData()` (`mainwindow.cpp:205`) and move the Object
   Window to `LeftDockWidgetArea` at `mainwindow.cpp:168`.

9. **[Medium]** Add cell-coordinate and selected-object labels to the
   status bar. Affects `src/view/window/mainwindow.cpp:114-126`.

10. **[Medium]** Add the missing real-CK File-menu actions
    (`actionCreate_Archive`, `actionCompactSmallMaster`,
    `actionCompile_Papyrus_Scripts`, etc.) listed in
    `docs/CK_Real_Integration_Plan.md:71-82`.

11. **[Low]** Rename the saved-layout ini key to the CK-style
    `QtCreationKitSavedSettings.ini` filename for migrant familiarity
    (`src/view/window/windowlayout.cpp:83-86`).

12. **[Low]** Move `mStatusPluginInfo` from permanent to transient
    status-bar slot (`mainwindow.cpp:121`).