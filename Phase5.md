# Phase 5: Advanced Editing — Animation (10-14 hours)

## Architecture Audit Summary

Three disconnected animation layers exist with a critical data-flow gap:

```
┌──────────────────────┐     BROKEN      ┌──────────────────────┐
│ NifAnimationParser   │ ──────────────▶ │ NifAnimationState    │
│ (byte-pattern scan)  │   no connection  │ (playback engine)    │
│ Used by AnimEditor   │                 │ Used by Viewport     │
└──────────────────────┘                 └──────────────────────┘
         ▲                                         ▲
         │              BROKEN                     │
┌────────┴────────────────────────────────────────┴───────┐
│ NifParser block-level parsing                           │
│ - Recognizes NiKeyframeController ✓                     │
│ - Recognizes NiControllerManager ✓                      │
│ - DOES NOT parse NiKeyframeData ✗ (no block class)     │
│ - DOES NOT parse NiTransformData ✗ (no block class)    │
│ - keyframeDataRef block data: skipped at line 602-606   │
└─────────────────────────────────────────────────────────┘
```

**Root cause**: `NiKeyframeData` / `NiTransformData` block types don't exist in `nifrecord.hpp`. The NIF parser can't extract actual keyframe float arrays from NIF files. The standalone `NifAnimationParser` works around this with raw byte scanning but is fragile and disconnected.

**Fix strategy**: Build from the bottom up — ESM block types → NIF parser integration → bridge to NifAnimation format → viewport population → editor UI.

---

## Step 5.1: NIF Keyframe Data Blocks (Estimated: 2-3 hours)

**Goal**: Parse actual keyframe float data from NIF files.

### Step 5.1.1: Add NiKeyframeData block class to nifrecord.hpp/cpp
- Add `class NifKeyframeData : public NifObject` with fields:
  - `quint32 numKeys`
  - `QVector<Nif::TransformKeyframe> keyframes` (time + translation + quaternion rotation + scale)
  - `quint32 interpolationType` (linear/quaternion/etc.)
- Add `parse()` method that reads: numKeys, then for each key: time(float), tx ty tz(float×3), qw qx qy qz(float×4), sx sy sz(float×3)
- Add `write()` method for export
- Register in CMakeLists.txt

### Step 5.1.2: Add NiTransformData block class to nifrecord.hpp/cpp
- Add `class NifTransformData : public NifObject` with fields:
  - Translation keys: `QVector<Nif::Vector3Keyframe>`
  - Rotation keys: `QVector<Nif::QuaternionKeyframe>`
  - Scale keys: `QVector<Nif::Vector3Keyframe>`
- Add `parse()` and `write()` methods
- Register in CMakeLists.txt

### Step 5.1.3: Register new block types in NifParser
- In `nifparser.cpp` block type switch (~line 398), add cases for:
  - `"NiKeyframeData"` → `new NifKeyframeData()`
  - `"NiTransformData"` → `new NifTransformData()`
  - Also handle `"NiFlipController"`, `"NiMaterialColorController"`, `"NiVisController"` as skip-safe unknowns

### Step 5.1.4: Connect keyframeDataRef to NiKeyframeController
- In `nifparser.cpp` `extractGeometry()` (~line 599-607), after getting `dataBlock`:
  - `dynamic_cast<NifKeyframeData*>(dataBlock)` to extract keyframes
  - Populate `anim.keyframes` from the parsed data
  - Handle both NiKeyframeData (Euler) and NiTransformData (quaternion) formats
  - Convert quaternion rotation to Euler angles for `TransformKeyframe`

### Step 5.1.5: Build verification
- `cmake --build build --target openck --config Debug`
- Verify zero errors

---

## Step 5.2: Bridge NifParser → NifAnimation (Estimated: 1-2 hours)

**Goal**: Connect the block parser's animation data to the playback system.

### Step 5.2.1: Populate AnimClip channels from NifParser in initAnimationState()
- In `nifviewportwidget.cpp` `initAnimationState()` (~line 1930-1934), replace empty `AnimClip` creation:
  - Call `nifParser->getAllAnimationControllers()` to get `QVector<NiKeyframeController>`
  - For each controller, look up target node name via `refToNode` map
  - Create `AnimChannel` with `boneName = targetNode->name`
  - Convert `Nif::TransformKeyframe` (quaternion rotation) → `AnimKeyframe` (Euler rotation)
  - Populate `channel.keyframes` from controller's keyframes
  - Set `channel.duration` from last keyframe time
  - Append populated channels to the `AnimClip`

### Step 5.2.2: Add NifParser helper to return animation data in AnimClip format
- Add method to `NifParser`: `QVector<AnimClip> getAnimClips() const`
  - Internally calls `getAllAnimationControllers()`, groups by clip name
  - Converts quaternion rotations to Euler for each keyframe
  - Returns ready-to-use `AnimClip` objects

### Step 5.2.3: Build verification
- `cmake --build build --target openck --config Debug`
- Verify zero errors

---

## Step 5.3: Animation Timeline Widget (Estimated: 2-3 hours)

**Goal**: Visual keyframe editor replacing the current table-based view.

### Step 5.3.1: Create TimelineWidget class (new file)
- `src/view/window/timeline.hpp/cpp`
- Custom QWidget with paintEvent rendering:
  - Horizontal time ruler with tick marks (0.1s increments, 0.5s major ticks)
  - Vertical lanes per bone/channel (expandable)
  - Diamond-shaped keyframe indicators at their time positions
  - Current time indicator (red vertical line)
  - Selection highlight (blue background on selected keyframes)
  - Drag handle at time indicator base
- Mouse interaction:
  - Click on timeline → set current time
  - Click on keyframe → select it
  - Drag keyframe → move it (snaps to 0.01s grid)
  - Shift+click → range select
  - Delete key → remove selected keyframes
  - Ctrl+C / Ctrl+V → copy/paste keyframes
  - Double-click on empty space → add keyframe at that time
- Signals: `keyframeMoved(bone, oldTime, newTime)`, `keyframeSelected(bone, time)`, `keyframeAdded(bone, time)`, `keyframeRemoved(bone, time)`, `timeChanged(float)`
- Register in CMakeLists.txt

### Step 5.3.2: Replace QTableWidget in AnimationEditor with TimelineWidget
- Remove `mKeyframeTable` member from `AnimationEditor`
- Add `TimelineWidget* mTimeline` member
- Connect timeline signals to editor slots
- Update `updateTimeline()` to feed timeline with clip channel data

### Step 5.3.3: Add keyframe property panel
- Right side panel showing selected keyframe properties:
  - Bone name (read-only)
  - Time (editable spinbox)
  - Translation X/Y/Z (editable spinboxes)
  - Rotation X/Y/Z (editable spinboxes, degrees)
  - Scale X/Y/Z (editable spinboxes)
- Changes update the timeline and underlying data model

### Step 5.3.4: Build verification
- `cmake --build build --target openck --config Debug`
- Verify zero errors

---

## Step 5.4: Animation Editing Commands (Estimated: 1-2 hours)

**Goal**: Undo/redo support for all keyframe operations.

### Step 5.4.1: Add MoveKeyframeCommand
- `src/model/tools/movekeyframecommand.hpp/cpp`
- Stores: bone name, old time, new time, keyframe data
- `undo()`: moves keyframe back to old time
- `redo()`: moves keyframe to new time
- Register in CMakeLists.txt

### Step 5.4.2: Add AddKeyframeCommand
- `src/model/tools/addkeyframecommand.hpp/cpp`
- Stores: bone name, time, keyframe data
- `undo()`: removes keyframe
- `redo()`: inserts keyframe at sorted position
- Register in CMakeLists.txt

### Step 5.4.3: Add RemoveKeyframeCommand
- `src/model/tools/removekeyframecommand.hpp/cpp`
- Stores: bone name, time, keyframe data (for undo)
- `undo()`: re-inserts keyframe
- `redo()`: removes keyframe
- Register in CMakeLists.txt

### Step 5.4.4: Integrate commands with AnimationEditor
- All editing operations go through QUndoStack
- Ctrl+Z / Ctrl+Y wired to undo stack

### Step 5.4.5: Build verification
- `cmake --build build --target openck --config Debug`
- Verify zero errors

---

## Step 5.5: Animation Import/Export (Estimated: 1-2 hours)

**Goal**: Load/save animation data from external formats.

### Step 5.5.1: Implement BSAnimationGraphShader parser
- In `nifparser.cpp` block type switch, add case for `"BSAnimationGraphShader"` → create `NifNode` (treat as extra data on node)
- Parse: `linkedAnim`, `variable.find()`, `animationName` fields
- Store parsed data on the target node for later retrieval

### Step 5.5.2: Add animation export to XML/JSON
- Add `NifAnimationExporter` class in `libs/files/nifanim/`
- `exportToXml(NifAnimation*, QString path)` — writes all clips, channels, keyframes as XML
- `exportToJson(NifAnimation*, QString path)` — writes as JSON
- Register in CMakeLists.txt
- Wire to AnimationEditor export button

### Step 5.5.3: Add animation import from XML/JSON
- `NifAnimationImporter` class in `libs/files/nifanim/`
- `importFromXml(QString path)` → `NifAnimation*`
- `importFromJson(QString path)` → `NifAnimation*`
- Register in CMakeLists.txt
- Wire to AnimationEditor import button

### Step 5.5.4: Add animation write-back to NIF
- `NifAnimationWriter` — writes modified keyframes back into NIF binary
- Updates `NiKeyframeData` blocks in-place
- Handles adding new blocks for new channels

### Step 5.5.5: Build verification
- `cmake --build build --target openck --config Debug`
- Verify zero errors

---

## Step 5.6: Animation Preview (Estimated: 1-2 hours)

**Goal**: Full playback with blending and loop controls.

### Step 5.6.1: Fix AnimationEditor preview to render in viewport
- Replace local QTimer scrubbing with actual NifAnimationState integration
- Embed a NifViewportWidget (or connect to main viewport) for live preview
- Play/pause/stop buttons control NifAnimationState

### Step 5.6.2: Implement animation blending in NifAnimationState
- Add `m_blendAnim` / `m_blendChannels` / `m_blendWeight` members
- Add `blendWith(NifAnimation*, float weight)` method
- In `getCurrentFrame()`, interpolate between current and blend channels using weight
- Add `setBlendWeight(float)` for cross-fade control

### Step 5.6.3: Add blend controls to animation toolbar
- Second clip combo for blend target
- Blend weight slider (0-100%)
- Cross-fade duration spinbox

### Step 5.6.4: Add event markers / bookmarks
- Click on timeline to add named markers (e.g., "footstep", "attack")
- Markers stored as `QVector<AnimMarker>` with `{time, name, color}`
- Export markers with animation data

### Step 5.6.5: Build verification
- `cmake --build build --target openck --config Debug`
- Verify zero errors

---

## Step 5.7: Dead Code Cleanup & Final Audit (Estimated: 1 hour)

**Goal**: Zero stubs, zero dead code, zero TODOs.

### Step 5.7.1: Remove NifAnimationParser byte-scanner
- Delete `libs/files/nifanim/nifanimation.cpp` and `.hpp`
- Remove from CMakeLists.txt
- The block parser now handles everything; the byte scanner is redundant

### Step 5.7.2: Remove AnimationEditor stub
- Delete `AnimationEditor::updateProperties()` empty method
- Either implement it (show keyframe details in properties panel from Step 5.3.3) or remove the declaration

### Step 5.7.3: Audit all animation files for TODOs/stubs
- `grep -rn "TODO\|FIXME\|HACK\|stub\|STUB" src/view/window/animationeditor.cpp src/view/window/timeline.cpp src/view/window/nifviewportwidget.cpp src/model/tools/nifanimationstate.hpp libs/files/nifanim/ libs/files/nif/nifparser.cpp libs/files/esm/nifrecord.cpp`
- Fix every finding

### Step 5.7.4: Update TECHNICAL_DEBT.md
- Remove P4-05 (node hierarchy — fixed), P4-10 (animation playback — fixed)
- Add any new items discovered during Phase 5

### Step 5.7.5: Final build verification
- `cmake --build build --target openck --config Debug`
- Verify zero errors, zero warnings (if possible)

---

## File Summary

| Action | File | Step |
|--------|------|------|
| **Edit** | `libs/files/esm/nifrecord.hpp` | 5.1.1, 5.1.2 |
| **Edit** | `libs/files/esm/nifrecord.cpp` | 5.1.1, 5.1.2 |
| **Edit** | `libs/files/nif/nifparser.cpp` | 5.1.3, 5.1.4, 5.5.1 |
| **Edit** | `src/view/window/nifviewportwidget.cpp` | 5.2.1 |
| **Edit** | `src/view/window/animationeditor.cpp` | 5.3.2, 5.4.4, 5.5.2-3, 5.6.1 |
| **Edit** | `src/view/window/animationeditor.hpp` | 5.3.2, 5.6.1 |
| **Edit** | `CMakeLists.txt` | 5.1.5, 5.3.1, 5.4.1-3, 5.5.2-3, 5.7.1 |
| **New** | `src/view/window/timeline.hpp` | 5.3.1 |
| **New** | `src/view/window/timeline.cpp` | 5.3.1 |
| **New** | `src/model/tools/movekeyframecommand.hpp` | 5.4.1 |
| **New** | `src/model/tools/movekeyframecommand.cpp` | 5.4.1 |
| **New** | `src/model/tools/addkeyframecommand.hpp` | 5.4.2 |
| **New** | `src/model/tools/addkeyframecommand.cpp` | 5.4.2 |
| **New** | `src/model/tools/removekeyframecommand.hpp` | 5.4.3 |
| **New** | `src/model/tools/removekeyframecommand.cpp` | 5.4.3 |
| **New** | `libs/files/nifanim/nifanimationexporter.hpp` | 5.5.2 |
| **New** | `libs/files/nifanim/nifanimationexporter.cpp` | 5.5.2 |
| **New** | `libs/files/nifanim/nifanimationimporter.hpp` | 5.5.3 |
| **New** | `libs/files/nifanim/nifanimationimporter.cpp` | 5.5.3 |
| **New** | `libs/files/nifanim/nifanimationwriter.hpp` | 5.5.4 |
| **New** | `libs/files/nifanim/nifanimationwriter.cpp` | 5.5.4 |
| **Delete** | `libs/files/nifanim/nifanimation.hpp` | 5.7.1 |
| **Delete** | `libs/files/nifanim/nifanimation.cpp` | 5.7.1 |
| **Edit** | `TECHNICAL_DEBT.md` | 5.7.4 |

---

## Iteration Schedule

| Iteration | Steps | Gate |
|-----------|-------|------|
| **Iter 1** | 5.1 → 5.2 | `cmake --build` passes, NIF files load with real keyframe data in viewport |
| **Iter 2** | 5.3 → 5.4 | `cmake --build` passes, TimelineWidget renders keyframes, undo/redo works |
| **Iter 3** | 5.5 → 5.6 | `cmake --build` passes, import/export works, blending plays correctly |
| **Iter 4** | 5.7 | Final audit: grep for stubs/TODOs, delete dead code, final build passes |
