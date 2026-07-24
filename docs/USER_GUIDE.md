# OpenCK User Guide

## Overview

OpenCK is an open-source replacement for Bethesda Game Studios' Creation Kit, written in C++ using the Qt framework. It is heavily inspired by OpenCS, developed by the OpenMW team for Morrowind.

OpenCK allows you to:
- Open and edit ESM/ESP files for multiple Bethesda games
- Manage plugin load orders
- Edit records (NPCs, weapons, spells, etc.)
- Create new plugins
- Work with Skyrim (LE/SE/AE), Oblivion, and Morrowind

## Installation

### System Requirements

- **Operating System**: Windows 10/11 (64-bit)
- **RAM**: 4GB minimum (8GB recommended)
- **Storage**: 500MB available space
- **Qt**: 5.15 or newer (included in distribution)

### Installing from Distribution

1. Download the latest release from the GitHub releases page
2. Extract the archive to your desired location
3. Run `openck.exe` from the extracted directory

### Building from Source

See the [BUILD.md](BUILD.md) file for detailed build instructions.

Quick start:
```bash
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/msvc2019_64"
cmake --build . --config Release
```

## First-Time Setup

### Configuring Game Paths

Before using OpenCK, you must configure the data paths for your games:

1. Launch OpenCK
2. Navigate to **Settings** → **Game Configuration**
3. For each game you want to use:
   - Select the game from the dropdown
   - Browse to the game's data directory
   - Click **Save**

#### Default Data Directory Locations

**Skyrim Special Edition/Anniversary Edition**
```
C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\data
```

**Skyrim Legendary Edition**
```
C:\Program Files (x86)\Steam\steamapps\common\Skyrim\data
```

**Oblivion**
```
C:\Program Files (x86)\Steam\steamapps\common\Oblivion\Data
```

**Morrowind**
```
C:\Program Files (x86)\Steam\steamapps\common\Morrowind\Data Files
```

### Editor Configuration

OpenCK stores its configuration in `editor.ini` located in the same directory as `openck.exe`. You can manually edit this file if needed.

Key settings:
- `ActiveGame` - The currently selected game
- `DataPaths` - JSON object mapping game names to directory paths

## Opening Files

### Opening an Existing Plugin

1. Go to **File** → **Open** (or press `Ctrl+O`)
2. Navigate to your plugin file (.esm or .esp)
3. Select the file and click **Open**

The plugin will load and display in the main editor window. You can view:
- **Records**: All records in the plugin
- **Headers**: Plugin headers and metadata
- **Strings**: Localized strings (if present)

### Opening Master Files

Master files (ESM files) can be opened the same way as plugins. When a plugin references a master file, OpenCK will automatically load it if the path is configured.

### Creating a New Plugin

1. Go to **File** → **New Plugin** (or press `Ctrl+N`)
2. Select the game for your new plugin
3. Choose a parent master file (if applicable)
4. Click **Create**

A new plugin will be created with a valid header structure.

## Navigating the Editor

### Main Window Layout

The OpenCK editor consists of several panels:

- **Record List**: Displays all records in the current plugin
- **Record Editor**: Shows the selected record's data
- **Properties Panel**: Displays metadata about the selected record
- **Search Bar**: Filter records by editor ID or type

### Record Types

OpenCK supports the following record types:

#### Skyrim Record Types
- **NPC_** - NPCs and Actors
- **WEAP** - Weapons
- **ARMOR** - Armor
- **SPEL** - Spells
- **MAGIC** - Magic Effects
- **QUEST** - Quests
- **DIAL** - Dialogue
- **INFO** - Dialogue Information
- **GMST** - Game Settings
- **GLOB** - Global Variables

#### Oblivion Record Types
- **NPC_** - NPCs and Actors
- **WEAP** - Weapons
- **ARMOR** - Armor
- **SPEL** - Spells
- **MAGIC** - Magic Effects
- **QUEST** - Quests
- **CONT** - Containers
- **INGR** - Ingredients

#### Morrowind Record Types
- **ACRT** - Actored Classes
- **ACTT** - Actor Templates
- **ALCH** - Alchemy
- **APAZ** - Spell Effects
- **ARMA** - Armor
- **BOOK** - Books
- **CLAS** - Classes
- **CNTN** - Containers
- **FACT** - Factions
- **GMST** - Game Settings
- **INSE** - Insect Sets
- **INGR** - Ingredients
- **KEYM** - Keys
- **LGNC** - Land Generators
- **LTEX** - Land Textures
- **MGEF** - Magic Effects
- **NPC_** - NPCs and Actors
- **PROS** - Projectiles
- **PRTY** - Partys
- **RACE** - Races
- **SCPT** - Scripts
- **SOUN** - Sounds
- **SPEL** - Spells
- **STAT** - Static Objects
- **TREE** - Trees
- **WEAP** - Weapons

### Filtering Records

You can filter the record list using the search bar:
- Type the editor ID to find a specific record
- Type a record type (e.g., "NPC_") to filter by type
- Use partial matches (e.g., "weap" will match "WEAP")

### Sorting Records

Click on column headers to sort:
- **Editor ID**: Alphabetically by editor ID
- **Record Type**: Alphabetically by record type
- **Flags**: By record flags

## Editing Records

### Selecting a Record

1. Click on a record in the record list
2. The record's data will appear in the editor panel
3. Modify the values as needed

### Editing Data Fields

Data fields vary by record type. Common editable fields include:

- **Editor ID** (EDID): The unique identifier for the record
- **Name** (FNAM): Display name
- **Data fields**: Type-specific numerical or text values

### Editing Models

Some records (WEAP, ARMOR, etc.) have model data:
- **MODL**: Model filename
- **MODB**: Binary model data
- **MODT**: Texture data

Models are stored as binary data and cannot be directly edited in OpenCK. You will need external tools to modify 3D models and textures.

### Editing Scripts

Script subrecords (SNAM) contain script data:
- Scripts are stored as bytecode
- Use a script editor or decompiler to modify scripts
- OpenCK can view but not directly edit script bytecode

### Undo/Redo

OpenCK supports undo/redo for record edits:
- **Undo**: `Ctrl+Z`
- **Redo**: `Ctrl+Y`

Note: Undo/redo is currently limited to in-memory edits and may not be available for all record types.

## Saving Files

### Saving Changes

1. Make your edits to the records
2. Go to **File** → **Save** (or press `Ctrl+S`)
3. The file will be saved with your changes

### Saving As New File

1. Go to **File** → **Save As** (or press `Ctrl+Shift+S`)
2. Choose a filename and location
3. Click **Save**

### Master vs Plugin Files

- **ESM files** (masters): Typically not modified directly
- **ESP files** (plugins): Modified to add content

When saving, OpenCK preserves the Master flag from the original file.

## Plugin Load Order

### Viewing Load Order

1. Go to **Tools** → **Load Order**
2. View the current load order for your selected game
3. Notes on conflicts between plugins

### Managing Load Order

To change the load order:
1. Select a plugin in the list
2. Use **Move Up** / **Move Down** buttons
3. Or drag and drop plugins to reorder

### Conflict Detection

OpenCK will highlight conflicts between plugins:
- Records with the same editor ID
- Conflicting data values
- Dependency issues

Conflicts are marked with icons in the load order view.

### Merge Plugins

To merge multiple plugins into one:
1. Select the plugins you want to merge
2. Go to **Tools** → **Merge Plugins**
3. Follow the wizard to combine them

## Multi-Game Support

### Switching Games

1. Go to **Settings** → **Game Configuration**
2. Select a different game from the dropdown
3. Click **Switch Game**

OpenCK will reload with the new game's settings and supported record types.

### Game-Specific Features

Different games have different record types and formats:
- **Skyrim**: Supports all modern Creation Kit features
- **Oblivion**: Supports Oblivion-specific record types
- **Morrowind**: Supports Morrowind-specific record types

When switching games, only records supported by the selected game will be visible.

### Master File Requirements

Each game has different master file requirements:
- **Skyrim**: Requires Skyproc or similar for masterless plugins
- **Oblivion**: Some plugins require Oblivion.esm
- **Morrowind**: Requires Morrowind.esm

OpenCK will warn you if required masters are missing.

## Troubleshooting

### Common Issues

#### Plugin Won't Open

**Problem**: Plugin fails to open with an error.

**Solution**:
1. Verify the file is a valid ESM/ESP file
2. Check that the file is not corrupted
3. Ensure you have the required master files
4. Try opening with a different game selected

#### Record Not Saving

**Problem**: Changes don't persist after saving.

**Solution**:
1. Ensure you're saving to a valid location
2. Check file permissions
3. Verify the plugin is not marked as read-only
4. Try saving as a new file

#### Slow Performance

**Problem**: OpenCK is slow when opening large plugins.

**Solution**:
1. Close other applications to free up memory
2. Disable unnecessary plugins in load order
3. Use search/filter to reduce the number of records displayed
4. Consider splitting large plugins into smaller ones

#### Missing Records

**Problem**: Some records don't appear in the editor.

**Solution**:
1. Check that you're using the correct game
2. Verify the record type is supported
3. Ensure the plugin is properly formatted
4. Try re-exporting the plugin from the official Creation Kit

### Error Messages

#### "Failed to load record: [Record Type]"

The record type is not yet supported. OpenCK is currently in development and not all record types are implemented.

#### "Master file not found: [Master Name]"

The required master file is missing or not configured. Configure the master file path in Settings → Game Configuration.

#### "Invalid ESM header"

The file is not a valid ESM/ESP file or is corrupted. Verify the file integrity.

### Getting Help

If you encounter issues not covered here:

1. Check the [GitHub Issues](https://github.com/open-ck/openck/issues) page
2. Search for similar issues
3. Create a new issue with:
   - Description of the problem
   - Steps to reproduce
   - OpenCK version
   - Game and plugin version
   - Error messages (if any)

## Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| Open File | `Ctrl+O` |
| Save File | `Ctrl+S` |
| Save As | `Ctrl+Shift+S` |
| New Plugin | `Ctrl+N` |
| Undo | `Ctrl+Z` |
| Redo | `Ctrl+Y` |
| Search/Filter | `Ctrl+F` |
| Close File | `Ctrl+W` |
| Quit | `Ctrl+Q` |

## Command Line Usage

OpenCK can be launched from the command line with options:

```bash
openck.exe [options] [file]
```

### Options

- `--game <name>`: Set the active game (skyrim, skyrimse, skyrimae, oblivion, morrowind)
- `--data-path <path>`: Override data directory path
- `--load-order <file>`: Load load order from file
- `--version`: Display version information
- `--help`: Display help information

### Examples

```bash
# Open with specific game
openck.exe --game skyrimse plugin.esp

# Open with custom data path
openck.exe --data-path "C:\Games\Skyrim\data" plugin.esp

# Display version
openck.exe --version
```

## Advanced Topics

### Plugin Structure

ESM/ESP files contain:
- **Header** (TES4): Global plugin metadata
- **Records**: Individual game objects (NPCs, weapons, etc.)
- **Strings**: Localized text strings

### Record Structure

Each record contains:
- **Record Type** (4-character code)
- **Flags**: Record flags (master, deleted, etc.)
- **Editor ID** (EDID): Unique identifier
- **Data**: Type-specific data fields
- **Scripts**: Optional script data
- **Models**: Optional 3D model data

### Load Order Principles

1. **Master files first**: ESM files should load before ESP files
2. **Dependencies first**: Plugins with dependencies should load before dependents
3. **Conflicts last**: Plugins that override others should load later
4. **Test your load order**: Use a mod manager to verify compatibility

### Best Practices

1. **Backup your plugins**: Always backup before making changes
2. **Test frequently**: Save and test changes regularly
3. **Use editor IDs**: Always use meaningful editor IDs
4. **Keep plugins small**: Split large changes into multiple plugins
5. **Document your changes**: Keep notes on what you've modified
6. **Check load order**: Regularly review and adjust load order
7. **Resolve conflicts**: Address conflicts between plugins
8. **Validate plugins**: Use validation tools to check plugin integrity

## Contributing

OpenCK is an open-source project. Contributions are welcome!

### How to Contribute

1. **Report bugs**: Create issues on GitHub
2. **Suggest features**: Open feature requests
3. **Write code**: Contribute to the codebase
4. **Write tests**: Help improve test coverage
5. **Write documentation**: Improve this guide
6. **Review code**: Review pull requests

### Development Setup

See [BUILD.md](BUILD.md) for building from source.

### Code Style

Follow existing code style:
- Use meaningful variable and function names
- Add comments for complex logic
- Write unit tests for new features
- Follow C++17 best practices

### Pull Request Process

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Write tests
5. Submit a pull request

## License

OpenCK is released under the GNU General Public License v3.0. See the [LICENSE](LICENSE) file for details.

## Credits

- **OpenMW Team**: For OpenCS, which inspired OpenCK
- **Bethesda Game Studios**: For the Creation Kit and Bethesda games
- **Contributors**: All contributors to the OpenCK project

## Contact

- **GitHub**: https://github.com/open-ck/openck
- **Issues**: https://github.com/open-ck/openck/issues
- **Discussions**: https://github.com/open-ck/openck/discussions

---

*Last updated: June 2026*
