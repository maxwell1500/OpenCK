# OpenCK Architecture

## Overview

OpenCK is a C++/Qt application designed as an open-source replacement for Bethesda's Creation Kit. The codebase follows an MVC/MVP architecture inspired by OpenCS (OpenMW project), with clear separation between model, view, and file I/O layers.

## Directory Structure

```
openck/
├── libs/
│   ├── files/                    # File I/O layer (platform-independent)
│   │   ├── esm/                  # ESM/ESP binary reader/writer and record parsers
│   │   ├── data/                 # Utility modules (strings, etc.)
│   │   └── files.hpp             # File path management
│   └── files/data/strings.cpp    # String handling utilities
├── src/
│   ├── model/                    # Domain logic (platform-independent)
│   │   ├── doc/                  # Document management (open/save/load lifecycle)
│   │   ├── tools/                # Utility tools (reports)
│   │   └── world/                # World data model (collections, tables, columns)
│   └── view/                     # Qt UI layer
│       ├── doc/                  # Document loader UI
│       ├── world/                # World-specific delegates (enum, generic, variant)
│       └── window/               # Dialogs and main window
├── ui/                           # Qt Designer .ui files
├── build/                        # CMake build output
└── docs/                         # Documentation
```

## Architecture Layers

### 1. File I/O Layer (`libs/files/esm/`)

The lowest-level layer handles binary ESM/ESP file reading and writing.

**Key Components:**

| File | Purpose |
|------|---------|
| `esmreader.hpp/cpp` | Binary file reader for ESM/ESP files |
| `esmwriter.hpp/cpp` | Binary file writer for ESM/ESP files |
| `esmfile.hpp` | Low-level file abstraction |
| `records.hpp` | RecHeader, Flags, FormID types |
| `tes4.hpp` | TES4 header structure (HEDR, MAST, INTV, INCC) |
| `gmst.hpp/cpp` | GameSetting record type |
| `npcrecord.hpp/cpp` | NPC record type |
| `weaprecord.hpp/cpp` | Weapon record type |
| ... (22 record types) | |

**Reader Interface:**
```cpp
ESMReader reader(path);
reader.open();
NAME name = reader.readName();      // Read record/subrecord name (4CC)
bool hasLeft = reader.isLeft();      // Check if more records exist
reader.skipRecord();                 // Skip unknown record
header = reader.readHeader();        // Read RecHeader
```

**Writer Interface:**
```cpp
ESMWriter writer;
writer.save(file);                    // Write to file
writer.startRecord(NAME, RecHeader);  // Begin record
writer.endRecord();                   // End record
writer.writeType(data);               // Write raw data
writer.writeZString(str);             // Write null-terminated string
```

**Record Pattern:**
Every record type implements a consistent interface:
```cpp
struct RecordType
{
    QString id;                       // EDID - unique identifier
    void load(ESMReader& esm);        // Binary parsing
    void save(ESMWriter& esm) const;  // Binary serialization
    void blank();                     // Initialize with defaults
};
```

### 2. Domain Model Layer (`src/model/`)

The domain model manages document lifecycle, data collections, and business rules.

**Document Management (`src/model/doc/`):**

| File | Purpose |
|------|---------|
| `document.hpp/cpp` | Single plugin document (open/edit/save lifecycle) |
| `documentmediator.hpp/cpp` | Manages multiple open documents |
| `loader.hpp/cpp` | Multi-threaded background file loading |
| `messages.hpp/cpp` | Loading progress/error messages |

**World Data Model (`src/model/world/`):**

| File | Purpose |
|------|---------|
| `data.hpp/cpp` | Central dispatch hub - reads ESM and dispatches to collections |
| `collection.hpp` | Template collection container for typed records |
| `idcollection.hpp` | Collection with ID-based indexing |
| `idtable.hpp/cpp` | Bridges Collection<> to QAbstractItemModel (Qt Model/View) |
| `record.hpp` | Record wrapper with state management (Base/Modified/Deleted) |
| `basecollection.hpp` | Abstract base for all collections |
| `basecolumn.hpp` | Column definitions for record editing |
| `columns.hpp` | ColumnId enum and utilities |
| `ckid.hpp` | CK-specific ID system (Type_Gmst, etc.) |
| `metadata.hpp` | Plugin metadata (author, description, etc.) |

**Key Design Pattern — Collection/IdTable Bridge:**

```
ESMReader (binary) → Collection<RecordType> (in-memory) → IdTable (Qt model) → QTableView (UI)
```

1. `Data::continueLoading()` reads the ESM binary
2. For each record type, dispatches to the appropriate `Collection<ESXRecord>`
3. `Collection<ESXRecord>` stores records with state tracking
4. `IdTable` bridges the Collection to Qt's Model/View framework
5. `QTableView` displays the data

**Record States:**
```
enum State
{
    State_Base,           // Original record from master file
    State_Modified,       // Modified version in plugin
    State_ModifiedOnly,   // Created in this plugin only
    State_Deleted,        // Deleted (flag set)
    State_Erased          // Permanently removed from collection
};
```

### 3. View Layer (`src/view/`)

Qt-based UI layer handling user interaction.

| File | Purpose |
|------|---------|
| `mainwindow.cpp` | Main application window |
| `datadialog.cpp` | File selection dialog (Data dialog) |
| `gmstdialog.cpp` | Game settings editor |
| `viewmediator.cpp` | Mediator between model and view |
| `messageboxhelper.cpp` | Centralized message boxes |
| `window/*` | All dialog/window implementations |
| `world/delegates/*.cpp` | Custom delegates for rendering/editing |

**Editor Entry Point:**
```cpp
Editor::Editor(argc, argv)
    → FilePaths (detect games, set paths)
    → DocumentMediator (manage documents)
    → ViewMediator (UI wiring)
    → getDataPath() (load/save config)
```

## Data Flow

### Loading a Plugin

```
1. User selects file in DataDialog
2. Document constructor creates:
   - FilePaths (path management)
   - Data (central data hub)
3. Data::preload() creates ESMReader for the file
4. Data::continueLoading() iterates records:
   a. Read NAME (4CC)
   b. Switch on NAME:
      - 'GMST' → gameSettings.load(reader) → IdCollection<GameSetting>
      - 'NPC_' → npcCollection.load(reader)  → IdCollection<NPC_>
      - ... other types
      - default → skipRecord()
5. IdTable bridges each Collection to Qt model
6. UI displays data through QTableView
```

### Saving a Plugin

```
1. User clicks Save in UI
2. Document::save(savePath) called
3. ESMWriter opens file
4. For each Collection<ESXRecord>:
   - Iterate all records
   - For modified records (State_Modified, State_ModifiedOnly):
     - writer.startRecord(NAME, header)
     - record.save(writer)
     - writer.endRecord()
5. Writer finalizes file
```

## Module Dependencies

```
View (Qt UI)
  └── Model (domain logic)
       ├── Model/doc/ (document lifecycle)
       ├── Model/world/ (data collections)
       └── Libs/files/esm/ (binary I/O)
```

- View depends on Model
- Model depends on Libs
- Libs are platform-independent (no Qt dependency)
- View is the only Qt-dependent layer

## Template System

The codebase uses C++ templates extensively for type-safe record collections:

```cpp
// Template: Collection<RecordType>
Collection<GameSetting> gameSettings;
Collection<NPC_> npcCollection;
Collection<Weapon> weaponCollection;
```

Each Collection is typed to a specific record, providing:
- Type-safe storage
- Automatic ID indexing by EDID
- State tracking per record
- Column-based data access

## Key Design Decisions

1. **Platform-independent I/O**: All file parsing in `libs/` has no Qt dependency
2. **Template collections**: Type-safe, compile-time checked record containers
3. **Qt Model/View**: Uses IdTable bridge for clean separation
4. **State management**: Records track Base/Modified state for proper ESM merging
5. **Multi-game support**: Game parsers abstract game-specific differences
6. **Binary fidelity**: Reader/writer handle raw binary, preserving exact file content
