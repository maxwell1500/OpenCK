# Stage 2: Deep Code Audit & Quality Improvements

> **Goal:** Fix the top-priority code quality issues discovered during a comprehensive
> deep-dive code review across the entire OpenCK codebase.
>
> **Estimated effort:** ~40-60 hours across 10 major work items
> **Current technical debt resolved:** 36/36 items (100%) — Stage 1 complete
> **Stage 2 target:** Resolve 10 systemic code quality issues

---

## Overview

A thorough multi-agent code review was conducted across all layers of OpenCK:
- **Model layer** (`src/model/`) — Data, collections, commands, undo stack, tools
- **View layer** (`src/view/`) — MainWindow, editors, palette, compiler, widgets
- **ESM layer** (`src/libs/files/esm/`) — Record types, reader/writer, parsers
- **Utility layer** (`src/libs/files/`, `src/libs/nif/`, `src/libs/files/audio/`)
- **Test suite** (`tests/`) — All 18 test files
- **Build system** (`CMakeLists.txt`, `tests/CMakeLists.txt`)

**Total issues identified:** ~50+ across 4 severity levels (Critical, High, Medium, Low)

---

## Priority Matrix

| Priority | Count | Description |
|----------|-------|-------------|
| 🔴 Critical | 2 | Data corruption / non-functional components |
| 🟠 High | 7 | Design flaws, broken rendering, silent bugs |
| 🟡 Medium | 15 | Maintainability, performance, portability |
| 🟢 Low | ~26 | Style, naming, dead code, minor issues |

---

## Critical Issues

### C-01: ESM Subrecord Names Don't Match Any TES Format

**Severity:** 🔴 CRITICAL
**Files:** `src/libs/files/esm/*record.cpp` (all record type files)
**Category:** Data integrity — fundamental architectural flaw

#### Problem

The ESM layer uses invented subrecord names that do not correspond to any real
Bethesda plugin format (TES3, TES4, TES5). The ESM reader/writer **cannot
correctly parse or write any real plugin file**.

| Invented Name | Should Be | Used In | Actual TES Meaning |
|---------------|-----------|---------|-------------------|
| `FORM` | (header field) | TreeRecord:14, StatRecord:14, ActiRecord:14, etc. | FormID is part of record header, not a subrecord |
| `ITM2` | `ICON` | TreeRecord:16, etc. | Inventory icon path |
| `ODIT` | `MODL` | TreeRecord:17, etc. | Model filename |
| `FNAM` | (depends on record) | TreeRecord:15 (as int flags) | Full name string in actual TES formats |

#### Impact

Every record saved through this layer produces a corrupt plugin file that
cannot be loaded by the game, Creation Kit, or any mod manager. This is a
**showstopper bug** for the entire project's core purpose.

#### Root Cause

The record types appear to have been written from guessed/synthesized field
names rather than reverse-engineered from actual plugin files or reference
documentation (e.g., UESP, xEdit source).

#### Recommended Fix

1. Document the correct subrecord schema for each record type from authoritative
   sources (UESP wiki, xEdit source, or actual plugin hex dumps)
2. Rewrite each record's `load()` / `save()` methods with correct subrecord names
3. Add round-trip tests that write a record, read it back, and verify field values
4. Consider using an enum-based subrecord registry instead of raw `char[4]` literals

**Estimated effort:** 20-30 hours (all record types need revision)

---

### C-02: `RecordEditCommand::execute()` Is a No-Op

**Severity:** 🔴 CRITICAL
**File:** `src/model/tools/recordeditcommand.hpp:29-34`
**Category:** Broken functionality — command pattern

#### Problem

```cpp
void execute() override {
    Record<ESXRecord>& rec = const_cast<Record<ESXRecord>&>(
        mCollection.getRecord(mIndex));
    mNewState = rec.get();        // Captures current state — OK
    rec.setModified(rec.get());   // Sets modified to current state — NO CHANGE
}
```

`execute()` stores the current state in `mNewState` but never applies a new
state. The record is set to its own current value — a complete no-op.
`undo()` correctly restores `mOriginalState` (captured in constructor), so
undo works but the initial edit action does nothing.

#### Impact

Any edit operation using `RecordEditCommand` silently does nothing. The user
thinks they edited a record, but on re-open the original values remain.
This is a **data integrity issue** because the user has no feedback that their
edit was discarded.

#### Root Cause

The class was designed to accept the new state via `execute()` (because the
command pattern for edit often stores original state in constructor and new
state in `execute()`), but the implementation never stores what `execute()`
receives. The `mNewState` member is written but never applied.

#### Recommended Fix

Option A (minimal):
```cpp
void execute() override {
    Record<ESXRecord>& rec = const_cast<Record<ESXRecord>&>(
        mCollection.getRecord(mIndex));
    mNewState = rec.get();
    rec.setModified(mNewState);  // Already a no-op — need to apply external value
}
```

Option B (correct — accept new state in constructor):
```cpp
RecordEditCommand(Collection<ESXRecord>& collection, int index,
                  const ESXRecord& newState)
    : mCollection(collection), mIndex(index),
      mOriginalState(collection.getRecord(index).get()),
      mNewState(newState) {}

void execute() override {
    Record<ESXRecord>& rec = const_cast<Record<ESXRecord>&>(
        mCollection.getRecord(mIndex));
    rec.setModified(mNewState);
}

void undo() override {
    Record<ESXRecord>& rec = const_cast<Record<ESXRecord>&>(
        mCollection.getRecord(mIndex));
    rec.setModified(mOriginalState);
}
```

**Estimated effort:** 1-2 hours

---

## High Issues

### H-01: Systemic `const_cast` Abuse (200+ occurrences)

**Severity:** 🟠 HIGH
**Files:**
- `src/view/window/objectwindowdialog.cpp` — 200+ lines
- `src/view/window/searchdialog.cpp` — ~100 lines
- `src/view/window/mainwindow.cpp` — lines 902, 906, 1272
- `src/view/window/npceditor.cpp` — line 726
- `src/view/window/dialogueeditorwidget.cpp` — lines 111, 118, 150, 183, 211
- `src/view/window/dialoguetreeeditor.cpp` — lines 189, 196, 310, 333
- `src/view/window/aipackageeditor.cpp` — lines 148, 220
- `src/view/window/formideditorwidget.cpp` — lines 144, 156, 168, 180, 192
- `src/view/window/conflictdialog.cpp` — line 190
- `src/view/window/dialeditor.cpp` — line 384
- `src/view/window/facteditor.cpp` — line 160
- `src/model/world/data.cpp` — line 580
- Plus many more
**Category:** Design flaw — undefined behavior risk

#### Problem

Every call to `mData->getXxxCollection()` returns a `const IdCollection<T>&`
reference. Callers needing non-const access cast away const:

```cpp
auto& weaponCollection = const_cast<IdCollection<WeaponRecord>&>(
    mData->getWeaponCollection());
```

This pattern is repeated 200+ times across the codebase. If the underlying
object was originally declared `const`, this is undefined behavior.

#### Impact

- **UB risk** if the Data object is ever made truly const
- **Fragile design** — const-correctness cannot be enforced
- **Maintenance burden** — every new editor/view must replicate the pattern
- **Code noise** — obscures actual logic behind boilerplate casts

#### Root Cause

The `Data` class exposes all collections through `const` reference getters,
but the view layer needs to modify them. This suggests the `Data` class was
designed with a read-only public interface, but the requirement for mutation
was added later.

#### Recommended Fix

1. Add non-const overloads to `Data` class:
```cpp
class Data {
    // Existing const getters (for read-only access)
    const IdCollection<WeaponRecord>& getWeaponCollection() const;

    // New non-const getters (for mutation)
    IdCollection<WeaponRecord>& getWeaponCollection();
};
```

2. Remove all `const_cast` calls across the codebase

3. (Optional) Return `Collection<ESXRecord>*` from a single templated accessor
   to avoid 30+ separate getter methods

**Estimated effort:** 4-6 hours (mechanical but across many files)

---

### H-02: OpenGL VBO Double-Allocation (Terrain Rendering Broken)

**Severity:** 🟠 HIGH
**File:** `src/view/window/landscapeeditor.cpp:577-582`
**Category:** Broken rendering — graphics bug

#### Problem

```cpp
vbo.allocate(terrainVertices.constData(),
    terrainVertices.size() * sizeof(QVector3D));              // Line 577
shaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3,
    sizeof(QVector3D));                                        // Line 578
shaderProgram->enableAttributeArray(0);                        // Line 579

vbo.allocate(terrainNormals.constData(),
    terrainNormals.size() * sizeof(QVector3D));                // Line 581 — RE-ALLOCATES!
shaderProgram->setAttributeBuffer(1, GL_FLOAT,
    terrainVertices.size() * sizeof(QVector3D), 3,
    sizeof(QVector3D));                                        // Line 582
shaderProgram->enableAttributeArray(1);                        // Line 583
```

The second `vbo.allocate()` call (line 581) **re-allocates the entire VBO**,
invalidating the vertex attribute pointer set at line 578. The vertex attribute
now points at stale/freed memory. Normals are in the buffer, but vertices
render garbage.

#### Impact

- **Terrain geometry is invisible or displays as garbage**
- Lighting is uniform because normals are all `(0,1,0)` (set at line 563-566)
- The landscape editor's 3D preview is effectively non-functional

#### Root Cause

The developer assumed multiple `vbo.allocate()` calls would append to the
buffer, but OpenGL VBOs work differently — each `allocate()` call replaces
the entire buffer contents.

#### Recommended Fix

Option A (use separate VBOs):
```cpp
QOpenGLBuffer vertexVbo, normalVbo;
vertexVbo.allocate(terrainVertices.constData(), ...);
normalVbo.allocate(terrainNormals.constData(), ...);
// Set attribute 0 from vertexVbo, attribute 1 from normalVbo
```

Option B (interleave or concatenate in one VBO):
```cpp
vbo.allocate(vertexDataSize + normalDataSize);
vbo.write(0, terrainVertices.constData(), vertexDataSize);
vbo.write(vertexDataSize, terrainNormals.constData(), normalDataSize);
```

**Estimated effort:** 2-3 hours

---

### H-03: `QSpinBox` Used for Float Positions (Silent Data Loss)

**Severity:** 🟠 HIGH
**File:** `src/view/window/ref_editor.cpp:70-80,140-142,201`
**Category:** Data integrity — precision loss

#### Problem

```cpp
mPosXSpin = new QSpinBox();           // Line 70 — INTEGER spinbox
mPosXSpin->setValue(static_cast<int>(mRef->posX));  // Line 140 — truncates!
// ...
mRef->posX = static_cast<float>(mPosXSpin->value());  // Line 201 — already truncated
```

Positions (`posX`, `posY`, `posZ`) are stored as `float` in `RefrRecord`,
but the editor uses `QSpinBox` (integer). Every load truncates the decimal
portion, and every save writes the truncated value back.

#### Impact

**All fractional position values are silently destroyed.** In a modding context
where precise positioning matters (e.g., aligning objects, NPC pathing nodes),
this causes objects to be visibly misplaced when re-opened in the game or CK.

#### Root Cause

Copy-paste from an integer field editor pattern without considering that
coordinates require sub-unit precision.

#### Recommended Fix

Replace `QSpinBox` with `QDoubleSpinBox`:
```cpp
mPosXSpin = new QDoubleSpinBox();
mPosXSpin->setRange(-100000.0, 100000.0);
mPosXSpin->setDecimals(3);   // 3 decimal places for sub-unit precision
mPosXSpin->setSingleStep(1.0);
// Remove the static_cast<int> on load and static_cast<float> on save
```

Also check all other editors for the same pattern (recommend a search for
`new QSpinBox()` used with float data members).

**Estimated effort:** 1-2 hours

---

### H-04: `QDataStream` Byte-Order Mismatch (Save/Load Incompatible)

**Severity:** 🟠 HIGH
**File:** `src/view/window/objectpalette.cpp:357,431-445`
**Category:** Data integrity — file corruption

#### Problem

```cpp
// Save (lines 431-445) — no byte order set, uses default BigEndian
QDataStream out(&file);
out << refFormId << baseObj << ...;

// Load (line 357) — explicitly sets LittleEndian
QDataStream in(&fileData, QIODevice::ReadOnly);
in.setByteOrder(QDataStream::LittleEndian);
```

`QDataStream` default byte order is `BigEndian`. The save function does not
set a byte order, so files are written in BigEndian. The load function sets
`LittleEndian`. **All multi-byte values are byte-swapped on load.**

Additionally, the file extension is `.json` but the format is binary
(`QDataStream`), which is misleading.

#### Impact

**Placement files cannot be loaded correctly.** Every float and int32 value
will have swapped bytes, producing garbage coordinates, rotation values, and
scales. Objects will appear at wildly incorrect positions or cause crashes.

#### Root Cause

The save and load functions were written by different developers (or at
different times) and the byte order specification was not coordinated.

#### Recommended Fix

1. Add `out.setByteOrder(QDataStream::LittleEndian)` to the save function
2. Rename file extension to `.placement` or `.bin` (not `.json`)

**Estimated effort:** < 1 hour

---

### H-05: `getColumnId()` Always Returns Wrong Value

**Severity:** 🟠 HIGH
**File:** `src/model/world/columns.cpp:35-48`
**Category:** Logic bug — feature completely broken

#### Problem

```cpp
int getColumnId(const QString& name)
{
    QString lower = name.toLower();
    for (int i = 0; i < ColumnId_End; i++)
    {
        if (lower == name.toLower())   // BUG: compares against ITSELF
            return columnNames[i].id;
    }
    return -1;
}
```

The comparison `lower == name.toLower()` compares the already-lowercased
`lower` variable against... `name.toLower()` — which is the same string!
It never compares against `columnNames[i].name`. The function matches on
the first iteration and always returns `ColumnId_Id` (index 0).

#### Impact

**Any column lookup by name returns the wrong column.** This affects:
- Table column header mapping
- Search/filter by column name
- Any feature that resolves a column name to its enum ID
- Potentially corrupts data if wrong column is used for read/write

#### Root Cause

A simple typo: `name` (the parameter) should have been `columnNames[i].name`.

#### Recommended Fix

```cpp
for (int i = 0; i < ColumnId_End; i++) {
    if (lower == columnNames[i].name.toLower())
        return columnNames[i].id;
}
```

**Estimated effort:** < 30 minutes

---

### H-06: Feature Flags Use Non-Power-of-2 Values

**Severity:** 🟠 HIGH
**File:** `src/model/world/baseidtable.hpp:13-18`
**Category:** Logic bug — flag overlap

#### Problem

```cpp
enum Features {
    Feature_Constant = 1,        // 0b001
    Feature_AllowTouch = 2,      // 0b010
    Feature_ViewId = 3           // 0b011 — NOT a unique flag!
};
```

`Feature_ViewId = 3` is the bitwise OR of `Feature_Constant | Feature_AllowTouch`.
Checking `if (features & Feature_ViewId)` returns true if either or both of the
other flags are set, making it impossible to distinguish.

#### Impact

- `Feature_ViewId` can never be checked independently
- Any code trying to detect the `ViewId` feature will get false positives
- If used in switch statements, the behavior is ambiguous

#### Recommended Fix

```cpp
enum Features {
    Feature_Constant = 1,        // 0b001
    Feature_AllowTouch = 2,      // 0b010
    Feature_ViewId = 4           // 0b100 — proper unique bit
};
```

**Estimated effort:** < 30 minutes (then audit all usage sites)

---

### H-07: `process.waitForFinished(-1)` UI Hang

**Severity:** 🟠 HIGH
**File:** `src/view/window/papyruscompiler.cpp:197,726`
**Category:** UX — unrecoverable hang

#### Problem

```cpp
if (!process.waitForFinished(-1)) {   // -1 = No timeout
    emit compilationError("Compilation timed out");   // NEVER REACHED
    return false;
}
```

With `-1` as timeout, `waitForFinished()` waits indefinitely. The error
message says "timed out" but this code path is unreachable. If `pp64.exe`
hangs (e.g., circular script dependency, infinite loop), the entire UI
freezes forever.

#### Impact

- **UI freezes permanently** if the compiler process hangs
- User must force-kill the application via Task Manager
- Potential unsaved work loss

#### Recommended Fix

```cpp
if (!process.waitForFinished(30000)) {   // 30 second timeout
    process.kill();
    emit compilationError("Compilation timed out after 30 seconds");
    return false;
}
```

Add a progress dialog or background thread for long compilations.

**Estimated effort:** 1-2 hours

---

## Medium Issues

### M-01: `OGG/Vorbis` Decoder Treats Vorbis as Raw PCM

**Severity:** 🟡 MEDIUM
**Files:**
- `src/libs/files/audio/oggdecoder.cpp:78-84,196-204`
- `src/libs/files/audio/oggencoder.cpp:27-39`
**Category:** Broken functionality — audio

#### Problem

The OGG decoder reads the Vorbis identification header to extract sample rate
and channels, then treats all subsequent page data as raw PCM samples. Vorbis
is a perceptual codec — the page data is not PCM. The decoder also silently
generates a sine wave if "no samples decoded" (which is always the case for
real Vorbis files).

The OGG encoder is a complete stub that logs an error and returns `false`.

#### Impact

- Attempting to play audio from OGG Vorbis files produces **noise, not audio**
- Encoding to OGG silently fails
- Only WAV format works correctly

#### Recommended Fix

Use a proper Vorbis decoding library (e.g., `libvorbisfile` via `stb_vorbis`
or a system library). Alternatively, use Qt Multimedia's `QSoundEffect` or
`QMediaPlayer` which handle OGG natively.

**Estimated effort:** 4-8 hours

---

### M-02: `DeleteRecordCommand` Name Collision

**Severity:** 🟡 MEDIUM
**Files:**
- `src/model/tools/deleterecordcommand.hpp`
- `src/model/tools/templetedeleterecordcommand.hpp`
**Category:** Maintainability — potential compile error

#### Problem

Two classes named `DeleteRecordCommand` exist:
1. Non-template version (uses `BaseCollection*`, calls `insertRecord()`)
2. Template version (uses `Collection<ESXRecord>*`, calls `add()`)

Any translation unit including both headers will get a redefinition error.
The template version also uses `add()` (upsert/append) for undo instead of
`insertRecord()` (restore at original position), breaking expected undo behavior.

#### Impact

- Potential compile error if both headers are ever included together
- Template version's undo restores record at wrong position

#### Recommended Fix

1. Rename template version to `TemplatedDeleteRecordCommand`
2. Fix template undo to use `insertRecord(*mDeletedRecord, mIndex)` instead of `add()`

**Estimated effort:** < 1 hour

---

### M-03: `searchalgorithm.cpp` — 23 Nearly Identical Lambda Functions

**Severity:** 🟡 MEDIUM
**File:** `src/model/tools/searchalgorithm.cpp:36-322`
**Category:** Maintainability — massive code duplication

#### Problem

Each record type has 2-3 dedicated lambda functions that do the same thing:
iterate records, build ID/FormID/name vectors. There are ~280 lines of
near-identical code. Adding a new record type requires adding 3 new functions.

#### Impact

- High maintenance burden for adding new record types
- Bug fixes must be replicated across all 23 functions
- ~280 lines could be reduced to ~40 with a template approach

#### Recommended Fix

Use a template or macro to generate the per-type data extraction functions:
```cpp
template<typename Collection>
auto getGenericData(Data* data, auto getCollection, auto getName) {
    // ... common logic ...
}
```

Or use a type-erased interface to avoid template bloat.

**Estimated effort:** 3-5 hours

---

### M-04: `saveRecord()` Validates AFTER Mutation

**Severity:** 🟡 MEDIUM
**Files:**
- `src/view/window/weaponeditor.cpp:148-178`
- `src/view/window/actieditor.cpp:119-123`
- `src/view/window/alch_editor.cpp:388-398`
- (Pattern likely in all editors)
**Category:** Data integrity — partial mutation on validation failure

#### Problem

```cpp
void WeaponEditor::saveRecord() {
    mRecord->editorId = mEditorIdEdit->text();   // MUTATE FIRST
    mRecord->fullName = mFullNameEdit->text();    // MUTATE FIRST
    // ... more mutations ...
    if (!validateWeapon()) { return; }             // VALIDATE LATER
    accept();
}
```

The record is mutated before validation. If validation fails, the record is
left in a partially modified state. The caller sees validation failure but
the damage is already done.

#### Impact

- Partially modified records on validation failure
- Silent data corruption if validation is the only guard

#### Recommended Fix

Validate first, then mutate:
```cpp
void WeaponEditor::saveRecord() {
    if (!validateWeapon()) { return; }              // VALIDATE FIRST
    mRecord->editorId = mEditorIdEdit->text();      // THEN MUTATE
    mRecord->fullName = mFullNameEdit->text();
    accept();
}
```

Or use a temporary copy and only commit if validation passes.

**Estimated effort:** 2-4 hours (across all editors)

---

### M-05: `MastersList` Ignores `parent` Parameter

**Severity:** 🟡 MEDIUM
**File:** `src/model/window/masterslist.cpp:3-5`
**Category:** Memory management — Qt ownership model

#### Problem

```cpp
MastersList::MastersList(QObject* parent) {
    // parent is completely ignored!
}
```

For `QAbstractTableModel` subclasses, passing `parent` to the base class is
critical for Qt's parent-child ownership model.

#### Impact

- The `MastersList` object is not automatically deleted when its parent is destroyed
- Potential memory leak if the caller relies on Qt ownership

#### Recommended Fix

```cpp
MastersList::MastersList(QObject* parent)
    : QAbstractTableModel(parent) {
}
```

**Estimated effort:** < 30 minutes (then check all models for same issue)

---

### M-06: `topologicalSort()` Ambiguous Return Value

**Severity:** 🟡 MEDIUM
**File:** `src/view/window/papyruscompiler.cpp:792-794`
**Category:** Error handling — ambiguous state

#### Problem

```cpp
if (sorted.size() != dependencyGraph.size()) {
    return QVector<QString>();  // Empty = cycle OR empty graph
}
```

Returning an empty vector on circular dependency makes it impossible to
distinguish "circular dependency detected" from "no dependencies to compile."
The caller's fragile check (`sorted.isEmpty() && !dependencyGraph.isEmpty()`)
attempts to disambiguate but could miss edge cases.

#### Impact

- Ambiguous error reporting (user sees generic "compilation failed" with no
  indication of circular dependencies)
- Potential silent failure if `dependencyGraph` is empty

#### Recommended Fix

Use a result type that encodes the failure reason:
```cpp
enum class SortResult { Success, CircularDependency };
struct CompileOrderResult {
    QVector<QString> order;
    SortResult result;
};
CompileOrderResult topologicalSort(const QVector<ScriptDependency>& deps);
```

**Estimated effort:** 1-2 hours

---

### M-07: `Data::createNewRecord()` Brute-Force O(n*m) FormID Search

**Severity:** 🟡 MEDIUM
**File:** `src/model/world/data.cpp:1033-1057`
**Category:** Performance — potential UI freeze

#### Problem

```cpp
for (quint32 formId = 0x800; formId < 0x1000; formId++) {
    // ...
    auto collections = allCollections();  // ALLOCATES 32 pointers on EVERY iteration
    for (auto* collection : collections) {
        if (collection->containsFormId(formId)) {  // May scan all records
            found = true;
            break;
        }
    }
}
```

`allCollections()` is called inside every loop iteration (up to 32768 times),
each time allocating a `QVector` of 32 pointers. Additionally, each
`containsFormId()` call may scan all records in each collection.

#### Impact

- **Lag spike** when creating a new record in a large plugin
- Excess memory allocations
- Gets worse as the plugin grows

#### Recommended Fix

Cache the collections and build a `QSet<quint32>` of all used FormIDs:

```cpp
auto collections = allCollections();
QSet<quint32> usedFormIds;
for (auto* collection : collections) {
    for (int i = 0; i < collection->getSize(); i++) {
        usedFormIds.insert(collection->getRecord(i).get().formId);
    }
}
for (quint32 formId = 0x800; formId < 0x1000; formId++) {
    if (!usedFormIds.contains(formId)) {
        return formId;
    }
}
```

**Estimated effort:** 2-4 hours

---

### M-08: `LandscapeEditCommand` Stores Unused Parameters

**Severity:** 🟡 MEDIUM
**File:** `src/model/tools/landscapeeditcommand.hpp:14-17,44-50`
**Category:** Code quality — dead parameters

#### Problem

```cpp
LandscapeEditCommand(QVector<float>* heightmap, int terrainSize,
                     const QVector<float>& originalData,
                     const QVector<float>& newData,
                     int centerX, int centerY, int radius)
```

`execute()` and `undo()` replace the **entire heightmap** (`*mHeightmap = mNewData`).
The `centerX`, `centerY`, and `radius` parameters are stored but never used.
The command was presumably intended for brush-stroke-level undo (partial
heightmap updates) but was implemented as full-heightmap replacement.

#### Impact

- Unused parameters clutter the interface
- Large heightmap copies (1M+ floats) on every brush stroke — performance issue
- Misleading API (suggests partial updates are supported)

#### Recommended Fix

Either:
1. Implement partial heightmap updates using center/radius
2. Or remove the unused parameters and rename to `FullHeightmapEditCommand`

**Estimated effort:** 2-3 hours

---

### M-09: CMakeLists.txt — Missing Qt5 Components, No Install Targets

**Severity:** 🟡 MEDIUM
**File:** `CMakeLists.txt:25,654,679`
**Category:** Build system — broken Qt5 support, no installation

#### Problems

1. **Qt5 path** (line 25) omits `Xml` and `OpenGLWidgets` components:
   ```cmake
   find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets 3DCore 3DRender 3DExtras 3DInput)
   ```
   Missing `Xml` (linked at line 143) and `OpenGLWidgets` (linked at line 582).

2. **No install targets** — `cmake --install` does nothing.

3. **Hardcoded Qt6 path** (line 679):
   ```cmake
   get_filename_component(QT6_INSTALL_PREFIX "${_qt6_loc}/../../.." ABSOLUTE)
   ```

4. **Editor.ini copied to wrong directory** (line 654) for multi-config generators.

#### Impact

- Qt5 build is broken
- Cannot install to system prefix
- Qt6 deployment fails on non-standard installations
- Editor.ini not found in build output for multi-config builds

#### Recommended Fix

1. Add `Xml OpenGLWidgets` to Qt5 `find_package`
2. Add `install(TARGETS ...)` directives
3. Use `$<TARGET_FILE_DIR:openck>` for editor.ini copy target
4. Add `CMAKE_CXX_VISIBILITY_PRESET hidden` for Linux/macOS

**Estimated effort:** 2-4 hours

---

### M-10: `Logger` Singleton Issues

**Severity:** 🟡 MEDIUM
**File:** `src/libs/files/log/logger.hpp:27-30,99-104`
**Category:** Concurrency — data race

#### Problem

- `m_initialized` (plain `bool`) is written in `init()` without holding `m_mutex`
- Global singleton mutable state accessed from anywhere via macros
- If any `LOG_*` call happens before `init()`, behavior is undefined
- `qInstallMessageHandler()` is called in `init()` but never restored on shutdown

#### Impact

- Potential data race on `m_initialized`
- Missing log messages if logging occurs during static initialization
- Leaked message handler on shutdown

#### Recommended Fix

1. Make `m_initialized` an `std::atomic<bool>` or protect with mutex
2. Add a guard in `log()` to handle uninitialized state gracefully
3. Restore previous message handler in destructor

**Estimated effort:** 1-2 hours

---

## Low Severity Issues (Summary)

| # | Issue | File(s) | Effort |
|---|-------|---------|--------|
| L01 | `const_cast` in `addrecordcommand.cpp:7` | `src/model/tools/` | < 1h |
| L02 | `constr_cast` in `deleterecordcommand.cpp:7` | `src/model/tools/` | < 1h |
| L03 | `findColummnIndex` typo (extra 'm') | `baseidtable.hpp:25`, `idtable.hpp:35`, `idtable.cpp:245` | < 30m |
| L04 | `CkId::CkId(const QString ckid)` takes by value | `ckid.hpp:60` | < 30m |
| L05 | `modificationEnums` has duplicate "Deleted" | `columns.cpp:57-60` | < 30m |
| L06 | Commented-out code in `collection.hpp:222-227` | `collection.hpp` | < 15m |
| L07 | Unused `CkId log` variable | `loader.cpp:85` | < 15m |
| L08 | `#include <memory>` and `<type_traits>` unused | `objectpalette.hpp:5-6`, `landscapeeditor.hpp:5` | < 15m |
| L09 | Redundant `QString::fromUtf8(editorId.toUtf8())` | `objectpalette.cpp:226` | < 15m |
| L10 | `dynamic_cast` should be `qobject_cast` | `enumdelegate.cpp:65,128` | < 30m |
| L11 | `setSuffix("")` no-op | `weaponeditor.cpp:109` | < 15m |
| L12 | Stringstream instead of `QString::arg()` | `loader.cpp:24-26,66-69` | < 30m |
| L13 | `NOMINMAX` defined in source file | `genericdelegate.cpp:1` | < 15m |
| L14 | `PapyrusDebugger` include guard typo (`PYPARUS`) | `papyrusdebugger.hpp:1-2` | < 15m |
| L15 | `MastersList` constructor ignores `parent` | `masterslist.cpp:3-5` | < 15m |
| L16 | `Unused addMessage()` / `loadMessage()` stubs | `loader.hpp:28,65` | < 15m |
| L17 | `toUInt()` without error checking | Multiple editor files | 2-3h |
| L18 | `saveRecord()` validates after mutation pattern | All editors | 2-4h |
| L19 | Re-entrant `destroyed()` signals not disconnected | `papyruscompiler.cpp:15-36` | < 1h |
| L20 | `Q_UNUSED` with over 30 unused params | ~30 files | 1-2h |
| L21 | `union`/`memcpy` in `strings.cpp` for header parse | `strings.cpp` | < 1h |
| L22 | `record.hpp` `clone()` returns raw owning pointer | `record.hpp:78-87` | < 30m |
| L23 | `delegatefactory.hpp` missing virtual destructor | `delegatefactory.hpp:10-13` | < 15m |
| L24 | Inconsistent include style (relative vs bare) | Multiple files | 1-2h |
| L25 | `std::numeric_limits<char>` platform-dependent | `genericdelegate.cpp:129` | < 15m |
| L26 | `TableModelHelper` broken/unused class | `genericdelegate.hpp:9-26` | < 30m |

---

## Changes to `TECHNICAL_DEBT.md`

When Stage 2 begins, add the following items:

### PHASE 2 CODE QUALITY ITEMS

**C-01: ESM subrecord names don't match TES format** (CRITICAL)
- Files: `src/libs/files/esm/*record.cpp`
- Issue: Record types use invented subrecord names (`FORM`, `ITM2`, `ODIT`)
  that don't exist in any Bethesda plugin format. Reader/writer produces
  corrupt plugin files.
- Fix: Rewrite all record load/save methods with correct subrecord names
  from UESP/xEdit documentation.

**C-02: RecordEditCommand::execute() is a no-op** (CRITICAL)
- File: `src/model/tools/recordeditcommand.hpp`
- Issue: execute() captures current state but never applies a new value.
  The command does nothing on execution.
- Fix: Apply externally-provided new state in execute().

**H-01: Systemic const_cast abuse (200+ occurrences)** (HIGH)
- Files: `src/view/window/*`, `src/model/world/data.cpp`
- Issue: const_cast used everywhere because Data getters return const
  references but editors need mutation.
- Fix: Add non-const overloads to Data class, remove all const_cast calls.

**H-02: OpenGL VBO double-allocation** (HIGH)
- File: `src/view/window/landscapeeditor.cpp`
- Issue: vbo.allocate() called twice, second call invalidates first.
  Terrain rendering is broken.
- Fix: Use separate VBOs for vertices and normals.

**H-03: QSpinBox for float positions** (HIGH)
- File: `src/view/window/ref_editor.cpp`
- Issue: Integer spinbox truncates float position data silently.
- Fix: Replace with QDoubleSpinBox.

**H-04: QDataStream byte-order mismatch** (HIGH)
- File: `src/view/window/objectpalette.cpp`
- Issue: Save in BigEndian, load in LittleEndian. Files are unreadable.
- Fix: Add setByteOrder to save function.

**H-05: getColumnId() always returns wrong value** (HIGH)
- File: `src/model/world/columns.cpp`
- Issue: Comparison against self instead of columnNames[i].name.
- Fix: Correct the comparison target.

**H-06: Feature flags use non-power-of-2** (HIGH)
- File: `src/model/world/baseidtable.hpp`
- Issue: Feature_ViewId = 3 which overlaps with Feature_Constant|AllowTouch.
- Fix: Change to Feature_ViewId = 4.

**H-07: process.waitForFinished(-1) UI hang** (HIGH)
- File: `src/view/window/papyruscompiler.cpp`
- Issue: No timeout, UI hangs forever if compiler hangs.
- Fix: Add 30-second timeout and process.kill() on timeout.

**M-01: OGG/Vorbis decoder produces garbage** (MEDIUM)
- File: `src/libs/files/audio/oggdecoder.cpp`
- Issue: Treats Vorbis compressed data as raw PCM.
- Fix: Use proper Vorbis decoding library.

---

## Verification Plan

For each completed fix:

1. **Build check**: `cmake --build build --config Debug --target OpenCK` must succeed
2. **Test run**: `cmake --build build --config Debug --target RUN_TESTS` — all tests pass
   (except pre-existing `test_thememanager` failure)
3. **Regression check**: Verify adjacent code not broken by reading the diff
4. **Static analysis**: Scan for reintroduction of the same pattern

### CI/Pre-commit Hooks to Consider

- `clang-tidy` check for `const_cast` usage
- `cppcheck` for unused parameters and variables
- Regular expression check for no-op patterns

---

## Appendix: Review Methodology

The code review was performed by three parallel AI exploration agents:

- **Agent 1** (Model layer): `src/model/` — collections, commands, undo stack,
  data, tools
- **Agent 2** (View layer): `src/view/` — windows, editors, widgets, delegates
- **Agent 3** (ESM & Tests): `src/libs/files/esm/`, `tests/`, `CMakeLists.txt`

Each agent performed a thorough read of every `.hpp` and `.cpp` file in its
scope, looking for:
- Redundant/dead code
- Bad patterns and anti-patterns
- Incomplete implementations
- Include issues
- Memory/performance problems
- Missing validation
- Naming/efficiency issues

Findings were cross-referenced to eliminate duplicates and prioritized by
severity and impact.
