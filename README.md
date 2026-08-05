# OpenCK

**OpenCK** is an open-source replacement for Bethesda Game Studios' Creation Kit, written in C++ using the Qt framework. It is heavily inspired by the **OpenCS** project developed by the OpenMW team for Morrowind.

## Quick Start

- **Build**: See [docs/BUILD.md](docs/BUILD.md)
- **Usage**: See [docs/USER_GUIDE.md](docs/USER_GUIDE.md)
- **Architecture**: See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- **Roadmap**: See [ROADMAP.md](ROADMAP.md)
- **Implementation Plan**: See [docs/UNIFIED_PLAN.md](docs/UNIFIED_PLAN.md)
- **Status**: See [STATUS.md](STATUS.md)

## Supported Games

- Skyrim (Legendary / Special / Anniversary Edition)
- Oblivion
- Morrowind
- Fallout 3
- Fallout: New Vegas
- Fallout 4
- Starfield

## Features

### Editors & UI
- Object Window with 3-level hierarchical tree, filtering, and generic copy/paste for all record types
- Search dialog with multi-criteria find and generic record editing
- Cell View 2D map canvas with pan/zoom/select/marquee
- Render Window with gizmo transform tools (translate/rotate/scale)
- Landscape editor with texture layers, heightmap import/export, autopaint
- NavMesh editor with mesh check, cleaning, vertex welding, and T-junction removal
- Archive Browser for BSA and BA2 (both general and DX10 texture) archives with previews
- Data-driven record editing via the component/property architecture (`QtFormDialog`)

### File Formats
- ESM/ESP binary reader and writer (Morrowind through Starfield)
- BA2 archive reader/writer: general (GNRL) and DX10 texture archives, zlib and Starfield v3 LZ4
- BSA archive reader/writer (SSE v0x69)
- NIF loading/preview via bundled NifTools
- Papyrus scripting: prevalidator, type checker, language server, remote debugger

### Data & Tooling
- Plugin load order and active-plugin management
- ESL / light-master authoring with form-ID renumbering (`FormIdCompactor`, `PluginCompactor`)
- Bashed patch generation, asset validation, dependency scanning
- Version control integration (Git / Perforce)
- Batch rename, asset conversion, LOD generation, report export

## Build & Test

102 automated tests cover the record formats, components, archive readers/writers, undo system, and tooling. Run the full suite from `build/bin/Debug/` — every `test_*.exe` should exit 0.

## License

GNU General Public License v3.0 — See [LICENSE](LICENSE) for details.
