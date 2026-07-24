# OpenCK Implementation Plan

## Phase 1: Wire Up Record Loading and Saving

This document provides the detailed task breakdown for Phase 1 — making the existing record parsers actually functional by wiring them into the data dispatch system.

### Current State

The `Data::continueLoading()` method in `src/model/world/data.cpp` only handles `GMST` records:

```cpp
switch (name)
{
case 'GMST': gameSettings.load(*reader, base);    break;
default:
    messages.append(CkId::Type_LoadingLog, "Unknown record encountered!");
    reader->skipRecord();
    break;
}
```

The `Document::save()` method only writes the TES4 header:
```cpp
void Document::save(const QString& savePath)
{
    ESMWriter writer;
    QFile saveFile{ savePath };
    if (saveFile.open(QIODevice::WriteOnly))
    {
        writer.save(saveFile);
    }
}
```

### Task Breakdown

#### Task 1.1: Define CkId::Type enum for all record types

**File**: `src/model/world/ckid.hpp`

Add entries to the `CkId::Type` enum for each record type we need to support:

```cpp
enum Type
{
    Type_None = 0,
    Type_Gmst,
    Type_LoadingLog,
    Type_RunLog,
    Type_Npc_,          // NPCs
    Type_Weap_,         // Weapons
    Type_Armor_,        // Armor
    Type_Spel_,         // Spells
    Type_Magic_,        // Magic Effects
    Type_Quest_,        // Quests
    Type_Dial_,         // Dialogue
    Type_Info_,         // Dialogue Information
    Type_Glob_,         // Global Variables
    Type_Lcrt_,         // Location References
    Type_Pack_,         // AI Packages
    Type_Tree_,         // Trees
    Type_Alch_,         // Alchemy
    Type_Ingr_,         // Ingredients
    Type_Cont_,         // Containers
    Type_Ench_,         // Enchantments
    Type_Book_,         // Books
    Type_Misc_,         // Miscellaneous Items
    Type_Acti_,         // Activators
    Type_Stat_,         // Static Objects
    Type_Race_,         // Races
    Type_Class_,        // Classes
    Type_Fact_,         // Factions
    Type_PerK_,         // Perks
    Type_Soun_,         // Sounds
    Type_Wthr_,         // Weather
    Type_Ltex_,         // Land Textures
    Type_Land_,         // Land
    Type_End            // Sentinel
};
```

#### Task 1.2: Add record collections to Data class

**File**: `src/model/world/data.hpp` and `data.cpp`

Add member variables for each record collection:

```cpp
// Data class members
IdCollection<GameSetting> gameSettings;
IdCollection<Npc_> npcCollection;
IdCollection<Weapon> weaponCollection;
IdCollection<Armor> armorCollection;
// ... etc for all record types
```

In `Data` constructor, initialize each collection with columns and register with model:

```cpp
Data::Data(const QStringList& files, const FilePaths& paths)
{
    // GMST (already implemented)
    gameSettings.addColumn(new StringIdColumn<GameSetting>());
    gameSettings.addColumn(new RecordStateColumn<GameSetting>());
    gameSettings.addColumn(new VarTypeColumn<GameSetting>(BaseColumn::Display_GmstVarType));
    gameSettings.addColumn(new VarValueColumn<GameSetting>());
    addModel(new IdTable(&gameSettings), CkId::Type_Gmst);

    // NPC_
    npcCollection.addColumn(new StringIdColumn<Npc_>());
    npcCollection.addColumn(new RecordStateColumn<Npc_>());
    npcCollection.addColumn(new RecordTypeColumn<Npc_>());
    addModel(new IdTable(&npcCollection), CkId::Type_Npc_);

    // ... repeat for all record types
}
```

#### Task 1.3: Wire up record dispatch in Data::continueLoading()

**File**: `src/model/world/data.cpp`

Replace the switch statement in `continueLoading()` with a dispatch for each record type:

```cpp
bool Data::continueLoading(Messages& messages)
{
    if (!reader->isLeft())
        return true;

    NAME name = reader->readName();

    switch (name)
    {
    case 'GRUP': reader->skipGrupHeader();              break;
    case 'GMST': gameSettings.load(*reader, base);      break;
    case 'NPC_': npcCollection.load(*reader, base);     break;
    case 'WEAP': weaponCollection.load(*reader, base);  break;
    case 'ARMOR': armorCollection.load(*reader, base);  break;
    case 'SPEL': spellCollection.load(*reader, base);   break;
    case 'MAGIC': magicCollection.load(*reader, base);  break;
    case 'QUEST': questCollection.load(*reader, base);  break;
    case 'DIAL': dialCollection.load(*reader, base);    break;
    case 'INFO': infoCollection.load(*reader, base);    break;
    case 'GLOB': globCollection.load(*reader, base);    break;
    case 'LCRT': lcrtCollection.load(*reader, base);    break;
    case 'PACK': packCollection.load(*reader, base);    break;
    case 'TREE': treeCollection.load(*reader, base);    break;
    case 'ALCH': alchCollection.load(*reader, base);    break;
    case 'INGR': ingrCollection.load(*reader, base);    break;
    case 'CONT': contCollection.load(*reader, base);    break;
    case 'ENCH': enchCollection.load(*reader, base);    break;
    case 'BOOK': bookCollection.load(*reader, base);    break;
    case 'MISC': miscCollection.load(*reader, base);    break;
    case 'ACTI': actiCollection.load(*reader, base);    break;
    case 'STAT': statCollection.load(*reader, base);    break;
    case 'RACE': raceCollection.load(*reader, base);    break;
    case 'CLASS': classCollection.load(*reader, base);  break;
    case 'FACT': factCollection.load(*reader, base);    break;
    case 'PERK': perkCollection.load(*reader, base);    break;
    case 'SOUN': sounCollection.load(*reader, base);    break;
    case 'WTHR': wthrCollection.load(*reader, base);    break;
    case 'LTEX': ltexCollection.load(*reader, base);    break;
    case 'LAND': landCollection.load(*reader, base);    break;
    default:
        messages.append(CkId::Type_LoadingLog, "Unknown record encountered!");
        reader->skipRecord();
        break;
    }

    return false;
}
```

#### Task 1.4: Implement Document::save()

**File**: `src/model/doc/document.cpp`

Implement the save method to iterate all collections and write modified records:

```cpp
void Document::save(const QString& savePath)
{
    ESMWriter writer;

    QFile saveFile{ savePath };
    if (!saveFile.open(QIODevice::WriteOnly))
        return;

    writer.save(saveFile);

    // Iterate all collections and write modified records
    // For each collection:
    //   - Iterate all records
    //   - If record is Modified or ModifiedOnly:
    //     - writer.startRecord(NAME, header)
    //     - record.save(writer)
    //     - writer.endRecord()
}
```

#### Task 1.5: Add model wiring in Data constructor

**File**: `src/model/world/data.cpp`

After adding collection members, initialize each with columns and register with models:

```cpp
Data::Data(const QStringList& files, const FilePaths& paths)
    : contentFiles(files), paths(paths)
{
    // Initialize all collections with columns
    initCollections();

    // Register all models
    registerModels();
}
```

#### Task 1.6: Add record NAME constants

**File**: `src/model/world/ckid.hpp` or new file

Add static methods or constants for mapping 4CC names to CkId::Type:

```cpp
struct RecordNameMapping
{
    static NAME toName(CkId::Type type);
    static CkId::Type toType(NAME name);
};
```

### Testing Plan

1. **Load test**: Open a Skyrim Special Edition.esm file, verify all NPC_, WEAP_, ARMOR_, etc. records appear in the UI
2. **Edit test**: Modify a record value in the UI, verify it changes
3. **Save test**: Save the modified plugin, verify it opens in a hex editor with the correct data
4. **Roundtrip test**: Load the saved file, verify all data is preserved

### Risk Assessment

- **Risk**: Record parsers may have bugs from the original codebase (fix_braces.py scripts existed)
- **Mitigation**: Test with known-good ESM files first
- **Risk**: ESMWriter may not handle all subrecord types correctly
- **Mitigation**: Verify writer output with hex editor against known-good output
- **Risk**: Memory usage for large plugins (Skyrim.esm has 100k+ records)
- **Mitigation**: Profile memory usage; may need lazy loading later

### Success Criteria

- [ ] All 22 record types load from ESM/ESP files
- [ ] Modified records appear in UI tables
- [ ] Save produces valid ESM/ESP files
- [ ] Roundtrip load/save preserves data integrity
- [ ] No crashes when loading large plugins (100k+ records)
