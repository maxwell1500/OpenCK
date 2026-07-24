# Phase 5 Technical Debt: Performance, Shortcuts & Code Quality

## Step 5.8: Keyboard Shortcuts — Fix Conflicts & Wire Missing (Estimated: 1-2 hours)

**Goal**: All shortcuts match real CK, zero conflicts, all wired.

### Step 5.8.1: Fix shortcut conflicts
- F11: WeatherLight vs ToggleFullscreen → keep WeatherLight, reassign ToggleFullscreen to F11 only if no conflict
- Ctrl+Shift+W: Worldspaces vs NewWeapon → keep Worldspaces, move NewWeapon to Ctrl+Shift+N
- Ctrl+Shift+S: SaveAsPlugin vs NewSpell → keep SaveAsPlugin, move NewSpell to Ctrl+Shift+E (Edit spell)
- F5: ObjectWindow vs ResetVisibility → keep F5 as Refresh (real CK), wire ObjectWindow to F6

### Step 5.8.2: Fix wrong assignments
- Ctrl+P: Change from ObjectPalette to Preferences
- F5: Change from ObjectWindow to Refresh (actionRefresh or similar)
- FocusCamera: Change from Shift+F to F (real CK)
- Preferences: Change from Ctrl+, to Ctrl+P

### Step 5.8.3: Wire unwired shortcuts in setupShortcuts()
- NewPlugin: Ctrl+N
- ClosePlugin: Ctrl+W
- FindNext: F3
- FindPrevious: Shift+F3
- SearchAndReplace: Ctrl+H
- FocusCamera: F
- TopDown: T
- CycleView: Y
- ToggleMarkers: M
- MovementGizmo: E
- RotationGizmo: W
- ScaleGizmo: S (conflicts with Scale — use R for real CK)
- GridSnap: Q
- AxisX: X
- AxisY: C (real CK)
- AxisZ: Z
- Ghost/Hidden/Visible: 1 (cycle)

### Step 5.8.4: Add missing shortcuts
- Ctrl+1/2/3: Wire/Render/Solid modes (add actions if needed)
- Ctrl+G: Toggle Grid
- Ctrl+B: Toggle Bounding Boxes
- Home: Reset camera in render window
- Z: Toggle wireframe overlay in render window

### Step 5.8.5: Build verification
- cmake --build

---

## Step 5.9: Performance — VBO Caching (Estimated: 2-3 hours)

**Goal**: Stop rebuilding vertex buffers every frame. Single biggest perf win.

### Step 5.9.1: Add dirty flag to NifViewportWidget
- Add `bool m_meshDirty = true` member
- Set true in: buildMesh(), applyAnimationFrame(), onFilterChanged(), translateMesh(), scaleMesh()
- Set false after VBO upload in renderMesh()

### Step 5.9.2: Cache VBO data
- Only rebuild interleaved vertex array when m_meshDirty is true
- Only call vbo.allocate() / ibo.allocate() when m_meshDirty is true
- Only set vertex attribute pointers when m_meshDirty is true
- When not dirty, just call glDrawElements()

### Step 5.9.3: Sort shapes by material state
- Before rendering, sort shapes by: alpha mode (opaque first, then sorted transparent), then by texture, then by shader properties
- Store sorted shape order in a QVector<int> m_sortedShapeOrder
- Re-sort only when m_meshDirty is true

### Step 5.9.4: Optimize applyAnimationFrame()
- Move normal matrix computation outside inner loop (per-shape, not per-vertex)
- Use QElapsedTimer to measure actual frame time instead of fixed 33ms

### Step 5.9.5: Build verification
- cmake --build

---

## Step 5.10: Performance — Overlay VBOs (Estimated: 1-2 hours)

**Goal**: Replace immediate-mode GL overlays with VBOs.

### Step 5.10.1: Create NavmeshVBO class
- `src/view/window/navmeshvbo.hpp/cpp`
- When navmesh data is set, build a VBO with all triangles
- Store vertex count for draw
- Only rebuild when navmesh data changes

### Step 5.10.2: Create GridVBO class
- `src/view/window/gridvbo.hpp/cpp`
- Grid is static — build once, draw many times
- Store line vertices in VBO

### Step 5.10.3: Create BoundingBoxVBO class
- Rebuild when selection changes

### Step 5.10.4: Replace glBegin/glEnd in nifviewportwidget.cpp
- Replace navmesh rendering with VBO draw
- Replace grid rendering with VBO draw
- Replace bounding box rendering with VBO draw
- Replace path rendering with VBO draw
- Replace collision shape rendering with VBO draw

### Step 5.10.5: Build verification
- cmake --build

---

## Step 5.11: Code Quality — Eliminate Switch Duplication (Estimated: 3-4 hours)

**Goal**: Replace 30-way switch statements with virtual dispatch.

### Step 5.11.1: Create IRecordCollection interface
- `src/model/world/irecordcollection.hpp`
- Pure virtual methods: cloneRecord(), removeRecord(), getRecordCount(), getRecordByIndex(), searchId(), etc.
- Template-free, uses void* or BaseRecord* for type erasure

### Step 5.11.2: Make each collection implement IRecordCollection
- IdCollection<T> already has most methods — just add the interface
- Collection<T> same
- Each type's methods are already implemented, just need to expose through virtual interface

### Step 5.11.3: Refactor data.cpp removeRecord()
- Replace 456-line switch with: `getCollectionByType(type)->removeRecord(index)`
- Keep undo logic in data.cpp but delegate record operations to collection

### Step 5.11.4: Refactor data.cpp cloneRecord() / batchCloneWithUndo()
- Replace 30-case switches with loop over collection interface

### Step 5.11.5: Refactor data.cpp createNewRecord()
- Replace 26 identical lambdas with a single FormID allocation bitmap
- Add `quint32 allocateFormId()` method to Data class

### Step 5.11.6: Refactor data.cpp detectConflicts()
- Replace 30 checkCollection calls with loop

### Step 5.11.7: Refactor document.cpp save()
- Replace 383-line save with loop: `for each collection: for each record: if modified: save`

### Step 5.11.8: Refactor objectwindowdialog.cpp editSelected() / deleteSelected() / cloneSelected()
- Replace per-type switches with collection interface calls

### Step 5.11.9: Build verification
- cmake --build

---

## Step 5.12: Code Quality — Editor Undo Correctness (Estimated: 2-3 hours)

**Goal**: All editor edits go through undo stack atomically.

### Step 5.12.1: Make editors work on copies
- Each editor (NpcEditor, WeaponEditor, ArmorEditor, etc.) should take a COPY of the record
- On accept, create an EditRecordCommand with old state + new state
- On reject, discard the copy — original is untouched

### Step 5.12.2: Fix const_cast abuse
- Remove const_cast from objectwindowdialog.cpp
- Use collection.mutableRecord(index) or similar API
- Editors take non-const references or copies

### Step 5.12.3: Wire ColumnValidator to all editors
- Import ColumnValidator in each editor
- Call validateRecord() before commit
- Show validation errors to user

### Step 5.12.4: Fix validation-before-commit bug
- Save UI values to record BEFORE validation
- Validate the saved values
- Only commit to undo stack if validation passes

### Step 5.12.5: Build verification
- cmake --build

---

## Step 5.13: Performance — BA2 Memory Mapping (Estimated: 1-2 hours)

**Goal**: Reduce BA2 memory usage from archive-size to file-table-size.

### Step 5.13.1: Memory-map BA2 files
- Replace `mRawData = file.readAll()` with `file.map(0, file.size())`
- Store as `uchar* mMappedData` + `qint64 mFileSize`
- Parse file table from mapped data

### Step 5.13.2: Update extractEntry()
- Write directly from mapped pointer: `outFile.write(reinterpret_cast<const char*>(mMappedData + offset), size)`
- No intermediate QByteArray copy

### Step 5.13.3: Update file() accessor
- Return QByteArray::fromRawData() from mapped pointer (zero-copy when possible)

### Step 5.13.4: Handle cleanup
- `file.unmap(mMappedData)` in destructor
- Remove mRawData member

### Step 5.13.5: Build verification
- cmake --build

---

## Step 5.14: Code Quality — Exception Safety & Error Handling (Estimated: 1-2 hours)

**Goal**: No silent failures, proper error reporting.

### Step 5.14.1: Replace catch(...) with catch(Exception&)
- In data.cpp clone operations, catch `std::exception&` and log the error message
- Show QMessageBox on failure

### Step 5.14.2: Fix silent clone failures
- Add LOG_ERROR for every failed clone
- Show user-facing error message

### Step 5.14.3: Fix deleted-record save (DELE flag)
- In document.cpp save(), write DELE subrecord for deleted records
- Mark deleted records in collection state

### Step 5.14.4: Wire validation to save operations
- Call ColumnValidator::validateRecord() before every save
- Show validation errors in status bar or error list

### Step 5.14.5: Build verification
- cmake --build

---

## Iteration Schedule

| Iteration | Steps | Gate |
|-----------|-------|------|
| **Iter 1** | 5.8 (Shortcuts) | Build passes, shortcuts work |
| **Iter 2** | 5.9 (VBO caching) + 5.10 (Overlay VBOs) | Build passes, frame rate improves |
| **Iter 3** | 5.11 (Switch elimination) | Build passes, code reduced by ~2000 lines |
| **Iter 4** | 5.12 (Editor undo) + 5.14 (Error handling) | Build passes, edits undoable |
| **Iter 5** | 5.13 (BA2 memory) | Build passes, memory reduced |
| **Iter 6** | Final audit | Zero stubs/TODOs/dead code |
