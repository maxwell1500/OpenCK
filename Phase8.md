# Phase 8: Remaining Issues Fix Plan (8-12 hours)

## Issue 1: ESM Subrecord Names Are Custom (Estimated: 3-4 hours)

**Problem**: All subrecord names (FORM, LVLL, MCKA, ATKC, etc.) are invented. Files can't be read by the real CK, and real CK files can't be read by OpenCK.

**Strategy**: Map existing custom codes to real TES4 codes. Keep the internal data model the same, but translate at the serialization boundary.

### Step 8.1.1: Create TES4 subrecord code mapping table
- Create `libs/files/esm/tes4codes.hpp` with a mapping from custom codes to real TES4 codes:
```cpp
namespace Tes4Codes {
    // NPC_ subrecords
    constexpr quint32 NPC_FORM = 'ACBS';  // Configuration
    constexpr quint32 NPC_FULL = 'FULL';  // Display name
    constexpr quint32 NPC_SPLO = 'SPLO';  // Spells
    constexpr quint32 NPC_DOFT = 'DOFT';  // Default outfit
    // ... etc for all record types
}
```

### Step 8.1.2: Update ESMReader to translate codes on read
- In `readNSubHeader()`, after reading the 4-byte code, translate it:
```cpp
NAME translateCode(NAME rawCode) {
    // Map real TES4 codes to our internal codes
    switch (rawCode) {
        case 'ACBS': return 'FORM';  // NPC config → our internal FORM
        case 'FULL': return 'NAME';  // Display name → our internal NAME
        // ... etc
        default: return rawCode;
    }
}
```

### Step 8.1.3: Update ESMWriter to translate codes on write
- In `startSubRecord()`, translate internal codes back to real TES4 codes:
```cpp
NAME translateCodeForWrite(NAME internalCode) {
    switch (internalCode) {
        case 'FORM': return 'ACBS';
        case 'NAME': return 'FULL';
        // ... etc
        default: return internalCode;
    }
}
```

### Step 8.1.4: Update record parse/save methods
- Each record type's load() and save() methods use the internal codes
- The translation happens at the ESMReader/ESMWriter level
- No changes needed to individual record types

### Step 8.1.5: Build verification

---

## Issue 2: No Undo Support in Any Editor (Estimated: 2-3 hours)

**Problem**: All editors mutate records directly. No way to undo changes.

**Strategy**: Editors work on copies. On accept, create EditRecordCommand with old/new state.

### Step 8.2.1: Create base editor undo pattern
- Create a template or base class that all editors can use:
```cpp
template<typename RecordType>
class EditorUndoHelper {
public:
    static void commitEdit(Data* data, int recordIndex, const RecordType& newState) {
        auto& collection = data->getCollection<RecordType>();
        auto oldState = collection.getRecord(recordIndex); // copy
        auto* cmd = new EditRecordCommand<RecordType>(data, recordIndex, oldState, newState);
        data->getUndoStack()->push(cmd);
    }
};
```

### Step 8.2.2: Apply to NPC editor
- In NpcEditor::accept(), instead of directly modifying mRecord:
  1. Create a copy of the modified record
  2. Call EditorUndoHelper::commitEdit()
  3. The undo stack handles the actual mutation

### Step 8.2.3: Apply to all other editors
- Repeat for: Weapon, Armor, Spell, Alch, Ench, Book, Container, Ingredient, Misc, Activator, Class, Faction, Race, Perk, Quest, Cell, Worldspace, Location, Dialogue, Package

### Step 8.2.4: Build verification

---

## Issue 3: ~10 Editors Missing ColumnValidator (Estimated: 1 hour)

**Problem**: Only 6 of 16+ editors use ColumnValidator. Others have no validation.

**Step 8.3.1: Identify editors without validation**
- BookEditor, ContEditor, IngrEditor, MiscEditor, ActiEditor, ClassEditor, FactEditor, RaceEditor, PerkEditor, QuestEditor, CellEditor, WorldspaceEditor, LocationEditor, DialEditor, PackEditor

### Step 8.3.2: Add validation calls to each
- For each editor, add `#include "columnvalidator.hpp"` and call the appropriate validate method before accept
- If a validate method doesn't exist for the type, add one to ColumnValidator

### Step 8.3.3: Build verification

---

## Issue 4: PerkEditor/PackEditor Destructive Resize Stubs (Estimated: 1 hour)

**Problem**: Spin boxes that show array counts resize on save, destroying data.

### Step 8.4.1: Fix PerkEditor
- Remove the conditions spin box or make it read-only
- If the user needs to edit conditions, add a proper condition list editor (QTableWidget with add/remove)
- For now, make it display-only with a "Conditions: N (read-only)" label

### Step 8.4.2: Fix PackEditor
- Same approach: make targetIds/parameters spin boxes read-only
- Add a note that these are managed by the quest system

### Step 8.4.3: Build verification

---

## Issue 5: DialEditor onTopicSelected Empty Stub (Estimated: 1 hour)

**Problem**: Clicking a topic in the dialogue editor does nothing.

### Step 8.5.1: Implement topic detail display
- When a topic is selected, show its responses in a detail panel
- Display: response text, emotion type, speaker conditions
- Allow editing response text
- Wire the edited data back to the DialRecord

### Step 8.5.2: Add response editor
- Add a QTextEdit for response text
- Add a combo for emotion type
- Add a conditions table (simplified — just show condition count for now)

### Step 8.5.3: Build verification

---

## Issue 6: Compressed BA2 Entries Not Supported (Estimated: 1-2 hours)

**Problem**: Most real-world BA2 archives use compression. Currently skipped.

### Step 8.6.1: Add zlib decompression
- Include zlib (Qt has it built-in via Qt5::Zlib or system zlib)
- In Ba2Archive::extract(), when entry.compressed:
  1. Read compressed data from mapped pointer
  2. Decompress with `uncompress()` or `inflate()`
  3. Write decompressed data to file

### Step 8.6.2: Handle compression flags
- BA2 uses a flags field to indicate compression type (zlib, lz4, etc.)
- Start with zlib support (most common)
- Log unsupported compression types

### Step 8.6.3: Build verification

---

## Issue 7: OverlayVBO Uses Deprecated GL (Estimated: 2-3 hours)

**Problem**: glVertexPointer/glEnableClientState are deprecated in OpenGL 3.2+ core profile.

### Step 8.7.1: Create overlay shader
- Simple vertex shader: `gl_Position = mvp * vec4(aPos, 1.0);`
- Simple fragment shader: `gl_FragColor = vColor;`
- Compile and cache the shader program

### Step 8.7.2: Update OverlayVBO::draw()
- Replace glVertexPointer/glEnableClientState with shader-based rendering
- Use the same pattern as the main mesh renderer (VAO/VBO + shader)
- Support per-vertex color via vertex attribute

### Step 8.7.3: Update all overlay renderers
- Grid, axis, navmesh, bounding boxes, collision shapes, cell grid, cell references
- All should use the new overlay shader

### Step 8.7.4: Build verification

---

## Iteration Schedule

| Iteration | Steps | Gate |
|-----------|-------|------|
| **Iter 1** | 8.1 (TES4 codes) + 8.6 (compressed BA2) | Build passes, can read real TES4 files |
| **Iter 2** | 8.2 (undo) + 8.3 (validation) | Build passes, all editors undoable + validated |
| **Iter 3** | 8.4 (destructive stubs) + 8.5 (dialogue) | Build passes |
| **Iter 4** | 8.7 (deprecated GL) | Build passes, overlays use modern GL |
| **Iter 5** | Final audit | Zero stubs/TODOs/dead code |
