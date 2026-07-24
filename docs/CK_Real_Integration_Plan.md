# OpenCK — Real Creation Kit Integration Plan

This document tracks what the actual **Starfield Creation Kit** (`CreationKit.exe`
v1.16.244.0, shipped free with Starfield via Steam at
`C:\Program Files (x86)\Steam\steamapps\common\Starfield\`) contains under the
hood, so we can drive OpenCK's UI and editing architecture toward the same
shape — clean-room — without copying any of Bethesda's source code or assets.

## How we know this

- Bethesda's `CreationKit.exe` is a **Qt5** application (we use Qt6).
  Shipped Qt5 DLLs next to it: `Qt5Core.dll`, `Qt5Gui.dll`, `Qt5Widgets.dll`,
  `Qt5Network.dll`, `Qt5OpenGL.dll`, `Qt5Sql.dll`, `Qt5Svg.dll`.
- It links the open-source **QtAdvancedDocking** (ADS) library — a `CDockManager`
  on top of Qt. The DLL is `QtAdvancedDocking.dll` in the install dir. This
  library is MIT-licensed; we vendor it under `external/ads/`.
- The binary embeds full PDB strings in its `E:\BuildAgent\work\fee57674ddcb42c9\`
  build path. From those, **395 source file paths** were recovered. The path
  `Genesis\Construction Set\` is Bethesda's internal root for the CK's
  codebase. We do **not** copy these paths into our repo — they are referenced
  here only as a map of *what the real CK contains*, so we know what to mimic.
- We do not ship any of Bethesda's source. Where a file is listed below, the
  intent is to implement our own equivalent under our own path
  (`openck/libs/components/`, `openck/src/view/window/`, etc.).

## Legal line (important — keep repeating this in PRs)

Allowed:
- Reimplementing observable behavior of a publicly-distributed free tool.
- Using MIT-licensed libraries Bethesda themselves ship (QtAdvancedDocking).
- Documenting the real CK's general architecture for our own orientation.
- Implementing component-property design patterns (a generic pattern in many
  editors — Unity, Unreal, Godot, Visual Studio designers, Qt Designer).

Not allowed:
- Copying string literals verbatim from `CreationKit.exe` (error messages,
  dialog text, etc. — copyrighted expression).
- Reproducing Bethesda's exact source tree structure (`Genesis\Construction
  Set\...`) in our code. Our tree uses our own conventions.
- Calling our product "Creation Kit" — that's Bethesda's trademark. "OpenCK"
  is fine as long as we don't imply endorsement.
- Shipping any Bethesda asset: textures, fonts, icons, sounds, the
  `Starfield.esm` master, etc.

## The real CK's source layout (reference only)

The 395 paths are grouped below by subsystem, with the OpenCK side they map to
on the right. Use this as a reading list when you implement the corresponding
OpenCK feature.

### Core editor — the main window and shell

| Real CK path | What it does | OpenCK equivalent |
|---|---|---|
| `Genesis\Construction Set\Dialogs\CreationKitMainWindow.cpp` | Main app window, menus, menu wiring, toolbar | `src/view/window/mainwindow.cpp` (rewritten) |
| `Genesis\Construction Set\Misc\BGSWindowLayout.cpp` | Save/restore dock layout to `QtCreationKitSavedSettings.ini` | `src/view/window/windowlayout.hpp` (new) |
| `Genesis\Construction Set\Qt\Utility\CreationKitNativeEventFilter.cpp` | Native Windows event filter (so Qt events don't get eaten) | (low priority — defer) |
| `Genesis\Construction Set\Misc\TESPreferences.cpp` | Preferences dialog | `src/view/window/preferencesdialog.cpp` (already exists, refactor) |
| `Genesis\Construction Set\Misc\TESDialog.cpp` | Base dialog class | n/a |
| `Genesis\Construction Set\misc\BGSAdminTasks.cpp` | Background admin (auto-save, auto-validate) | (defer) |
| `Genesis\Construction Set\misc\BGSAutomatedProcesses.cpp` | Automated processes | (defer) |

### Top-level menus (524 actions recovered)

The CK's menus are: **File, Edit, View, Character, Gameplay, World,
ObjectWindows, RenderWindows, Navmesh, Terrain, Audio, Galaxy, Packin, Docks,
Theme, Tests, Help.** All these names are general-purpose English — no copying
concerns. Use the same English menu labels so users transitioning have zero
mental overhead.

Key menu actions to wire:
- `actionOpen_Material_Editor`, `actionOpen_Particle_Editor`,
  `actionBlockPatternEditor` — separate editors, defer
- `actionCell_View_Window` — show Cell View
- `actionGalaxy_View` — show Galaxy View
- `actionNavmesh_*` — NavMesh tools, defer
- `actionCheckIn`, `actionCheckOut` — version control, defer
- `actionCompile_Papyrus_Scripts` — script compile, has stub in OpenCK
- `actionCreate_Archive` — BA2 archive creation, has tool in `Tools/Archive2/`
- `actionCompactSmallMaster`, `actionCompactMediumMaster` — plugin compaction
- `actionConvertLargeMaster` etc. — master format conversion
- `actionClean_NavMesh_Splines`, `actionConnectNavmeshes_All_Interiors` — defer

### Object Window (the most-used panel)

| Real CK path | What it does | OpenCK equivalent |
|---|---|---|
| `Genesis\Construction Set\misc\BGSObjectWindow.cpp` | The Object Window panel (left side) | `src/view/window/objectwindowdialog.cpp` (refactor to use new tree) |
| `Genesis\Construction Set\misc\BGSObjectWindowTree.cpp` | Object Window tree model | new tree model |
| `Genesis\Construction Set\misc\BGSPaletteObject.cpp` | The Object Palette (bottom strip) | `src/view/window/objectpalette.cpp` (works) |
| `Genesis\Construction Set\Dialogs\DialogObjectPalette.cpp` | Object Palette dialog wrapper | wraps `objectpalette.cpp` |
| `Genesis\Construction Set\Dialogs\DialogObjectPaletteTreeView.cpp` | Object Palette tree view | n/a (we use list) |
| `Genesis\Construction Set\Qt\GenericTreeView\QtGenericTreeView.cpp` | Generic tree view base | n/a |
| `Genesis\Construction Set\Qt\GenericTreeView\QtGenericTreeModel.cpp` | Generic tree model | new model in `src/model/` |
| `Genesis\Construction Set\misc\BGSGenericDataView.cpp` | Generic data view (filter/search) | n/a |
| `Genesis\Construction Set\misc\BGSGenericCategoryLayout.cpp` | Object Window category layout | n/a |

### Cell View (the 2D top-down cell browser)

| Real CK path | OpenCK equivalent |
|---|---|
| `Genesis\Construction Set\Misc\TESCellView.cpp` | `src/view/window/cellsdialog.cpp` (refactor) |

### Render Window (the 3D viewport)

These all together form the Render Window — multi-week effort, **deferred**.

| Real CK path | OpenCK equivalent |
|---|---|
| `Genesis\Construction Set\misc\BGSRenderWindow.cpp` | (defer) |
| `Genesis\Construction Set\misc\BGSRenderWindowEditModule.cpp` | (defer) |
| `Genesis\Construction Set\misc\BGSRenderWindowEditModuleManager.cpp` | (defer) |
| `Genesis\Construction Set\misc\BGSRenderWindowGizmo.cpp` | (defer) |
| `Genesis\Construction Set\misc\BGSRenderWindowReferenceEditModule.cpp` | (defer) |
| `Genesis\Construction Set\misc\BGSRenderWindowNavmeshEditModule.cpp` | (defer) |
| `Genesis\Construction Set\misc\BGSRenderWindowNavMeshSplineEditModule.cpp` | (defer) |
| `Genesis\Construction Set\misc\BGSRenderWindowPrimitiveGizmo.cpp` | (defer) |
| `Genesis\Construction Set\misc\BGSRenderWindowVolumeGizmo.cpp` | (defer) |
| `Genesis\Construction Set\misc\BGSRenderWindowLightEditModule.cpp` | (defer) |
| `Genesis\Construction Set\misc\BGSRenderWindowUtils.cpp` | (defer) |
| `Genesis\Construction Set\misc\BGSRenderWindowLandscapeCutModule.cpp` | (defer) |
| `Genesis\Construction Set\misc\RenderWindowManager.cpp` | (defer) |
| `Genesis\Construction Set\misc\BGSPreviewWindow.cpp` | (defer) |
| `Genesis\Construction Set\Misc\BGSPreviewCore.cpp` | (defer) |
| `Genesis\Construction Set\Dialogs\Widgets\PreviewWidget.cpp` | (defer) |

### Property editor (the core of editing)

| Real CK path | What it does | OpenCK equivalent |
|---|---|---|
| `Genesis\Construction Set\Editor Properties\Inspector\BGSInspectorDialog.cpp` | Inspector (Properties) window | `src/view/window/qtformdialog.cpp` (new) |
| `Genesis\Construction Set\Editor Properties\Inspector\BGSEditorPropertyGrid.cpp` | The property grid itself | `src/view/widgets/editorpropertygrid.cpp` (new) |
| `Genesis\Construction Set\Editor Properties\Properties\BGSArrayEditorProperty.cpp` | Array property | `libs/components/editorproperty.hpp` (new) |
| `Genesis\Construction Set\Editor Properties\Properties\BGSFormComponentArrayEditorProperty.cpp` | Array-of-components property | same file |
| `Genesis\Construction Set\Editor Properties\Properties\BGSSoundHookEditorProperty.cpp` | Sound hook property | (defer — Starfield-only) |
| `Genesis\Construction Set\Qt\FormEditing\Editor Properties\QtEditorPropertiesWidget.cpp` | Properties widget base | `libs/components/formcomponentwidget.hpp` (new) |
| `Genesis\Construction Set\Qt\FormEditing\Editor Properties\QtTESFormWidget.cpp` | Per-form widget base | same file |
| `Genesis\Construction Set\Qt\FormEditing\Editor Properties\QtTESFormDialog.cpp` | The generic form dialog | `src/view/window/qtformdialog.cpp` (new) |
| `Genesis\Construction Set\Qt\FormEditing\Editor Properties\QtFormDataWidget.cpp` | Per-form data widget | (defer) |
| `Genesis\Construction Set\Qt\FormEditing\Editor Properties\FormListPropertyWidget.cpp` | List property widget | (defer) |
| `Genesis\Construction Set\Qt\FormEditing\Editor Properties\FormPickerWithTypeWidget.cpp` | Form picker (dropdown) | `libs/components/editorproperty.hpp` `FormEditorProperty` |
| `Genesis\Construction Set\Qt\FormEditing\QtCreationKitFormDialog.cpp` | `QtCreationKitFormDialog` (the new 2020s-era editor) | `src/view/window/qtformdialog.cpp` (new) |
| `Genesis\Construction Set\Qt\FormEditing\QtCreationKitFormDialogManager.cpp` | Manages open dialogs | `src/view/window/qtformdialog.cpp` (same) |
| `Genesis\Construction Set\Qt\FormEditing\FormListWidget.cpp` | List widget | (defer) |
| `Genesis\Construction Set\Qt\FormEditing\PromptDuplicateFormDialog.cpp` | Prompt when duplicating | (defer) |
| `Genesis\Construction Set\misc\PropertyGrid.cpp` | Property grid helper | (defer) |
| `Genesis\Construction Set\misc\BGSReferencePropertiesDialog.cpp` | Reference (placed-object) properties | (defer) |
| `Genesis\Construction Set\misc\BGSReferenceBatchTool.cpp` | Batch-edit references | (defer) |
| `Genesis\Construction Set\misc\BGSReferenceCollection.cpp` | Multi-select ref editing | (defer) |

### FormComponents (the pieces that compose forms)

| Real CK path | What it does | OpenCK equivalent |
|---|---|---|
| `Genesis\Construction Set\Qt\FormEditing\FormComponents\TESActorBaseDataWidget.cpp` | NPC/creature actor base data | (defer — Tier 3) |
| `Genesis\Construction Set\Qt\FormEditing\FormComponents\TESSpellListWidget.cpp` | Spell list editor | (defer — Tier 3) |
| `Genesis\Construction Set\Qt\FormEditing\FormComponents\QtContainerTableModel.cpp` | Container items table | (defer — Tier 2 needs it) |
| `Genesis\Construction Set\Qt\FormEditing\FormComponents\BGSDestructibleObjectFormWidget.cpp` | Destructible object | (defer — Tier 3) |
| `Genesis\Construction Set\Qt\FormEditing\FormComponents\BGSFormLinkDataWidget.cpp` | FormLinkData (Starfield) | (defer) |
| `Genesis\Construction Set\Qt\FormEditing\FormComponents\BGSModelMaterialSwapWidget.cpp` | Model material swap | (defer) |
| `Genesis\Construction Set\Qt\FormEditing\FormComponents\BGSModTemplateItemsWidget.cpp` | Mod template items | (defer) |
| `Genesis\Construction Set\Qt\FormEditing\FormComponents\BGSOverridePackCollectionWidget.cpp` | Override pack | (defer) |
| `Genesis\Construction Set\Qt\FormEditing\FormComponents\IGroupedFormTableComponentWidget.h` | Grouped table base | (defer) |

### TESForms (the 127 record-type editors)

The CK has **127 `_Editor.cpp` files**, one per record type. We replace
these with our generic `QtFormDialog` for all Tier 1+2 record types. The
remaining custom-dialog cases (CELL, QUST, SCEN, etc.) we keep as bespoke
dialogs that *use* the generic property grid for the basic component section
and add custom widgets for their special parts.

For reference, the 127 are:

```
ActorValue, AlchemyItem, BGSAcousticSpace, BGSAddonNode, BGSAffinityEvent,
BGSArtObject, BGSAssociationType, BGSBaseAlias, BGSBendableSpline, BGSBiome,
BGSBodyPart, BGSBodyPartData, BGSBoneModifier, BGSCameraPathManager,
BGSCameraShot, BGSCollisionLayer, BGSColorForm, BGSConditionForm,
BGSConstructibleObject, BGSCraftingResourceOwner, BGSDamageType, BGSDebris,
BGSDefaultObject, BGSEntryPointFunction, BGSEquipSlot, BGSFootstep,
BGSFootstepSet, BGSGameplayOption, BGSGameplayOptionGroup,
BGSGenericBaseForm, BGSGenericBaseFormTemplate, BGSHazard, BGSHeadPart,
BGSIdleMarker, BGSImpactData, BGSInstanceNamingRules, BGSKeyword, BGSLensFlare,
BGSLensFlareSprite, BGSLevPackIn, BGSLightingTemplate, BGSListForm, BGSLocAlias,
BGSMaterialType, BGSMessage, BGSMod_Object, BGSMovableStatic, BGSMovementType,
BGSMusicPaletteTrack, BGSMusicSilenceTrack, BGSMusicSingleTrack, BGSMusicTrack,
BGSMusicTrackFormWrapper, BGSMusicType, BGSNote, BGSObjectSwap, BGSOutfit,
BGSPackIn, BGSPerk, BGSProjectile, BGSRefAlias, BGSRelationship, BGSResource,
BGSReverbParameters, BGSScene, BGSSceneActionCamera, BGSSceneActionMove,
BGSSceneActionTimeline, BGSSceneCollection, BGSShaderParticleGeometryData,
BGSSnapTemplate, BGSSnapTemplateNode, BGSSoundEcho, BGSSoundEchoDatum,
BGSSoundKeywordMapping, BGSSoundTagSet, BGSSpeechChallengeObject,
BGSStaticCollection, BGSTalkingActivator, BGSTerminalMenu, BGSTextureSet,
BGSTransform, BGSVoiceType, BGSWwiseKeywordMapping, CreatureSounds, EffectItem,
EffectSetting, EnchantmentItem, MagicItem, SpellItem, TESAmmo, TESClimate,
TESCombatStyle, TESEffectShader, TESFaction, TESFurniture, TESGameSettings,
TESGlobal, TESIdleManager, TESImageSpace, TESImageSpaceModifier, TESLandTexture,
TESLight, TESLoadScreen, TESNPC, TESObjectACTI, TESObjectARMO, TESObjectBOOK,
TESObjectCell, TESObjectMISC, TESObjectREFR, TESObjectSTAT, TESObjectWEAP,
TESQuest, TESQuestReward, TESQuestStage, TESQuestTarget, TESRace, TESResponse,
TESScript, TESSound, TESTopic, TESTopicInfo, TESWaterForm, TESWeather,
TESWeatherList, TESWorldSpace
```

Of these, OpenCK currently has flat per-record structs in `libs/files/esm/`
matching only a subset. We migrate these in this order, applying Tier 1+2
components to all of them at once:
**STAT, MISC, ARMO, WEAP, BOOK, ALCH, CONT, ACTI** (8 record types,
covering ~80% of common editing).

### Other subsystems (deferred but listed for completeness)

- **Pathfinding/NavMesh**: `Genesis\Construction Set\Pathfinding\*` — defer
- **Spaceship editor**: `Genesis\Construction Set\Interface\SpaceshipBuilderStates\*`,
  `Genesis\Construction Set\Dialogs\DialogSpaceship.cpp`,
  `Genesis\Construction Set\misc\BGSSpaceshipEditorWindow.cpp`,
  `Genesis\Construction Set\misc\BGSSpaceshipInteriorEditModule.cpp` — defer
- **Galaxy view**: `Genesis\Construction Set\Dialogs\GalaxyView.cpp`,
  `Genesis\Construction Set\Dialogs\GalaxyCreateOverlayDialog.cpp`,
  `Genesis\Construction Set\Qt\GalaxyView\*` — defer
- **Particle editor**: `Genesis\Construction Set\Dialogs\Particle Editor\*` — defer
- **Material editor**: `Genesis\Construction Set\Dialogs\MaterialSwapEditor\*`,
  `Genesis\Construction Set\misc\MaterialEditor.cpp` (in our code) — defer
- **Block pattern editor**: `Genesis\Construction Set\Qt\GalaxyView\BlockPatternEditor\*` — defer
- **Reflection probes**: `Genesis\Construction Set\Dialogs\ReflectionProbes\*` — defer
- **Workshop**: `Genesis\Construction Set\Dialogs\DialogWorkshop.cpp` — defer
- **Power grid**: `Genesis\Construction Set\Dialogs\DialogPowerGrid.cpp` — defer
- **Crowd region**: `Genesis\Construction Set\Dialogs\DialogCrowdRegion.cpp` — defer
- **Volumes**: `Genesis\Construction Set\Dialogs\DialogVolumes.cpp` — defer
- **Houdini**: `Genesis\Construction Set\Houdini\*` — defer
- **Effect sequence / timeline**: `Genesis\Construction Set\Dialogs\EffectSequence\*`,
  `Genesis\Construction Set\Qt\Utility\QtConditionTableModel.cpp` — defer
- **LOD generation**: `Genesis\Construction Set\LOD\*`,
  `Genesis\Construction Set\misc\BGSSimplygon.cpp` — defer
- **Validation**: `Genesis\Construction Set\misc\Validation\*` — defer
- **Mod management (BSMasterManagement)**: `Genesis\Construction Set\misc\BSMasterManagement.cpp` — defer
- **Lip synch**: `Genesis\Construction Set\Misc\LipSynchManager.cpp` — defer
- **Scripting (Papyrus)**: `Genesis\Construction Set\Script\*` — stub exists in OpenCK (`papyruscompiler.cpp`), defer
- **Morph editor**: `Genesis\Construction Set\misc\BGSMorphEditorDialog.cpp` — defer
- **Find forms / search-replace**: `Genesis\Construction Set\misc\BGSFindFormsDialog.cpp`,
  `Genesis\Construction Set\misc\BGSSearchReplaceDialog.cpp` — defer
- **Archive browser**: `Genesis\Construction Set\misc\BGSArchiveBrowser.cpp` — defer
- **Mod utils**: `Genesis\Construction Set\Misc\BGSModUtils.cpp` — defer
- **Version control**: `Genesis\Construction Set\Misc\TESVersionControl.cpp`,
  `Genesis\Construction Set\Misc\TESUser.cpp` — defer
- **Snippets (CSV/TSV import-export)**: `Genesis\Construction Set\misc\BGSSnippet.cpp` — defer
- **Scene**: `Genesis\Construction Set\misc\BGSSceneView.cpp`,
  `Genesis\Construction Set\misc\BGSSceneInfoWindow.cpp` — defer
- **Animation**: `Genesis\Construction Set\misc\Animation\HavokAnimationPreview.cpp` — defer
- **Asset browser**: `Genesis\Construction Set\Dialogs\Widgets\AssetHierarchy*\` — defer
- **Asset tag service**: `Genesis\Construction Set\Qt\AssetTag\QtAssetTagShared.cpp`,
  `Genesis\Construction Set\Services\*` — defer
- **Tooltips for forms / use report**: `Genesis\Construction Set\Misc\TESUseReport.cpp` — defer
- **Loot preview**: `Genesis\Construction Set\misc\BGSGlobalLootPreview.cpp` — defer
- **File details / bit array**: `Genesis\Construction Set\Misc\TESFileDetails.cpp`,
  `Genesis\Construction Set\misc\TESBitArrayFile.cpp` — defer
- **Directional ambient lighting colors**: `Genesis\Construction Set\misc\BGSDirectionalAmbientLightingColors_Editor.cpp` — defer
- **Light picker**: `Genesis\Construction Set\Qt\GenericTableView\LightPicker\*` — defer
- **Gameplay option tables**: `Genesis\Construction Set\Qt\GenericTableView\QtGameplayOptionTableModel.cpp` — defer
- **Form table**: `Genesis\Construction Set\Qt\GenericTableView\QtFormTableModel.cpp` — defer
- **Spanning table header**: `Genesis\Construction Set\Qt\Utility\QtSpanningTableHeaderView.cpp` — defer
- **Snap template**: `Genesis\Construction Set\Qt\Utility\QtSnapTemplateTree*` — defer
- **Validator**: `Genesis\Construction Set\Qt\Utility\BGSValidators.cpp` — defer
- **Layout constraint**: `Genesis\Construction Set\Qt\Layout\QtConstraintLayout.cpp` — defer
- **Perk**: `Genesis\Construction Set\Qt\Perk\*` — defer
- **Sound utils**: `Genesis\Construction Set\misc\SoundDialogUtils.cpp`,
  `Genesis\Construction Set\misc\BGSDialogueExport.cpp` — defer
- **Theme**: real CK has QSS resources like `:/StyleSheets/DefaultQtAdvancedDocking.css` — we use our own
- **Resource loader / app resources**: `Genesis\Construction Set\misc\AppResourceLoaders.cpp` — defer
- **Exterior cell mover**: `Genesis\Construction Set\misc\TESExteriorMover.cpp` — defer
- **Creation package / detail specs**: `Genesis\Construction Set\misc\TESCreation\*` — defer
- **Control utilities**: `Genesis\Construction Set\Misc\ControlUtilities.cpp` — defer
- **Audiokinetic capture**: `Genesis\Construction Set\Misc\TESAudioCapture.cpp` — defer
- **Condition form**: `Genesis\Construction Set\Dialogs\FindFormsByCondition\DialogFindFormsByCondition.cpp`,
  `Genesis\Construction Set\Qt\Utility\ConditionExport.cpp`,
  `Genesis\Construction Set\Qt\Conditions\QtParameterUtils.cpp` — defer
- **Form history**: `Genesis\Construction Set\Dialogs\FormHistoryDialog.cpp` — defer
- **Folder keywords**: `Genesis\Construction Set\Dialogs\FolderKeywordsDialog\*` — defer
- **Layer manager**: `Genesis\Construction Set\Dialogs\Layers\*`,
  `Genesis\Construction Set\misc\BGSLayerDialog.cpp` — defer
- **Form list dialog**: `Genesis\Construction Set\Dialogs\Forms\BGSListFormDialog.cpp` — defer
- **Affinity event / ambience / biome / curve / face / gameplay / resource / snap / transform / wwise dialogs**:
  14 specialized `BGS*FormDialog.cpp` files under `Dialogs\Forms\` — defer
- **Generic item delegates / types**: `Genesis\Construction Set\Qt\GenericItemEditors\ItemDelegates\BGSTypesItemDelegate.cpp` — defer
- **Image space / mod / weather widget**: `Genesis\Construction Set\Dialogs\Widgets\ImageSpaceWidget.cpp`,
  `Genesis\Construction Set\Dialogs\Widgets\ScriptFragmentWidget.cpp`,
  `Genesis\Construction Set\Dialogs\Widgets\WeatherListEditor.cpp` — defer
- **Furniture / message severity / block height**: `Genesis\Construction Set\Dialogs\Widgets\FurnitureActiveMarkersWidget.cpp`,
  `Genesis\Construction Set\Dialogs\Widgets\MessageSeverityTableWidget.cpp`,
  `Genesis\Construction Set\misc\Landscape\BGSRenderWindowLandscapeCutModule.cpp` — defer
- **Form list / form picker / list-form widgets** (already covered above)
- **Color picker**: `Genesis\Construction Set\Qt\ColorPicker\*` — defer
- **Curve / float curve / keyframe widgets**: `Genesis\Construction Set\Dialogs\Widgets\Curve\*` — defer
- **Item delegates / types**: covered above
- **DialogFindFormsByCondition**: covered above
- **Action system / hotkeys**: `Genesis\Construction Set\misc\BGSRenderHotKeys.cpp` — defer
- **Volume utilities**: `Genesis\Construction Set\misc\BGSVolumeUtils.cpp` — defer
- **Reflection probe (dialog)**: `Genesis\Construction Set\Dialogs\ReflectionProbes\ReflectionProbeDialog.cpp` — defer
- **Edit module scroll**: `Genesis\Construction Set\misc\EditModuleScrollOperations.cpp` — defer
- **Render setup**: `Genesis\Construction Set\misc\CKRenderSetup.cpp` — defer
- **Dialog show-hide**: `Genesis\Construction Set\Dialogs\DialogShowHideWindow.cpp` — defer
- **Dialog warnings**: `Genesis\Construction Set\Dialogs\DialogWarnings.cpp` — has stub in OpenCK
- **Color transform (BATCH prefix)**: ~30 string hits `BatchPosition3dTexTGCxform` etc. — these are the BATCH property variants the CK supports. Defer.

### Asset data files (do NOT ship these; reference only for parsing)

In `C:\Program Files (x86)\Steam\steamapps\common\Starfield\Data\`:

| Path | What it is | Our use |
|---|---|---|
| `CreationKit - Shaders.ba2` | Pre-baked CK-specific shader BA2 | n/a (we don't render CK-specific things) |
| `Source\TGATextures\Terrain\OverlayMasks\*.tif` | Terrain overlay source TIFs for landscape editing | n/a (defer landscape) |
| `EditorFiles\Bundles\*.pofx` | Particle effect templates | (defer) |
| `EditorFiles\RuleTemplates\Bundles\*.json` | Particle rule templates | (defer) |
| `EditorFiles\RuleTemplates\ShaderModels\*.json` | Shader model templates | (defer) |
| `EditorFiles\Particles\*.pex` | Particle effect scripts | (defer) |
| `EditorFiles\Primitives\*.nif` | Primitive mesh templates | (defer) |
| `DataViews\ObjectWindow\_common\*.filter` | JSON Object Window filter definitions — we use these to inspire our own | read for behavior only |
| `Snippets\*.txt` | CSV/TSV import-export templates | (defer) |
| `LandscapeBrushes\*.lbr` | Landscape brush definitions | (defer) |
| `OPAL\*.opl` | Object palette layout files | (defer) |
| `lex\*.tlx`, `*.clx` | Syntax highlighting lexers for the script editor | (defer) |
| `styles\*.qss` | Qt stylesheets (dark/light themes) | reference only — we write our own |
| `Tools\Papyrus Compiler\*` | Papyrus compiler | has its own exe; OpenCK wraps it |

## The 90+ BGS*_Component / TES*_Component classes (the reusable parts)

These are the *components* that compose any form. Implementing them is the
real work of the refactor.

### Tier 1 — universal (every form uses these)

| Component | What it does | OpenCK location |
|---|---|---|
| `TESFullName_Component` | `fullName: String` | `libs/components/tesfullname.hpp` |
| `TESModel_Component` | `modelPath: String`, `lodModelPath: String` | `libs/components/tesmodel.hpp` |
| `TESTexture_Component` | `iconPath: String`, `smallIconPath: String` | `libs/components/testexture.hpp` |
| `TESHealth_Component` | `health: Int` | `libs/components/teshealth.hpp` |
| `TESValue_Component` | `value: Int` | `libs/components/tesvalue.hpp` |
| `TESWeight_Component` | `weight: Float` | `libs/components/tesweight.hpp` |
| `TESDescription_Component` | `description: String` (multiline) | `libs/components/tesdescription.hpp` |
| `TESContainer_Component` | `items: FormComponentArray<TypedFormValuePair>` | `libs/components/tescontainer.hpp` |
| `BGSKeywordForm_Component` | `keywords: FormArray` (every record type in Starfield) | `libs/components/bgskeywordform.hpp` |

### Tier 2 — equipment

| Component | What it does | OpenCK location |
|---|---|---|
| `TESBipedModel_Component` | Biped slots + male/female models | `libs/components/tesbipedmodel.hpp` |
| `TESEnchantableForm_Component` | Enchantment form + max charges | `libs/components/tesenchantableform.hpp` |
| `BGSInstanceNamingRulesForm_Component` | Starfield instance naming rules | `libs/components/bgsinstancenamingrulesform.hpp` |
| `BGSPickupPutdownSounds_Component` | Pickup/drop sound forms | `libs/components/bgspickupputdownsounds.hpp` |

### Tier 3 — actor / world (defer)

- `TESActorBaseData_Component` — NPC/creature base data (faction, race, class)
- `TESAIForm_Component` — AI data
- `TESAttackDamageForm_Component` — damage data
- `TESSpellList_Component` — spell list
- `BGSAnimationGraph_Component` — animation graph
- `BGSBodyPartInfo_Component` — body parts
- `BGSPropertySheet_Component` — base form property sheet
- `BGSPapyrusScripts_Component` — attached Papyrus scripts
- `BGSMod_Template_Component` — mod template
- `BGSDestructibleObjectForm_Component` — destructible object
- `BGSFormLinkData_Component` — form link data
- `BGSModelMaterialSwap_Component` — model material swap
- `BGSSoundTag_Component` — sound tag
- `BGSPerkRankArray_Component` — perk ranks
- `BGSIdleCollection_Component` — idle animations
- `BGSLinkedVoiceType_Component` — voice type
- `BGSSkinForm_Component` — skin
- `BGSQualityUpgrade_Component` — quality upgrade
- `BGSPreviewTransform_Component` — preview transform
- `BGSMenuDisplayObject_Component` — menu display object
- `BGSOrbitalDataComponent_Component` — orbital data
- `BGSPlanetContentManagerContentProperties_Component` — planet content props
- `BGSPathingData_Component` — pathing data
- `BGSPrimitive_Component` — primitive
- `BGSObjectPaletteDefaults_Component` — palette defaults
- `BGSObjectWindowFilter_Component` — window filter
- `BGSLodOwner_Component` — LOD owner
- `BGSLodRuntimeOwner_Component` — LOD runtime
- `BGSModFilterSource_Component` — mod filter source
- `BGSCityMapsUsage_Component` — city maps usage
- `BGSBlockEditorMetaData_Component` — block meta
- `BGSBlockCellHeighGrid_Component` — block cell height grid
- `BGSBlockBashData_Component` — block bash data
- `BGSAdaptiveTriggerData_Component` — adaptive trigger
- `BGSAddToInventoryOnDestroy_Component` — add-to-inventory on destroy
- `BGSAttachParentArray_Component` — attach parent array
- `BGSAttackData_Component` — attack data
- `BGSBipedObject_Component` — biped object
- `BGSCraftingResourceOwners_Component` — crafting resource owners
- `BGSCraftingUseSound_Component` — crafting use sound
- `BGSCrowdComponent_Component` — crowd component
- `BGSDefaultLayer_Component` — default layer
- `BGSDestructibleObject_Component` — destructible object
- `BGSFeaturedItemMessage_Component` — featured item message
- `BGSForcedLocRefType_Component` — forced loc ref type
- `BGSInstanceNamingRulesForm_Component` — (in Tier 2)
- `BGSJSONFileForm_Component` — JSON file form
- `BGSLodOwner_Component` — (above)
- `BGSMod_Template_Component` — (above)
- `BGSNativeTerminalForm_Component` — native terminal form
- `BGSOrbitedDataComponent_Component` — orbited data
- `BGSOverlayDesignatedPlacementInfo_Component` — overlay placement
- `BGSOverridePackCollection_Component` — override pack collection
- `BGSSnapTemplateComponent_Component` — snap template
- `BGSSoundTag_Component` — (above)
- `BGSSpaceshipAIActor_Component` — spaceship AI actor
- `BGSSpaceshipEquipment_Component` — spaceship equipment
- `BGSSpaceshipHullCode_Component` — spaceship hull code
- `BGSSpaceshipWeaponBindings_Component` — spaceship weapon bindings
- `BGSStarDataComponent_Component` — star data
- `BGSStoredTraversals_Component` — stored traversals
- `BGSWorldSpaceOverlay_Component` — world space overlay
- `BlockHeightAdjustment_Component` — block height adj
- `Blueprint_Component` — blueprint
- `BlueprintVariant_Component` — blueprint variant
- `HoudiniData_Component` — Houdini data
- `ParticleSystem_Component` — particle system
- `ReflectionProbes_Component` — reflection probe
- `SurfaceTreePatternSwapInfo_Component` — surface pattern swap
- `UniqueOverlayList_Component` — unique overlay list
- `UniquePatternPlacementInfo_Component` — unique pattern placement
- `Volumes_Component` — volumes
- `WaterHeight_Component` — water height
- `BGSSpacePhysicsFormComponent` — space physics
- `BGSVehicleConfigFormComponent` — vehicle config
- `BGSVolumeFormComponent` — volume
- `LensFlareAttachmentComponent` — lens flare
- `LightAttachmentFormComponent` — light attachment
- `ObjectAttachmentFormComponent` — object attachment
- `ParticleFormComponent` — particle
- `BGSMaterialPropertyComponent` — material property
- `BGSPropertySheet` — base form property sheet (also in Tier 3)

### EditorProperty leaf types (14 of them)

These are the leaf-level editors that components expose:

- `BGSBoolEditorProperty`
- `BGSStringEditorProperty`
- `BGSFormEditorProperty` (form-ID picker)
- `BGSArrayEditorProperty`
- `BGSFormArrayEditorProperty`
- `BGSFormComponentArrayEditorProperty`
- `BGSBasePoint2EditorProperty`
- `BGSBasePoint3EditorProperty`
- `BGSBitfieldEditorProperty`
- `BGSEnumWithImageEditorProperty`
- `BGSMinMaxEditorProperty`
- `BGSStringEditorProperty`
- `BGSTemplateEditorProperty`
- `BGTypedFormValuePairEditorProperty`
- `QtEditorProperty` (abstract base)
- `BGSSoundHookEditorProperty`

Plus implicit ones the CK uses (visible in binary strings):
- `BGSInt8/16/32/64EditorPropertyTestValue` (different integer sizes)
- `BGSFloatEditorPropertyTestValue`
- `BGSColorEditorPropertyTestValue`
- `BGSDoubleEditorPropertyTestValue`
- `BGSReflectionFormComponent` (reflection-driven)

For our Tier 1+2 work we need: `Bool`, `Int`, `Float`, `String`, `Form`,
`FormArray`, `FormComponentArray` (for containers), `Enum`, `Bitfield`.
We can add the rest as we move into Tier 3.

## Implementation order (recap of the build plan)

1. **Workstream A** — parsing bugs (CellRecord spin, post-load crash, log cleanup)
2. **Workstream B** — QtAdvancedDocking integration
3. **Workstream C.1** — core abstractions (Component, EditorProperty, FormComponentWidget, EditorPropertyGrid)
4. **Workstream C.2** — Tier 1 components
5. **Workstream C.4** — migrate 8 record types
6. **Workstream C.5** — QtFormDialog
7. **Workstream C.6** — delete bespoke editors
8. **Workstream C.7** — wire ObjectWindowDialog
9. **Pause and verify** — edit a STAT, an ARMO, an NPC_ end-to-end
10. **Workstream C.3** — Tier 2 components
11. **Workstream B.4** — WindowLayout persistence
12. **Workstream C.8** — tests
13. **Workstream D** — docs

## Where to go for reference during implementation

- **OpenCK source** — read the current `stat_editor.cpp`, `armor_editor.cpp`,
  `objectpalette.cpp`, `objectwindowdialog.cpp`, `mainwindow.cpp` to see the
  *current* shape. We're replacing these with the component pattern.
- **xEdit (TES5Edit/SSEEdit/FO4Edit/xEdit)** — the open-source Bethesda
  modder's tool, written in Pascal/Delphi, has been the de-facto open
  implementation of Bethesda's record structures for 20 years. Its source
  (https://github.com/TES5Edit/TES5Edit) is the best reference for record
  subrecord layout and parsing. We follow its parsing conventions, not CK's.
- **NifTools** — open-source NIF parsing, MIT-licensed. We use it already.
- **QtAdvancedDocking docs** — `githubuser0xFFFF/Qt-Advanced-Docking-System`
  has usage examples in its `demo/` directory.
- **The real `CreationKit.exe`** — install dir at
  `C:\Program Files (x86)\Steam\steamapps\common\Starfield\`. Run it once to
  see the UI in action. We do not decompile it.
