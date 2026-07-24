# UndoStack Integration & Validation Completion Plan

## Current State Assessment

| Metric | Status |
|--------|--------|
| Record types with editors | 30/30 (complete) |
| Editors with validation | 4/30 (Npc, Weapon, Quest, Fact) |
| Editors with UndoStack | 0/30 |
| Call sites to modify | 2 (ObjectWindowDialog::editSelected, SearchDialog::openEditorForResult) |
| RecordEditCommand usability | **Broken** for direct-edit workflow |

## Root Problem

`RecordEditCommand` captures the original state in its constructor and reads the new state from the Collection during `execute()`. But editors modify records **directly via raw pointer** (not through the command), so by the time `execute()` is called:
- The record has already been modified
- There's no way to distinguish "original" from "new"
- Undo would restore the already-modified state

```
Current broken flow:
  RecordEditCommand constructed -> captures state A (but editor hasn't opened yet)
  Editor opens, user modifies record -> state becomes B
  Editor closes, execute() called -> reads state B from collection
  Undo called -> restores state B (same as current!) <- BROKEN
```

## Architecture Decision

**Create `EditRecordCommand<ESXRecord>`** - a separate command type that explicitly receives both before/after states, designed specifically for the editor workflow.

```
New correct flow:
  Capture original state A from collection
  Editor opens, user modifies record -> state becomes B
  Editor closes, EditRecordCommand(A, B) constructed
  Command pushed to UndoStack
  Undo called -> restores state A <- CORRECT
```

---

## Phase 1: UndoStack Infrastructure (2 edits)

### 1.1 Create `EditRecordCommand` template

**File:** `src/model/tools/editrecordcommand.hpp` (new file)

```cpp
template<typename ESXRecord>
class EditRecordCommand : public Command
{
public:
    EditRecordCommand(Collection<ESXRecord>& collection, int index,
                      const ESXRecord& originalState, const ESXRecord& newState,
                      const QString& description = QString());
    
    void execute() override;  // Apply newState to collection
    void undo() override;     // Restore originalState to collection
    QString name() const override;

private:
    Collection<ESXRecord>& mCollection;
    int mIndex;
    ESXRecord mOriginalState;
    ESXRecord mNewState;
    QString mName;
};
```

- `execute()`: calls `collection.getRecord(mIndex).setModified(mNewState)`
- `undo()`: calls `collection.getRecord(mIndex).setModified(mOriginalState)`
- Uses existing `Record<ESXRecord>::setModified()` which handles state transitions (Base->Modified, etc.)
- Add to `CMakeLists.txt` in MODEL_HEADERS

### 1.2 Update CMakeLists.txt

Add `editrecordcommand.hpp` to `MODEL_HEADERS` list alongside `recordeditcommand.hpp`.

---

## Phase 2: Editor Integration at Call Sites (2 files modified)

**Key insight:** All 30 editors follow an identical pattern at their call sites. We only need to modify **2 locations** to enable UndoStack for all editors.

### 2.1 Modify `ObjectWindowDialog::editSelected()`

**File:** `src/view/window/objectwindowdialog.cpp` (~30 switch cases)

**Before (current):**
```cpp
case CkId::Type_Npc_:
{
    auto& collection = const_cast<IdCollection<NpcRecord>&>(mData->getNpcCollection());
    if (recordIndex >= 0 && recordIndex < collection.size())
    {
        NpcRecord* npc = const_cast<NpcRecord*>(&collection.getRecord(recordIndex).get());
        NpcEditor editor(mData, npc, this);
        if (editor.exec() == QDialog::Accepted)
        {
            LOG_INFO(QString("NPC '%1' edited").arg(editorId));
        }
    }
    break;
}
```

**After (new):**
```cpp
case CkId::Type_Npc_:
{
    auto& collection = const_cast<IdCollection<NpcRecord>&>(mData->getNpcCollection());
    if (recordIndex >= 0 && recordIndex < collection.size())
    {
        NpcRecord* npc = const_cast<NpcRecord*>(&collection.getRecord(recordIndex).get());
        NpcRecord originalState = npc->get();  // Capture BEFORE editor opens
        
        NpcEditor editor(mData, npc, this);
        if (editor.exec() == QDialog::Accepted)
        {
            auto& coll = const_cast<IdCollection<NpcRecord>&>(mData->getNpcCollection());
            int idx = coll.searchId(npc->editorId);
            if (idx >= 0 && mData->getUndoStack())
            {
                EditRecordCommand<NpcRecord>* cmd = new EditRecordCommand<NpcRecord>(
                    coll, idx, originalState, *npc,
                    "Edit NPC: " + npc->editorId);
                mData->getUndoStack()->push(cmd);
            }
            LOG_INFO(QString("NPC '%1' edited").arg(editorId));
        }
    }
    break;
}
```

**Changes per case:**
1. Add `RecordType originalState = record->get();` before editor creation
2. After `editor.exec() == QDialog::Accepted`, create and push `EditRecordCommand`

### 2.2 Modify `SearchDialog::openEditorForResult()`

**File:** `src/view/window/searchdialog.cpp` (~28 switch cases)

Same transformation as above. Same pattern, same 2-line addition per case.

### 2.3 Special Cases

**DialEditor / InfoEditor** - These use a different pattern (`loadRecord()` method, no Data* in constructor). They need special handling:
- Option A: Refactor to standard `Data*, Record*` constructor pattern (cleaner but more changes)
- Option B: Handle them separately with direct collection access at call site

**MaterialEditor** - Called from MainWindow directly, not through ObjectWindowDialog. Needs its own integration.

---

## Phase 3: Validation for Remaining 26 Editors (~26 files)

Group editors by validation complexity to batch work efficiently.

### Group A: Simple Required Fields (8 editors)

| Editor | Record Type | Validation Rules |
|--------|-------------|-----------------|
| ActiEditor | ActiRecord | EditorID not empty, unique |
| BookEditor | BookRecord | EditorID not empty, unique; Name not empty |
| ClassEditor | ClassRecord | EditorID not empty, unique; Name not empty |
| EnchEditor | EnchRecord | EditorID not empty, unique |
| GlobVarEditor | GlobalVariable | EditorID not empty, unique; Name not empty |
| IngrEditor | IngrRecord | EditorID not empty, unique; Name not empty |
| MiscEditor | MiscRecord | EditorID not empty, unique; Name not empty |
| StatEditor | StatRecord | EditorID not empty, unique |

### Group B: Numeric Range Validation (8 editors)

| Editor | Record Type | Additional Rules |
|--------|-------------|-----------------|
| AlchEditor | AlchRecord | EditorID + Value 0-99999, Weight 0-999 |
| ContEditor | ContRecord | EditorID + Value 0-9999999 |
| LcrtEditor | LocationRefType | EditorID + numeric ranges for any float fields |
| MagicEditor | MagicRecord | EditorID + magnitude/area ranges |
| PackEditor | PackageRecord | EditorID + AI package validation |
| RaceEditor | RaceRecord | EditorID + name required |
| TreeEditor | TreeRecord | EditorID + numeric ranges |
| WorldspaceEditor | WorldspaceRecord | EditorID + name required |

### Group C: Cross-Reference Validation (6 editors)

| Editor | Record Type | Additional Rules |
|--------|-------------|-----------------|
| ArmorEditor | ArmorRecord | EditorID + value/weight ranges + DT 0-9999 |
| CellEditor | CellRecord | EditorID + formID validation for references |
| FactEditor | FactRecord | Already done |
| PerkEditor | PerkRecord | EditorID + level/stage validation |
| RefEditor | RefrRecord | EditorID + reference target validation |
| SpellEditor | SpellRecord | EditorID + magic school/effects validation |

### Group D: Complex/Tree Editors (4 editors)

| Editor | Record Type | Notes |
|--------|-------------|-------|
| DialEditor | DialRecord | Needs refactoring (different constructor pattern) |
| InfoEditor | InfoRecord | Needs refactoring (different constructor pattern) |
| QuestGraphEditor | QuestRecord | Tree-based, may need different approach |
| AIPackageEditor | PackageRecord | Tree-based, may need different approach |

---

## Phase 4: Testing & Documentation (1 file)

### 4.1 Update ROADMAP.md / TECHNICAL_DEBT.md

Mark completed items:
- X-01: UndoStack integration for editor changes
- X-02: Validation on record field ranges (all 30 editors)

### 4.2 Manual Test Plan

For each of the 4 priority editors (Npc, Weapon, Quest, Fact):
1. Open editor, modify fields, save -> verify undo works (Ctrl+Z restores original)
2. Modify fields, save -> verify redo works (Ctrl+Y restores modified)
3. Trigger validation errors -> verify dialog blocks save
4. Multiple edits -> verify undo stack depth works correctly

---

## File Change Summary

| Action | Files | Effort |
|--------|-------|--------|
| **New file** | `editrecordcommand.hpp` | 1 file, ~60 lines |
| **Modified** | `CMakeLists.txt` | 1 line |
| **Modified** | `objectwindowdialog.cpp` | ~30 cases, +2 lines each = ~60 lines added |
| **Modified** | `searchdialog.cpp` | ~28 cases, +2 lines each = ~56 lines added |
| **Modified** | 26 editor .hpp files | Add `bool validate*()` declaration |
| **Modified** | 26 editor .cpp files | Add `validate*()` implementation |
| **Total** | ~30 files | ~250 lines of new code, ~120 lines modified |

---

## Risk Mitigation

| Risk | Mitigation |
|------|-----------|
| EditRecordCommand breaks existing RecordEditCommand usage | They're separate classes; RecordEditCommand unchanged |
| const_cast needed for collection access | Already used throughout codebase; consistent pattern |
| DialEditor/InfoEditor refactoring scope | Defer to Phase 3D; they're rarely used |
| Validation false positives | Keep validation rules conservative; can be refined later |
| Undo stack memory growth | Existing UndoStack has maxDepth=100; unchanged |

---

## Execution Order

1. **Phase 1** (2 edits) -> Build & verify compiles
2. **Phase 2** (2 files) -> Build & verify compiles, manual test undo on Npc/Weapon
3. **Phase 3** (26 editors) -> Batch by Group A->B->C->D, build after each group
4. **Phase 4** (docs) -> Final build, update documentation
