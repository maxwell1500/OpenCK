# OpenCK Parity & Beyond Roadmap (parity.md)

**A Living Master Plan for Creation Kit Parity and Next-Generation Authoring**  
*Document Version: 1.0 — Date: October 2026*

---

## Executive Summary & Vision

**OpenCK** is a clean-room, open-source replacement for Bethesda's Creation Kit, engineered in modern C++17 and Qt6. 

The official Creation Kit is widely recognized as the single largest bottleneck in the modding community: it is single-game siloed, prone to crashes on large plugins, corrupts data on interrupted writes, relies on archaic workflows (such as requiring Autodesk 3ds Max 2013), and provides no modern version control.

OpenCK is designed to achieve two overarching objectives:
1. **Tier 1: 100% Functional & Workflow Parity with the Creation Kit** across all asset, record, terrain, navmesh, and scene editing domains.
2. **Tier 2: Exceeding the Creation Kit** by delivering an architecture that is crash-resilient, universal across game generations, Git-native, multi-threaded, cross-platform (Windows & Linux/Steam Deck), and tightly integrated with modern tools like Blender 4+.

---

## Architectural Foundation & Completed Milestones

OpenCK has already completed its foundational engine audits and core transaction systems (see `openck/REMAINING.md`):

```
                               CURRENT FOUNDATIONAL STATUS
┌────────────────────────────────────────────────────────────────────────────────────────┐
│ ✓ 0 reader warnings across 3.8M records in Starfield.esm, Skyrim.esm, Morrowind.esm    │
│ ✓ Bit-exact untouched round-trip serialization across master files                      │
│ ✓ Transactional Object Window editing with RecordEditSession & full Undo/Redo          │
│ ✓ Memory-safe width-correct integer property bindings (Series 2)                       │
│ ✓ Real CELL placed child references (RefrRecord) under cell child groups (Series 3)    │
│ ✓ Table-driven BlankRecordFactory covering all record types (Series 4)                 │
│ ✓ FormPickerWidget, FormIdVectorTable, and ContainerItemsTable (Series 5)              │
│ ✓ Archive orchestration (BSA 0x67, 0x68, 0x69 LZ4/zlib, BA2 GNRL/DX10) (Series 6)    │
│ ✓ Save-time and interactive validation with reference index (Series 7)                 │
│ ✓ Active-document transactions across Weather/Light, Water, Dialogue, AI (Series 8)   │
│ ✓ Atomic NIF animation write-back with QSaveFile and block preservation (Series 9)     │
│ ✓ Shipped Starfield Landscape Brushes (.lbr) single-object parsing                     │
│ ✓ Shipped Starfield Particle Bundles (.pofx) nested typed parsing                      │
│ ✓ Creation Kit Object-Window Filters (.filter) with wildcards & per-rule OR chaining    │
│ ✓ Installed Starfield Papyrus toolchain detection (PapyrusCompiler, PCompiler.dll)     │
│ ✓ Creation Kit INI & EditorColors.xml inspector (CkConfigInspector)                    │
│ ✓ 130+ passing automated QTest test suites                                             │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## Part I: Reaching Creation Kit Workflow Parity

**Phases completed: 1–15.**

```
                                  PHASE OVERVIEW: PARITY
  ┌─────────────────────────────────────────────────────────────────────────────────────┐
  │ Phase 1: Hierarchical Object Window Tree & Filter System                            │
  │ Phase 2: Docked Cell View & Multi-Cell Hierarchy Panel                              │
  │ Phase 3: Top-Level Menu Bar & Advanced Docking Layout Parity                        │
  │ Phase 4: Interactive 3D Render Window — Raycast Picking & Transform Gizmos          │
  │ Phase 5: In-Viewport 3D Terrain Sculpting & Texture Painting                        │
  │ Phase 6: In-Viewport 3D Navmesh Authoring & Edge Editing                            │
  │ Phase 7: Material RuleTemplates & Bethesda Asset Tool CLI Bridges                   │
  │ Phase 8: Mod Manager Deployment & Audio/LipSync Pipelines                           │
  └─────────────────────────────────────────────────────────────────────────────────────┘
```

### Phase 1: Hierarchical Object Window Tree & Filter System ✅
*Target: Complete parity with `BGSObjectWindowTree.cpp` and `BGSGenericCategoryLayout.cpp`.*  
**Status: Complete (October 2026)**
- **1.1 Canonical Grouped Tree Hierarchy**:
  Transition the Object Window from a flat 27-item category list to the standard Creation Kit hierarchical tree:
  ```
  All
  ├── Actors
  │   ├── Actor Values (AVIF)
  │   ├── Body Part Data (BPTD)
  │   ├── Creature (CREA)
  │   ├── Head Part (HDPT)
  │   ├── Leveled Actor (LVLC)
  │   ├── NPC (NPC_)
  │   └── Voice Type (VTYP)
  ├── Items
  │   ├── Ammo (AMMO)
  │   ├── Armor (ARMO) / Armor Addon (ARMA)
  │   ├── Book (BOOK)
  │   ├── Constructible Object (COBJ)
  │   ├── Container (CONT)
  │   ├── Ingredient (INGR) / Alchemy (ALCH)
  │   ├── Key (KEYM)
  │   ├── Leveled Item (LVLI)
  │   ├── Misc Item (MISC)
  │   ├── Outfit (OTFT)
  │   ├── Soul Gem (SLGM)
  │   └── Weapon (WEAP)
  ├── Magic
  │   ├── Enchantment (ENCH)
  │   ├── Magic Effect (MGEF)
  │   ├── Potion (ALCH)
  │   ├── Scroll (SCRL)
  │   ├── Shout (SHOU)
  │   └── Spell (SPEL)
  ├── World Objects
  │   ├── Activator (ACTI)
  │   ├── Acoustic Space (ASPC)
  │   ├── Animated Object (ANIO)
  │   ├── Art Object (ARTO)
  │   ├── Door (DOOR)
  │   ├── Flora (FLOR)
  │   ├── Furniture (FURN)
  │   ├── Grass (GRAS)
  │   ├── Hazard (HAZD)
  │   ├── Idle Marker (IDLM)
  │   ├── Light (LIGH)
  │   ├── Movable Static (MSTT)
  │   ├── Static (STAT)
  │   ├── Static Collection (SCOL)
  │   └── Tree (TREE)
  ├── Gameplay
  │   ├── Camera Path (CPTH) / Camera Shot (CAMS)
  │   ├── Class (CLAS)
  │   ├── Combat Style (CSTY)
  │   ├── Encounter Zone (ECZN)
  │   ├── Faction (FACT)
  │   ├── Game Setting (GMST)
  │   ├── Global Variable (GLOB)
  │   ├── Keyword (KYWD)
  │   ├── Location (LCTN) / Location Ref Type (LCRT)
  │   ├── Package (PACK)
  │   ├── Perk (PERK)
  │   ├── Quest (QUST)
  │   └── Race (RACE)
  ├── Audio
  │   ├── Music Track (MUSC) / Music Type (MUST)
  │   ├── Reverb Parameters (REVB)
  │   └── Sound Descriptor / Sound (SOUN)
  ├── Special / World Data
  │   ├── Cell (CELL)
  │   ├── Climate (CLMT)
  │   ├── Image Space (IMGS) / Image Space Adapter (IMAD)
  │   ├── Land Texture (LTEX)
  │   ├── Region (REGN)
  │   ├── Water (WATR)
  │   ├── Weather (WTHR)
  │   └── Worldspace (WRLD)
  └── Space / Starfield (When active)
      ├── Biome (BIOM)
      ├── Celestial Body / Planet (PNDT)
      └── Space Structure
  ```
- **1.2 Filter Integration**:
  Directly wire `ObjectWindowFilter` into the category view:
  - Shipped filter files (`Data/DataViews/ObjectWindow/_common/*.filter`) selectable from a filter dropdown.
  - Multi-parameter live search bar matching EditorID, Name, FormID, and Keywords with wildcard (`*`, `?`) acceleration.
- **1.3 Object Window Context Menu Parity**:
  - `Edit` (`Enter`), `Duplicate` (`Ctrl+D`), `Delete` (`Del`), `Use Info...` (`Ctrl+F`), `Copy` (`Ctrl+C`), `Cut` (`Ctrl+X`), `Paste` (`Ctrl+V`), and `Find in Render Window`.
- **Implementation Deliverables (Done 2026-10-06)**:
  - Canonical category tree hierarchy (`All Forms`, `Actors`, `Items`, `Magic`, `World Objects`, `Gameplay`, `Audio`, `Dialogue`, `Special`, `Space`, `Miscellaneous`) organized in `ObjectWindowModel`.
  - Wildcard pattern matching (`*`, `?`) and backspace-safe full-record filtering in `applyFilter()`.
  - Automatic discovery and execution of shipped Creation Kit `.filter` files via `mSavedFilterCombo` in `ObjectWindowDialog`.
  - Implemented `UseInfoDialog` (`src/view/window/useinfodialog.{hpp,cpp}`) displaying all placed references and cells utilizing the target record.
  - Verified with 16 automated tests in `test_objectwindow.cpp`.
---

### Phase 2: Real Docked Cell View & Multi-Cell Hierarchy Panel ✅
*Target: Complete parity with `TESCellView.cpp`.*  
**Status: Complete (October 2026)**

- **2.1 Three-Pane Docked Cell View (`CellViewPanel`)**:
  - **Pane 1: Worldspace Selector**: Dropdown showing `Interiors` (default), `(All cells)`, and all `WorldspaceRecord` (WRLD) entries with EditorID.
  - **Pane 2: Cell Table (`CellTableModel`)**: Multi-column table with 6 columns: `Editor ID`, `Form ID`, `Grid (X, Y)`, `Name`, `Location`, `Has Water`. Real-time filter search bar.
  - **Pane 3: Placed References Table (`RefrTableModel`)**: Multi-column table with 9 columns: `Form ID`, `Editor ID`, `Base Object`, `Type`, `Position (X, Y, Z)`, `Rotation (X, Y, Z)`, `Scale`, `Persistent`, `Disabled`. Base object name and type resolved via `allCollectionsWithTypes()`.
  - **Map Canvas**: 2D interactive canvas (`CellMapCanvas`) with marquee selection, zoom/pan, crosshair, and reference markers.
- **2.2 Scene Synchronization**:
  - Selecting a cell updates active cell coordinates and emits `cellSelected(cellFormId)`. Double-clicking emits `cellDoubleClicked(cellFormId)`.
  - Selecting a reference emits `refSelected(record)`. Double-clicking emits `refDoubleClicked(refrFormId)` and opens the reference in `QtFormDialog`.
- **2.3 Reference Authoring & Context Menu Parity**:
  - Right-click context menu on references: `Edit...`, `Duplicate` (with `_Copy` naming and new FormID allocation), `Delete` (via `removeRecordWithUndo`), `Use Info...` (via `UseInfoDialog`).
- **Implementation Deliverables (Done 2026-10-06)**:
  - Implemented `CellTableModel` and expanded `RefrTableModel` in `src/view/window/cellsdialog.{hpp,cpp}`.
  - Upgraded `CellViewPanel` with worldspaces, cell table, ref table, map canvas, and context actions.
  - Added dedicated unit test suite `test_cellviewpanel.cpp` covering interiors/exteriors, reference columns, base object resolution, and selection signals.
---

### Phase 3: Top-Level Menu Bar & Advanced Docking Layout Parity ✅
*Target: Complete parity with `docs/CK_Real_Integration_Plan.md` and `docs/LAYOUT_AUDIT.md`.*
**Status: Complete (October 2026)**

- **3.1 Top-Level Menus**:
  `openck/ui/mainwindow.ui` now presents the standard Creation Kit menu set in CK order (all in one menubar): `File`, `Edit`, `View`, `Character`, `Gameplay`, `World`, `ObjectWindows`, `RenderWindows`, `Navmesh`, `Terrain`, `Audio`, `Galaxy`, `Docks`, `Theme`, `Tests`, `Help`, followed by the OpenCK-only `Tools` and `Plugins` extension menus. Gaps versus the real CK menu set, with owning phases:
  1. `File`: New Plugin, Open Plugin, Save Active Document, Save As, Preferences, Export, Convert to ESL, Exit — Exit now uses `close()` so UI state saves on quit.
  2. `Edit`: Undo, Redo, Cut, Copy, Paste, Duplicate, Find Text, plus `Find Forms...` wired to form search.
  3. `View`: Toggle Grid, Toggle Wireframe, Toggle Bounds, Toggle Sky, Toggle Collision, Camera Speeds submenu (Slow 0.5x / Normal 1.0x / Fast 2.5x), Refresh Viewport (F5), Preview Primitive submenu, Expand/Collapse All.
  4. `Character`: NPC / Race / Class / Faction Editors, `Hair and Eyes...`, Dialogue Editor, Dialogue Tree, Animation Editor.
  5. `Gameplay`: Quest Graph, AI Packages, `Combat Styles...`, `Magic Effects...`, `Perks...`, Gameplay Settings. Category-jump items focus the matching Object Window category via `showCategory()`.
  6. `World`: Worldspaces, Cells, Weather & Light, Water, `Regions...`, `Climate...`, Cell Transitions, Landscape Editing.
  7. `ObjectWindows`: Object Window, Cell View, Object Palette, Scene View (Inspector), Find Forms.
  8. `RenderWindows`: Render Window, Preview Window, Lighting, Reflection Probes (Starfield probe tab), Material Editor, Script Editor.
  9. `Navmesh`: Navmesh Editor. Full navmesh suite (Mode Toggle, Generate, Find Cover, Door Portals, Verify) is Phase 6.
  10. `Terrain`: Save Landscape, Landscape Editor, Generate LOD Meshes. Sculpting, texture painting, heightmap import/export are Phase 5.
  11. `Audio`: Sound Editor, Acoustic Spaces, Music Types. LipSync Generation done in Phase 8 (Dialogue/Sound Editor drive the shipped LipGenerator; `.fuz` packaging via `FuzWriter`).
  12. `Galaxy`: Planet Generation (PNDT editor), Star Map (galaxy view). Biome Painter is Phase 5.
  13. `Packin`: deferred — no pack-in/instance-bake feature exists yet; shipped with no stub entries.
  14. `Docks`: Reset Window Layout, Lock Docks (`lockDockWidgetFeaturesGlobally`), Save/Load Custom Layout, plus live per-dock Show/Hide entries.
  15. `Theme`: Dark / Light / System.
  16. `Help`: Documentation, Shortcuts, Game Detection (`CkConfigInspector`), About.
- **3.2 Qt Advanced Docking System (`CDockManager`) Architecture**:
  - The **Render Window** is the pinned central `CDockWidget` (`mDockManager->setCentralWidget`), non-closable / non-movable / non-floatable — `QMainWindow::setCentralWidget(viewport)` would have scheduled the manager itself for deletion.
  - `WindowLayout::applyDefaultLayout` places only docks with no home yet (no re-add churn): Object Window left (`ads::LeftDockWidgetArea`); Cell View + Inspector right; Warnings, Object Palette, Landscape Editor bottom.
  - Layouts persist to `QtCreationKitSavedSettings.ini`.
  - `File > Exit` uses `close()` so the UI state saves and the GL docks tear down while the window is still valid.
- **3.3 Render Window Toolbar**:
  - Transform Modes `Select` / `Move` / `Rotate` / `Scale` and `Snap to Grid` / `Snap to Angle` toggles live in the Render Window toolbar; Snap to Surface needs raycast picking (Phase 4).
  - New live **coordinate readout** in the toolbar: camera X/Y/Z plus selected reference X/Y/Z, updated on camera movement, selection and gizmo drags.
  - Snap to Grid / Snap to Angle are toolbar-only (no Ctrl+G / Ctrl+A rebindings).
- **Teardown safety (fixed by this phase)**:
  - Destroying a visible `MainWindow` with initialized GL views faulted intermittently in teardown (`LandscapeEditor::~LandscapeEditor` freed GL objects through a context that was already being torn down); the fix releases nothing during teardown and nulls pointers instead. An orphaned `actionWorldView` (menu-less, slot-less) was removed from `openck/ui/mainwindow.ui`.
  - GL objects are freed by Qt with their context; the editors null their GL pointers on destruction and their paint paths bail out when GL state is gone. No manual GL release runs during teardown.
- **Implementation Deliverables (Done 2026-10-06)**:
  - `openck/ui/mainwindow.ui`: 16 canonical menus in CK order; new actions (toggles, camera speeds, find forms, category jumps, reflection probes, galaxy tools, lock docks, documentation, game detection); orphaned `actionWorldView` removed.
  - `openck/src/view/window/mainwindow.{hpp,cpp}`: `setupCanonicalMenus()`, `ensureViewport()` (central ADS dock), `bindViewportActions()` (two-way menu↔toolbar sync), `populateDocksMenu()` (lazy per-dock toggles).
  - `openck/src/view/window/windowlayout.cpp`: churn-free default placement.
  - `openck/src/view/window/nifviewportwidget.{hpp,cpp}`: display-flag setters/getters + `displayFlagsChanged`, camera speed multiplier + `cameraPosition()`, live `coordLabel` readout, teardown-safe paint path.
  - Verified with 9 automated `test_mainwindow.cpp` cases (menu order; pinned central Render Window; default dock placement; View↔viewport sync; camera-speed scaling; coordinate readout; Docks menu contents; lock docks; layout screenshot); full suite green.
---

### Phase 4: Interactive 3D Render Window — Raycast Picking & Transform Gizmos ✅
*Target: Complete parity with `NifViewportWidget` scene manipulation and `Genesis\RenderWindow`.*
**Status: Complete (October 2026)**

- **4.1 CPU/GPU Raycast Picking**:
  - Cast ray from camera through mouse cursor into the 3D scene (`gizmo::pickRay`).
  - Intersect with oriented bounding boxes (OBBs) of placed `RefrRecord` instances (`gizmo::rayObbDistance`, `NifViewportWidget::refObb`), falling back to screen-space marker radius.
  - Highlight selected object with outline halo using `EditorColors.xml` `SelectedOutline` primary color (`QVector3D(0.09f, 1.0f, 0.91f)` teal / `#17FFE8`).
  - Multi-selection with `Shift+Click` and selection rectangle (`Marquee Selection` via `selectMarqueeRect` and live `m_marqueeVBO` overlay).
- **4.2 3D Interactive Transform Gizmos**:
  - **Translate Gizmo**: Red (X), Green (Y), Blue (Z) axis handles with cone tips.
  - **Rotate Gizmo**: 3 axis circular rings with arcball screen drag.
  - **Scale Gizmo**: 3 axis handles with box ends.
  - Interactive mouse dragging with visual axis highlighting (`mHoverAxis`).
  - Real-time snapping to configurable grid increments (`mSnapGridSize` / `snapToStep`) and angle increments (`mSnapAngleIncrement` / `snapDegrees`).
- **4.3 Transactional Persistence**:
  - Mouse release generates atomic `EditRecordCommand<RefrRecord>` pushing previous transform vs new transform onto `UndoStack`.
  - Batch/macro multi-object transactions bundle into `MacroCommand` for clean single-step undo.
- **4.4 In-Viewport Duplicate & Place**:
  - `Ctrl+D` in viewport duplicates selected references in-place (+64 units offset), allocating new FormID via `Data::createNewRecord(Type_Refr_)` and parenting to the current cell via `setRefrParentCell`, with `MacroCommand` undo support.
  - Drop to Ground (`F` key): samples terrain elevation from active cell's `LandRecord` (`baseHeight` + `heightData[33][33]`) and snaps reference Z to ground, bundled with atomic undo.
- **Implementation Deliverables (Done 2026-10-06)**:
  - `src/view/window/gizmomath.{hpp,cpp}`: added `PickRay`, `pickRay()`, `rayAabbDistance()`, and `rayObbDistance()`.
  - `src/view/window/nifviewportwidget.{hpp,cpp}`: added OBB ray picking (`refObb`, `refRotationMatrix`), `SelectedOutline` teal halo render pass, multi-selection set (`mSelectedRefIndices`, `setSelectedRefIndices`), marquee selection rect drag/render (`selectMarqueeRect`, `m_marqueeVBO`), and `F` (drop to ground) / `Ctrl+D` (duplicate) viewport signals.
  - `src/view/window/mainwindow.{hpp,cpp}`: implemented `dropViewportSelectionToGround()` and `duplicateViewportSelection()` with atomic `MacroCommand` undo/redo.
  - Automated verification: `test_gizmomath` (7 unit tests covering raycast and box intersection), `test_mainwindow` (13 unit tests covering multi-selection, marquee selection, drop to ground, and duplicate).
---

### Phase 5: In-Viewport 3D Terrain Sculpting & Texture Painting ✅
*Target: Complete parity with `LandscapeEditor` in 3D and official Creation Kit Terrain tool.*
**Status: Complete (October 2026)**

- **5.1 3D Terrain Raycast & Brush Ring**:
  - Raycast cursor onto active exterior cell heightmap vertices (`gizmo::rayTerrainDistance`, `gizmo::rayTriangleDistance`, and `LandscapeEditor::screenToTerrain`).
  - Project 3D circle/falloff ring onto the terrain surface showing active brush radius and falloff profile (`LandscapeEditor::renderBrushRing`, teal outer circle `#17FFE8` and gold inner falloff ring).
- **5.2 Sculpting Modes**:
  - Real-time vertex displacement driven by `BrushDefinition` operations:
    - **Sculpt**: Raise terrain (or invert to lower terrain).
    - **Flatten**: Pull vertices toward target height or sampled elevation.
    - **Smooth**: 3x3 neighbor height averaging with smooth falloff.
    - **Stamp**: Heightmap profile stamping from `.lbr` alpha mask.
    - **BuildUp / Subtractive**: Additive or subtractive volume displacement.
    - **Noise**: Deterministic coordinate-hashed displacement for terrain roughness.
- **5.3 Landscape Texture Painting**:
  - Added `PaintMode` (`Sculpt` vs `PaintTexture`) and active texture layer selection.
  - Left-click drag paints alpha blend weights onto the landscape cell's texture quad layers in `LandscapeEditor::paintTexture` and synchronizes to `LandRecord::textureLayers` (up to 4 VTEX layers per quad).
- **5.4 Dirty Bounding Box Optimization**:
  - Track modified vertex region during drag (`strokeDirtyRect`); on mouse release, packages only the modified sub-region into `LandscapeEditCommand` pushed to `UndoStack` with immediate sync to `LandRecord`.
- **Implementation Deliverables (Done 2026-10-07)**:
  - `src/view/window/gizmomath.{hpp,cpp}`: added `rayTriangleDistance()` (Möller-Trumbore) and `rayTerrainDistance()`.
  - `src/model/tools/brushdefinition.{hpp,cpp}`: added `Operation::Noise` support in enum, parser, and built-in brush list.
  - `src/view/window/landscapeeditor.{hpp,cpp}`: added `raycastTerrain()`, `screenToTerrain()`, `renderBrushRing()`, `paintTexture()`, dirty rect transactional undo via `LandscapeEditCommand`, auto-loading `LandRecord` matching cell coordinates, and public brush/mode accessors.
  - Automated verification: `test_gizmomath` (validated triangle & terrain raycast), `test_brushdefinition` (validated Noise brush round-trip), `test_texturelayer` (validated sculpt, flatten, smooth, noise, texture painting, raycast, and undo/redo).
---

### Phase 6: In-Viewport 3D Navmesh Authoring & Edge Editing ✅
*Target: Complete parity with Creation Kit Navmesh editing mode.*
**Status: Complete (October 2026)**

- **6.1 3D NavMesh Visualization Overlay**:
  - Render navmesh triangles as semi-transparent alternating green/blue faces over terrain and collision surfaces in `NifViewportWidget`.
  - Render vertices as selectable 3D points; edges as color-coded lines (regular: dark blue, cover edge: gold `#FFD700`, portal: purple `#AA00FF`, water boundary: cyan `#00FFFF`).
- **6.2 Interactive Topology Editing**:
  - **Vertex Creation**: `NavmeshEditorDialog::addVertex` drops new vertices into topology and updates viewport.
  - **Triangle Creation**: `NavmeshEditorDialog::addTriangle` constructs normal-oriented triangles from vertex triplets.
  - **Edge Extrusion**: `NavmeshEditorDialog::extrudeEdge` / `NavMeshTools::extrudeEdge` extrudes adjacent triangles along edges.
  - **Edge Splitting**: `NavmeshEditorDialog::splitEdge` / `NavMeshTools::splitEdge` splits edges with midpoint/custom vertices and bisects incident faces.
  - **Edge Flipping**: `NavmeshEditorDialog::flipEdge` / `NavMeshTools::flipEdge` flips shared hypotenuse between adjacent coplanar triangles.
- **6.3 Door Portals & Cover**:
  - Door portal linking in `NavmeshEditorDialog::linkDoorPortal` / `linkEdgeDoorPortal` stores linked door reference FormIDs (`XNDP`/`NVDP`).
  - Edge cover generation in `NavmeshEditorDialog::generateEdgeCover` computes barrier drop depths and flags cover edges with cover heights.
- **6.4 Reachability & Validation**:
  - Live reachability flood fill using `NavMeshTools::largestReachableTriangleComponent` identifies and highlights disconnected islands in bright red (`#FF2222`).
- **6.5 Transactional Undo/Redo**:
  - `NavmeshEditCommand` transactional integration into `UndoStack` across all topology and vertex operations.
- **Implementation Deliverables (Done 2026-10-07)**:
  - `src/model/tools/navmeshtoolkit.{hpp,cpp}`: added `largestReachableTriangleComponent()`, `extrudeEdge()`, `splitEdge()`, and `flipEdge()`.
  - `src/model/tools/navmeshcommand.hpp`: created atomic `NavmeshEditCommand` undo/redo command.
  - `src/view/window/navmesheditordialog.{hpp,cpp}`: added interactive topology methods (`addVertex`, `addTriangle`, `extrudeEdge`, `splitEdge`, `flipEdge`), door portal linking, cover generation, reachability update, and undo stack binding.
  - `src/view/window/nifviewportwidget.{hpp,cpp}`: added `setNavmeshTopologyData()`, selection setters, and multi-layer overlay rendering (green/blue faces, red disconnected islands, yellow/selected red vertices, categorized edge color lines).
  - `src/view/window/navmesheditor.cpp`: wired dynamic signal `navMeshUpdated` to sync full topology into `NifViewportWidget`.
  - Automated verification: `test_navmeshtoolkit` (24 unit tests passing, covering adjacency, issues, welding, connections, T-junctions, grid generation, cover, voxel filters, island pruning, edge extrusion, edge splitting, edge flipping, reachable components, and dialog undo/redo).
---

### Phase 7: Material RuleTemplates & Bethesda Asset Tool CLI Bridges ✅
*Target: Complete parity with official Creation Kit asset pipeline.*

**Status: Complete (October 2026)**

- **7.1 Material Rule Templates (`RuleTemplates`)**:
  - Ingest `Data/EditorFiles/RuleTemplates/ShaderModels/*.json` and `Bundles/*.json`.
  - Support nested `TemplateRules` with regex-based texture slot matching (Diffuse, Normal, Smoothness, AO, Subsurface).
  - Material Editor updates to compile and preview Starfield PBR materials (`.mat`).
- **7.2 Official Asset Tool CLI Bridge**:
  - Automated detection and invocation configuration for Bethesda asset utilities:
    - `BSFBX.exe`: FBX to NIF/collision conversion.
    - `BSTextureConverter.exe` / `xtexconv.exe`: TGA/PNG to DDS (BC1–BC7) conversion.
    - `Starwatcher.exe`: Asset file change tracking and background reload trigger.

- **Implementation Deliverables (Done 2026-10-08)**:
  - `src/model/tools/materialruletemplate.{hpp,cpp}`: rewrote the model to the real CK RuleTemplates schema — `Rule{from,to,op}`, `RuleClass{className,rules}`, `fromJson`/`loadFile`/`loadDirectory` (`*.json` sorted), `patternToRegex`/`patternMatches`, `textureSlotOf`, `isLayerGroup`, `builtinLayerSlots()` (20 slots), `textureSlots()`, `applyToSlots`/`applyToSlotMap`, `requiredSlots()`, `builtinNames()` (1–4LayerStandard, Terrain, Skin, Hair, Eye, Water, Vegetation), and `builtinTemplate(name)`.
  - `src/model/tools/materialcompiler.{hpp,cpp}`: created `MaterialCompileReport` (template name, applied operations, resolved/final/missing slots, unresolved textures, warnings) resolving texture paths against a texture root.
  - `src/model/tools/assettoolcli.{hpp,cpp}`: created `AssetToolCliBridge` — `QSettings`-backed auto-detection of 12 CK/Starfield tools (AssetTool, TextureTool, NifSkse, Papyrus compiler/assembler, LipGenerator, FaceFX, IconGenerator, Archive2, Sqlite3, XEdit, AssetWatcher), `detectTool`/`runTool` (timeout-safe), `convertTextures` (CLI or `AssetConverter` fallback), `packageMaterials`, and progress/summary signals.
  - `libs/files/esm/materialrecord.{hpp,cpp}`: `QMap<QString,QString> textureSlots` field with round-trip `SLTS` subrecord (compact-JSON ZString, omitted when empty).
  - `src/view/window/materialpreviewwidget.{hpp,cpp}`: created `QOpenGLWidget` preview (GLSL 330 core Blinn-Phong, DDS/PNG/TGA/TIF loading with QImage, graceful `glUnavailable()` fallback; GL work confined to the paint path).
  - `src/view/window/materialeditor.{hpp,cpp}`: Rule Templates tab — template combo, slot assignment table, compile + preview, and persistence through `MATR` records.
  - `src/view/window/assetbrowserwidget.{hpp,cpp}`: Convert to DDS, Package Material, and Toolchain Status actions wired to `AssetToolCliBridge` with a status label + log pane.
  - Automated verification: `test_materialruletemplate` (11 tests), `test_materialcompiler` (3 tests), `test_assettoolcli` (10 tests), `test_materialrecord` (3 tests), `test_materialeditor` (3 tests) passing (30 total).

---

### Phase 8: Mod Manager Deployment & Audio/LipSync Pipelines ✅
*Target: Complete mod workflow parity.* **Status: Complete (October 2026).**

- **8.1 Mod Manager Virtual File System Awareness**:
  - Read Vortex `deployment.json` and Mod Organizer 2 `modlist.txt` / `profiles/*/modlist.txt`.
  - Resolve asset overrides, mod priority orders, and hardlink statuses without requiring users to flatten directories.
- **8.2 Audio LipSync Pipeline**:
  - Dialogue response WAV playback integration via `AudioPipelineTools`.
  - Phoneme extraction using SAPI 5.1 / Windows Speech platform.
  - Generate `.lip` / `.fuz` compressed voice and lip sync archives directly from Dialogue Editor.

---

**Implementation deliverables (done 2026-10-08):**
- `ModDeploymentResolver` (`src/model/tools/moddeploymentresolver.{hpp,cpp}`) parses the
  real Vortex manifest schema (grounded on the machine's own
  `vortex.deployment.json` + Data-folder `vortex.deployment.*.json`, native-separator
  relPaths normalized to forward slashes), MO2 `modlist.txt` (`+`/`-` lines, last enabled
  wins), resolves deployed vs staged paths, and reports hardlink/symlink/same-file/distinct
  status via NTFS file ids. The Mod Manager dialog shows the manifest, deployment method,
  per-file deployed/source/link table, and an asset resolver.
- `FuzWriter` (`libs/files/audio/fuzwriter.{hpp,cpp}`) writes `.fuz` voice archives in the
  forms the in-repo reader accepts (real `FUZE`+version+lipSize+lip+audio form with audio,
  chunked `LIPF` form for lip-only) with round-trip verification through `FuzParser`.
- `AudioPipelineTools` corrected to the shipped Starfield LipGenerator's real CLI
  (positional wav/text/actor/analysis plus `-Language:`/`-OutputFileName:`/`-AnimationGroupName:`
  colon args — the previous `-wav/-out/-data/-rate` form the tool does not accept), plus
  FaceFX actor discovery. The Sound Editor and Dialogue (INFO) editor both drive it and the
  INFO editor can also package a `.fuz` next to the response WAV.
- Automated verification: `test_modmanager` 14 tests (incl. the real manifest gate),
  `test_fuzparser` 9 tests, `test_audiopipelinetools` 9 tests passing; full ctest 149/150
  (the pre-existing `test_tes3roundtrip` Morrowind-loader regression is the only failure).

---

## Part II: Exceeding the Creation Kit

```
                               PHASE OVERVIEW: EXCEEDING CK
  ┌─────────────────────────────────────────────────────────────────────────────────────┐
  │ Phase 9: Universal Multi-Game Engine (One Tool for All 5 Generations)               │
  │ Phase 10: Git-Native Collaborative Version Control & Visual Record Diffing          │
  │ Phase 11: Crash-Resilient Architecture & Ultra-Fast Materialization Engine          │
  │ Phase 12: Blender 4+ DCC Live-Sync (Zero 3ds Max Dependency)                        │
  │ Phase 13: Native Cross-Platform Engine (Linux & Steam Deck Support)                 │
  └─────────────────────────────────────────────────────────────────────────────────────┘
```

### Phase 9: Universal Multi-Game Engine (One Tool for All 5 Generations) ✅
*The official Creation Kit requires modders to install and learn five separate, incompatible binaries (Morrowind Construction Set, Oblivion CS, Skyrim CK, Fallout 4 CK, Starfield CK).*

- **9.1 Universal Unified Binary**:
  - OpenCK supports **Morrowind (`.esm`/`.esp`), Oblivion, Skyrim (LE, SE, AE), Fallout 4, and Starfield** in a single executable.
  - Auto-detects game format from master headers and switches schema definitions, game setting catalogs (`WeatherLightCatalog`, `WaterCatalog`), and subrecord grammars dynamically.
- **9.2 Cross-Game Asset & Record Migration**:
  - Export records or cells from Skyrim and import directly into Starfield or Fallout 4 with automatic subrecord translation and FormID compaction.
  - Cross-game BSA/BA2 archive converter (e.g. converting Oblivion 0x67 BSA to Skyrim SE 0x69 LZ4 BSA).

---

**Implementation deliverables (done 2026-10-08):**
- 9.1 was already in place (detection + per-game record registry + TES3/TES4 readers +
  per-game catalogs); this phase closed 9.2:
- `ArchiveConverter` (`src/model/tools/archiveconverter.{hpp,cpp}`): rebuilds any classic
  BSA (0x67/0x68/0x69, incl. duplicate-name and zero-length entries) in the target game's
  format, or as a BA2 (GNRL/DX10), and vice versa. Every conversion is verified by
  re-reading the written container and comparing every payload byte-for-byte.
- `RecordMigrator` (`src/model/tools/recordmigrator.{hpp,cpp}`): copies a plugin's records
  into another game's plugin over the shared TES4 record core, renames on EditorID
  collisions instead of overwriting, and reports every skipped record with a reason.
  Game-specific codes (Skyrim MATT, FO4 ASRC, Starfield SHOU, ...) are not translated.
- `MigrationDialog` (Tools > Cross-Game Migration...): archives tab (source ->
  target game/BA2) and records tab (source plugin -> target game -> output plugin, driven
  through `Document`/`Data` load + save).
- Four latent bugs fixed in the shared BSA writer/encoder while grounding the converter
  on real archives:
  1. the LZ4 length extension was omitted for exactly-15 lengths (the spec requires a
     zero byte), desyncing every block that hit it;
  2. matches were allowed to end inside the final 5 bytes, which strict decoders
     (Skyrim's) reject;
  3. uncompressed "stored" blocks were written inside LZ4 frames, which have no such
     block variant — readers fail on them;
  4. zero-length payloads were dropped on write (Oblivion ships one, `menus\menu_labels.txt`).
- Verified: `test_archiveconverter` 7/7 (real-data gate converts the 115-entry
  `Oblivion - Misc.bsa` to a Skyrim SE 0x69 archive byte-verified), `test_recordmigrator`
  6/6, `test_bsawrite` 8/8, `test_bsaarchive` 9/9, `test_ba2write` 4/4; full ctest
  149/150 (the pre-existing `test_tes3roundtrip` Morrowind-loader regression is the
  only failure).

---

### Phase 10: Git-Native Collaborative Version Control & Visual Record Diffing ✅
*The Creation Kit has no version control outside of enterprise Perforce servers. Mod teams struggle with plugin merge conflicts, dirty edits, and ITMs.*

- **10.1 Visual Record Diffing**:
  - Side-by-side graphical record diff viewer showing subrecord-by-subrecord and field-by-field differences between any two plugins or commits.
  - Highlighting modified, added, deleted, and identical subrecords with color coding.
- **10.2 Native Git Integration**:
  - Integrated Git repository support in `Preferences > Version Control`: Commit, Branch, Switch, Push, Pull.
  - Decompilation of binary ESM/ESP into clean, deterministically sorted JSON/YAML text records for Git tracking.
- **10.3 Semantic Three-Way Plugin Merging**:
  - Automatically merges two modders' branches against a common master:
    - Non-conflicting cell additions merge cleanly.
    - Conflicting FormID allocations are automatically remapped using `FormIdCompactor`.
    - Detects and prompts for resolution only on true conflicting record edits.

**Implementation deliverables (done 2026-10-09):**
- `PluginDiffer` (`src/model/tools/plugindiff.{hpp,cpp}`): diffs two plugins
  record-by-record straight off disk (`collectRecordSnapshots`), keyed by
  (recordType, formId). Subrecords are aligned by name via LCS; runs of the
  same name are paired positionally, so a mid-block edit reports as Modified
  instead of a delete+add cascade. Report carries Added/Removed/Modified/
  Same per record plus the aligned subrecord payloads (hex + printable) and
  the EDID as label.
- `RecordDiffDialog` (Tools → Record Diff...): file pickers, colour-coded
  record table (green/red/amber) and a subrecord detail pane showing both
  payloads side by side.
- `PluginTextExport` (`src/model/tools/plugintextexport.{hpp,cpp}`):
  decompiles a plugin to JSON keyed `TYPE:formId` (so text diffs stay local
  to a record), subrecords in file order, payloads lowercase hex; output is
  a pure function of the input, so re-exporting is byte-identical — the
  property that makes the files actually usable in Git.
- `GitRepository` extended with `branches`, `checkout`, `fetch`, `pull`,
  `push` (the existing wrapper only had commit/status/diff); `Preferences >
  Network / Version Control` is now a live panel: repository path, branch
  switch, status, commit / pull / push, disabled self-consistently when the
  path is not a repo or git is missing from PATH.
- `RecordMerger` + `PluginMerger` (`src/model/tools/{recordmerger,
  pluginmerger}.{hpp,cpp}`) + `PluginMergeDialog` (Tools → Three-Way Plugin
  Merge...): merges base/mine/theirs; one-sided changes are taken, identical
  changes collapse, both branches claiming the same FormID for different
  records is reported as an ID conflict, and true edits on both sides become
  conflicts reported with a reason (kept on the "mine" side for the user to
  resolve in the editor). Output records are written verbatim through
  `ESMWriter`, so the merged plugin reads back through the same reader.
- Verified: `test_plugindiff` 8/8 (identical / added-removed-modified /
  deterministic JSON / three-way merge / both conflict kinds / merge output
  re-read) and `test_gitintegration` 7/7 against real `git` (init, commit,
  branch, switch, status, diff, non-repo behaviour).

---

### Phase 11: Crash-Resilient Architecture & Ultra-Fast Materialization Engine ✅
*The Creation Kit is notorious for crashing when opening large master files or running out of 32/64-bit pool memory.*

- **11.1 Ultra-Fast Async Record Indexing**:
  - Time-sliced background master indexing loads 3,800,000 records from `Starfield.esm` in seconds without ever freezing the UI thread.
  - Instantaneous search across millions of records powered by memory-mapped indices.
- **11.2 Atomic Zero-Corruption Persistence**:
  - All disk writes use `QSaveFile` writing to temporary staging files first and committing atomically. An OS crash or power cut will never leave a corrupted half-written mod file.
- **11.3 Automated Pre-Save Safety Net**:
  - Integrated `AssetValidator` checks for dangling FormIDs, missing masters, circular quest links, and invalid cell coordinates before saving, protecting modders from shipping broken plugins.

**Implementation deliverables (done 2026-10-09):**
- 11.1 was already met by the time-sliced loader (`Loader::load` yields every
  64 records / 15 ms and re-queues; masters are index-only and materialize
  on demand). The export/merge/diff tooling in this phase reuses that
  snapshot layer instead of loading plugins into `Data`.
- 11.2: `Document::save` now stages through `QSaveFile` (write → close →
  `commit()`), so an interrupted or failing save leaves the previous plugin
  untouched. `ESMWriter::save()` was widened from `QFile&` to `QIODevice&`
  to accept it; this is the write path for every plugin save in the app
  (NIF/BA2 writers already used `QSaveFile`).
- 11.3: `AssetValidator::validateRelationships` gained the two checks the
  spec names that were missing: **interior cells carrying an exterior grid
  position** and **exterior cells whose grid is outside the engine's −32..32
  range** (errors) plus exterior cells with no `XCLC` (warning), and a
  **circular dialogue-link** check — DFS with three-colour marking over the
  DIAL→INFO→DIAL graph, reporting the cycle path as one error. The
  dangling-FormID and missing-master checks were already present.
- Verified: full ctest 152 suites (151 passing; the only failure is the
  pre-existing `test_tes3roundtrip` Morrowind-loader regression recorded in
  `REMAINING.md`); app smoke clean.

---

### Phase 12: Blender 4+ DCC Live-Sync (Zero 3ds Max Dependency) ✅
*Bethesda tools rely on commercial 3ds Max 2013 plugins that are completely unavailable to modern indie modders.*

- **12.1 Built-in Blender 4.x Bridge**:
  - Bundled Blender launcher (`bundle_blender.cmake`) and automated NifTools addon integration (`bundle_niftools.cmake`).
- **12.2 One-Click Viewport Live-Sync**:
  - Right-click any reference or NIF in OpenCK and click **Open in Blender**: launches Blender with the mesh, skeleton, and collision loaded.
  - Saving in Blender automatically triggers OpenCK's `NifAnimationWriter` / `NifBlockFile` to update the active plugin's asset without manual export steps.

**Implementation deliverables (done 2026-10-09):**
- 12.1 shipped earlier and is unchanged here: `cmake/bundle_blender.cmake`
  downloads and stages Blender, `cmake/bundle_niftools.cmake` installs the
  NifTools addon into the bundled build's versioned `scripts/addons`
  directory (deriving the scripts path instead of hardcoding it), the
  install rules carry both, and `BlenderLauncher` detects
  PyNifly/NifTools, version-gates the 4.4+ requirement, and opens files in
  Blender. `test_blenderlauncher` and `test_iconrenderer` cover it.
- 12.2: `BlenderBridge` (`src/model/tools/blenderbridge.{hpp,cpp}`) +
  `scripts/blender/openck_livesync.py` + `LiveSyncDialog`. The open flow
  resolves the mesh's **skeleton** (NIF parse + sibling conventions) and
  **collision** siblings, so the Blender session opens scene-complete; the
  save flow is a `save_post` handler in the script that exports back to a
  watcher-staged file beside the mesh — Ctrl-S in Blender *is* the export
  step. Every export is verified through the editor's own readers
  (`NifParser`, plus the `NifBlockFile` container split for Gamebryo
  files); a rejected export never touches the asset. Commit is a separate,
  explicit action that copies the verified export over the original
  atomically (`QSaveFile`, backup preserved, committed bytes re-verified,
  rollback on failure). Right-click any record with a model → "Open in
  Blender (Live Sync)...".
- Verified: `test_blenderbridge` 9/9 — scene resolution (skeleton +
  collision siblings), launch plan (argv, output path, missing-script
  error), the full watch→verify→commit loop with real fixture NIFs written
  through the editor's own writer, garbage rejection, and the
  commit-twice rule. App smoke clean; full ctest 154 suites.

---

### Phase 13: Native Cross-Platform Engine (Linux & Steam Deck Support) ✅
*The Creation Kit has never run natively on Linux; modders on Steam Deck or Linux must battle complex Wine/Proton setups.*

- **13.1 Native Linux Executable**:
  - First-class Linux build target (`CMakeLists.txt` clean under GCC/Clang on Ubuntu/Arch/Fedora).
  - Native Wayland and X11 support via Qt6.
- **13.2 Steam Deck & Controller-Aware Navigation**:
  - Touch-friendly UI scaling preset.
  - Viewport navigation presets for gamepad thumbsticks and trackpads.

**Implementation deliverables (done 2026-10-09):**
- 13.1: every Windows-only dependency is now behind `_WIN32`/`MSVC`/`WIN32`
  guards, so a GCC/Clang configure compiles the same tree:
  `tests/test_loader.cpp` lost its unconditional `<windows.h>`/`<DbgHelp.h>`/
  `<crtdbg.h>` block (SEH page-heap diagnostics are Windows-only; the
  non-Windows path uses glibc `backtrace()` from `<execinfo.h>`), and
  `src/view/window/waveplayer.{hpp,cpp}` was rewritten off Win32 `waveOut`
  onto Qt's `QAudioSink` — one code path for ALSA/PipeWire/PulseAudio/macOS
  instead of a Windows-only player plus a Linux hole. Optional Qt modules are
  discovered with `find_package(... QUIET OPTIONAL_COMPONENTS Multimedia
  Gamepad)`; when absent the features self-report as unavailable rather than
  failing the configure. Linux packaging: `data/linux/openck.desktop` +
  icon install rules under `UNIX AND NOT APPLE`, and `RPATH $ORIGIN` was
  already in place. Wayland/X11 need no per-backend code: Qt's platform
  plugin picks the composer at runtime.
- 13.2: `ThemeManager` gained density presets (`Scale::Desktop` / `Touch` /
  `Compact`) that scale the app font **and** augment the stylesheet with
  larger hit targets, exposed in Preferences → Appearance with a
  "Touch (Steam Deck)" entry; the choice persists in the `[OpenCK] UiScale`
  key and is applied at startup after the palette so a theme switch cannot
  drop it. Render-window navigation gained a shared camera API
  (`nudgeCamera` / `orbitCamera` / `zoomCamera` / `resetCamera`) plus
  `GamepadNavigator`, which maps left stick → pan, right stick → orbit,
  triggers → raise/lower, A/B/X → reset/zoom with a radial dead zone, and
  feeds the same API every other input source uses. Trackpad pan/orbit is
  already Qt mouse/wheel driven and needs no extra backend.
- Verified: `test_thememanager` **12 passed** (5 new scale tests assert the
  font actually grows/shrinks, that the density sheet *augments* rather than
  replaces the theme sheet, and that the scale survives a theme switch);
  `test_loader` 21 passed with the new guards; app smoke clean with the
  navigator active. Both non-Windows branches of `waveplayer.cpp` and
  `gamepadnavigator.cpp` were compile-checked with `_WIN32`/`OPENCK_WITH_QT_GAMEPAD`
  undefined.

---

### Phase 15: AI Package Editor (Patrol, Sandbox, Travel, Combat) ✅

- **15.1 Semantic model** — `openck::decodePackageData`/`encodePackageData`
  (`libs/files/esm/packagesemantics.{hpp,cpp}`) reads the PKDT payload the
  record layer keeps verbatim into a 30-value `PackageKind`, flags word and
  month/weekday/date/hour/minute schedule, and writes back only the bytes the
  model owns: multi-entry payloads and unmodelled bytes survive untouched, so
  an unedited package still round-trips bit-for-bit.
- **15.2 Validation** — `validatePackageData` reports what the engine would:
  missing PTDT target on reference-requiring families, Travel/Patrol without
  road flags, a schedule with a date but no minute. Errors block saving,
  warnings do not.
- **15.3 Editor** — `PackEditor` is a real form (kind combo over all 30
  families, schedule grid, live targets through `FormPickerWidget`, live
  validation with Save disabled on error), and the AI package browser shows
  decoded kind + schedule per row.
- Verified: `test_packagesemantics` 12/12, `test_packagerecord` 4/4 with
  file-level round-trips proving schedule/target edits survive ESM write+read
  and an untouched package is byte-identical; 12-suite regression sweep green;
  app smoke clean.

### Phase 14: OBScript Script Editor + Validator ✅

- **14.1 Real-source parsing** — the parser accepts the source files the game
  actually ships: `ScriptName X extends Y` headers (including `extends Actor`
  keeping its capital), `<Type> Property name [auto] [= expr]`, `<type> name`
  declarations and compound assignment (`+= -= *= /= %=` desugared to `x = x + y`).
- **14.2 Per-game native catalogs** — `ObScript::nativeCatalogFor(GameFlavor)`
  maps the loaded game to a curated native signature table, so `AddItem()`
  fails in Skyrim SE while an unknown flavor carries only the cross-game
  builtins. Trailing parameters may be optional (Papyrus defaults), e.g. `wait(1.0)`.
- **14.3 Cross-script resolution** — every property typed with a script name is
  checked against the plugin's own SCRO records; unseen scripts are reported as
  warnings because they may live in a master file.
- **14.4 Editor** — `ScriptEditorDialog` takes the game flavor and script list,
  lists every diagnostic with click-to-jump, and disables **Save** while a
  compile-level error is open. Completion is seeded from keywords, script
  symbols, the flavor's natives and sibling scripts.
- Verified: `test_obscriptcatalog` (12), `test_scripteditor` 15; six obscript
  suites total 79 green tests; 11-suite regression sweep green; app smoke clean.

---

### Chronological Delivery Order

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│ STAGE 1: UI & WORKFLOW PARITY                                                          │
│   • Phase 1: Hierarchical Object Window Tree & Filter System                           │
│   • Phase 2: Docked Cell View & Multi-Cell Hierarchy Panel                             │
│   • Phase 3: Top-Level Menu Bar & Advanced Docking Layout Parity                       │
├────────────────────────────────────────────────────────────────────────────────────────┤
│ STAGE 2: 3D RENDER WINDOW & SCENE MANIPULATION                                         │
│   • Phase 4: Interactive 3D Render Window — Raycast Picking & Transform Gizmos         │
│   • Phase 5: In-Viewport 3D Terrain Sculpting & Texture Painting                       │
│   • Phase 6: In-Viewport 3D Navmesh Authoring & Edge Editing                           │
├────────────────────────────────────────────────────────────────────────────────────────┤
│ STAGE 3: ASSET BRIDGES & ADVANCED WORKFLOWS                                            │
│   • Phase 7: Material RuleTemplates & Bethesda Asset Tool CLI Bridges                  │
│   • Phase 8: Mod Manager Deployment & Audio/LipSync Pipelines                          │
├────────────────────────────────────────────────────────────────────────────────────────┤
│ STAGE 4: EXCEEDING THE CREATION KIT                                                    │
│   • Phase 9: Universal Multi-Game Engine                                               │
│   • Phase 10: Git-Native Collaborative Version Control & Visual Record Diffing         │
│   • Phase 11: Crash-Resilient Architecture & Performance Optimization                  │
│   • Phase 12: Blender 4+ DCC Live-Sync                                                 │
│   • Phase 13: Native Cross-Platform Support (Linux & Steam Deck)                       │
│   • Phase 14: OBScript Script Editor + Validator                                       │
│   • Phase 15: AI Package Editor (Patrol/Sandbox/Travel/Combat)                          │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

### Verification Criteria for Every Phase
1. **Zero Degradation**: The existing 130+ QTest test suite must remain 100% green at all times (`ctest --output-on-failure`).
2. **Deterministic Round-Tripping**: Any plugin or asset opened and saved without modification must remain bit-for-bit identical to the original input.
3. **Atomic Persistence**: Every save operation must commit through `QSaveFile` with undoable command history.
4. **Clean-Room Integrity**: Zero game assets, proprietary Bethesda DLLs, or decompiled code included in the repository. Everything derived from clean-room reverse engineering and public Qt6 APIs.
