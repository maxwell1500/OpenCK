# OpenCK

[![Build Status](https://travis-ci.com/Open-CK/OpenCK.svg?branch=master)](https://travis-ci.com/Open-CK/OpenCK)
[![Appveyor](https://ci.appveyor.com/api/projects/status/vlg4y2e96180qddx?svg=true)](https://ci.appveyor.com/project/Adam-Gleave/openck-vv4vu)
[![CodeFactor](https://www.codefactor.io/repository/github/open-ck/openck/badge)](https://www.codefactor.io/repository/github/open-ck/openck)

**OpenCK** is an open-source replacement for Bethesda Game Studios' Creation Kit, written in C++ using the Qt framework. It is heavily inspired by the **OpenCS** project developed by the OpenMW team for Morrowind.

## Quick Start

- **Build**: See [docs/BUILD.md](docs/BUILD.md)
- **Usage**: See [docs/USER_GUIDE.md](docs/USER_GUIDE.md)
- **Architecture**: See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- **Roadmap**: See [ROADMAP.md](ROADMAP.md)
- **Implementation Plan**: See [docs/IMPLEMENTATION_PLAN.md](docs/IMPLEMENTATION_PLAN.md)

## Supported Games

- Skyrim Legendary Edition (Skyrim)
- Skyrim Special Edition (SkyrimSE)
- Skyrim Anniversary Edition (SkyrimAE)
- Oblivion
- Morrowind
- Fallout 3
- Fallout: New Vegas
- Fallout 4
- Starfield

## Features

### Working
- ESM/ESP binary reader and writer
- Multi-game support with auto-detection (Steam, Xbox Game Pass)
- Plugin load order management
- Active plugin selection from plugins.txt
- Game-specific configuration persistence

### In Development
- Full record type loading (22+ types implemented, wiring up dispatch)
- Record editing and saving
- Plugin merge and conflict detection
- 3D viewport for cell preview
- Script editing (Papyrus/OBScript)

### Planned
- Full Creation Kit feature parity
- Mod manager integration (Mod Organizer 2, Vortex)
- Automated testing and CI/CD
- Distributable Windows packages

## License

GNU General Public License v3.0 — See [LICENSE](LICENSE) for details.
