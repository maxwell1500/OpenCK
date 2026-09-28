# OpenCK — Agent Guidelines

This file describes the architecture, conventions, and gotchas an AI coding
agent needs to know to be productive in this codebase. Read it before
making non-trivial changes.

**`REMAINING.md` is the single source of truth for outstanding work.** Do not
create new planning/roadmap/debt/status documents; update `REMAINING.md` when
you complete an item or discover genuinely new remaining work.

## What OpenCK is

OpenCK is a C++/Qt6 open-source recreation of Bethesda's Creation Kit,
the official plugin editor for Bethesda's games. The goal is to let
users open, edit, and save `.esp`/`.esm` plugin files for Morrowind,
Skyrim, Fallout 4, and Starfield without needing the proprietary
Creation Kit. It does not ship any Bethesda assets, dependencies on
proprietary file formats beyond what the modding community has
documented, or any of Bethesda's source code. See
`docs/CK_Real_Integration_Plan.md` for what the real CK looks like
under the hood and how OpenCK's architecture maps to it.

## Build & test

Build: `cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.5.3/msvc2019_64 && cmake --build build --config Debug --target openck`

Test: every `test_*.exe` in `build/bin/Debug/` exits 0. Do **not** put Qt on
`PATH` to make this work — the build deploys the Qt runtime the tests need next
to the test exes, and `ctest` plus the bare exes run with no `PATH` help at all.
PowerShell loop:

```powershell
$tests = Get-ChildItem build\bin\Release\test_*.exe | ForEach-Object { $_.FullName }
$pass = 0
foreach ($t in $tests) { & $t 2>&1 | Out-Null; if ($LASTEXITCODE -eq 0) { $pass++ } }
"Pass: $pass"
```

## Layered architecture

```
libs/files/esm/      ← ESM/ESP parser. No Qt. Pure C++/QtCore.
libs/components/     ← Component-Property architecture. Tier 1+2 components.
src/model/           ← Domain objects (Data, Records, UndoStack).
src/view/            ← Qt UI. Dialogs, widgets, main window.
```

The `libs/components/` layer is the architectural centerpiece.
Components own slices of a record's subrecords and expose them as
`EditorProperty` leaves. The editor UI is data-driven: a record opens
in a `QtFormDialog` that walks the component list and renders each
component's properties. No per-record dialog code is needed.

## When you add a new record type

1. Create a struct in `libs/files/esm/MyRecord.hpp` embedding
   `openck::FormComponents components;`. Keep flat field mirrors for
   back-compat if other code reads them.
2. In the .cpp `load()`, seed the components you need
   (`components.add<TESModel_Component>()` etc.) then walk subrecords,
   dispatching each to the first component whose `canHandle(NAME)`
   returns true. Unhandled subrecords go in `rawSubRecords` for
   lossless round-trip.
3. `ObjectWindowDialog` will route any open through
   `QtFormDialogManager::instance().openOrFocus(formIdKey, &record->components)`.
4. Add the new record type to the switch in
   `ObjectWindowDialog::editSelected` if it's a new record kind.

See `libs/files/esm/Statrecord.cpp` and `Miscrecord.cpp` for the
canonical pattern.

## Legal / reuse rules

- **Do not** copy string literals, error messages, dialog text, or
  layout strings verbatim from `CreationKit.exe`. They are copyrighted
  expression. Use our own wording.
- **Do not** reproduce the real CK's source tree path
  (`Genesis\Construction Set\...`) in our own codebase. We use
  our own conventions.
- **Do not** ship any Bethesda asset (textures, fonts, icons, the
  `Starfield.esm` master, etc.).
- **Do not** call the product "Creation Kit" — that's Bethesda's
  trademark. "OpenCK" is fine as long as we don't imply endorsement.
- OK to reimplement observable behavior of a publicly-distributed
  free tool. OK to use the MIT- or LGPL-licensed libraries Bethesda
  themselves ship (QtAdvancedDocking is LGPL — we link it as a
  shared library to satisfy the LGPL dynamic-link clause, see
  `external/ads/CMakeLists.txt`).
- OK to use our own wording that describes the same concepts in
  English (e.g. "Object Window", "Cell View", "Render Window",
  "Keywords" — all general-purpose English).

## Important libraries and where they live

| Lib | What for | Where |
|---|---|---|
| Qt6 (5.5–6.5 tested, 6.5.3 used in CI) | UI framework | External, system Qt6 |
| QtAdvancedDocking (ADS) | Tear-off / tabbed / redockable windows | `external/ads/` (vendored) |
| NifTools nifly | NIF file I/O - **ground truth for block boundaries** | `build/blender/blender-*/4.2/scripts/addons/io_scene_niftools/dependencies/` (nifgen + pyffi). `external/pynifly/` is an empty placeholder. Parse with `nifgen.formats.nif.NifFile.from_stream(handle)` and read `block.io_start` / `block.io_size` per block; the class is `NifFile`, not `NIFFile`. It reads pre-20.2.0.5 containers, which have no size table, so it answers the question the C++ cannot. |
| libogg, libvorbis | OGG encoding | `external/ogg/`, `external/vorbis/` (vendored) |
| DbgHelp | Crash stack traces (Windows only) | system, linked via `dbghelp.lib` |

QtAdvancedDocking is LGPL-2.1-or-later with the Qt exception. We
build it as a SHARED library and ship `qtadvanceddocking-qt6.dll`
next to `openck.exe`. End users may replace that DLL with their own
build, which is what the LGPL dynamic-link clause intends.

## Gotchas

- **`NAME` macro**: `typedef uint32_t NAME;` in
  `libs/files/esm/common.hpp`. Used as 4-byte record / subrecord
  name. Use `NAME('FULL')` to spell literals.
- **Record header is 24 bytes** on disk: 4 name + 4 size + 4 flags + 4
  formId + 4 vcBytes + 2 version + 2 unknown. The
  `readHeader()` method reads all of this.
- **`Tes4Codes::fromTes4`**: translates on-disk 4-byte NAMEs. For
  example, on-disk `'MODL'` becomes internal `'ODIT'`. If your
  component's `canHandle` test reads from a real ESM stream, expect
  the translated name, not the raw one.
- **`ESMReader::recLeft` is signed `qint64`** and can be negative
  after a corrupt subrecord. The `Header::load` and `CellRecord::load`
  loaders check for this and break. Keep this pattern in any new
  record loaders you write.
- **Compressed records**: flag `0x00040000`. The component sees
  already-decompressed subrecords; the ESMReader handles zlib
  transparently. Just call the regular `readNSubHeader()` /
  `readZString()` / `readType<T>()` and the bytes are unmangled for
  you. See `libs/files/esm/esmreader.cpp`'s `decompressCurrentRecord`.
- **The cell loader used to spin** on a sub=0x0 return from
  `readNSubHeader`. We fixed this — when `readNSubHeader` returns 0
  it means the record is drained, and the load loop should break.
  Don't re-add the old 500-iter guard.
- **The ObjectWindowDialog used to open a bespoke
  `StatEditor`/`ArmorEditor`/etc.** for each record type. That
  pathway is deprecated. The supported path is
  `QtFormDialogManager::instance().openOrFocus(formIdKey, &record->components)`.
- **`openck::FormComponents::findByName("TESModel")` returns a
  `Component*`**. To access the typed fields, `static_cast`:
  `static_cast<tescomponents::TESModel_Component*>(components.findByName("TESModel"))`.
- **In strict binary parsers, read floats as u32 bits + memcpy, never
  `QDataStream::operator>>(float&)`.** Observed 2026-09-15 in
  `libs/files/nif/nifparser.cpp::parseBsMeshData`: `s >> floatVar`
  consumed 8 bytes and yielded 0.0 where the same stream position read as
  u32 + memcpy gave the correct 4-byte float (verified by device pos +
  value against ground-truth bytes). u8/u16/u32/i32 reads are exact.
- **`git commit -q` can mask a failed commit.** Observed 2026-09-26: a commit
  message containing backticks was passed inline through PowerShell, which
  evaluated them as command substitution; `git commit -q` printed nothing, the
  shell still reported success, and the change sat staged and uncommitted. Always
  write the message to a file and pass `-F <file>`, then confirm with
  `git log --oneline -1` that the hash and subject actually changed. Do not trust
  the exit code alone.
- **A `-F` message file that already exists will silently reuse its old
  contents.** Observed 2026-09-27: a scratch message path under `%TEMP%` was
  reused from two days earlier, so a commit went out under a completely
  unrelated subject that described work not in the commit at all — and the exit
  code was 0, so the mistake was invisible until the log was read. Prefer a
  message path you have just written, or delete it first; and compare the
  resulting `git log -1 --format=%s` against the subject you intended *word for
  word*, not just the hash. `Set-Content -Encoding UTF8` also writes a BOM on
  Windows PowerShell 5.1, which becomes part of the subject — write the file
  with `[System.IO.File]::WriteAllText($p, $text, (New-Object System.Text.UTF8Encoding($false)))`
  and check `[int][char]$subject[0]` is the character you expect.
- **The `Game` enum merges Skyrim LE, SE and AE into one value, on purpose.**
  They share a record layout, so record parsing and writing need no SE-specific
  branch and the merge is correct there. It is *not* correct for archive writing:
  LE uses BSA 0x68 and SE/AE use 0x69. That is why
  `BsaArchive::targetsForGame()` offers both for `Game::Skyrim` and the Create
  Archive dialog asks, rather than guessing. There is no LE/SE signal anywhere in
  the codebase today, so do not add a heuristic that infers one from a filename
  or a data-folder path without checking a real archive first.
- **`Qt6Test.dll was not found` is a deployment bug, not a `PATH` problem.**
  Observed repeatedly: every `test_*.exe` died at load with `0xC0000135`
  (STATUS_DLL_NOT_FOUND) and `ctest` reported nothing, which reads like a broken
  build but is a missing DLL next to the exes. Cause: `windeployqt` only ran
  against `openck.exe`, so it deployed the closure of what the *app* links, and
  the tests' `Qt6::Test` was never copied. `tests/CMakeLists.txt` now deploys
  over the test exes too (`deploy_qt_test_runtime`, stamp-driven off
  `build/qt-test-runtime.stamp`). If it ever reappears, build that target —
  do not add Qt to `PATH`, which only hides it for interactive runs and leaves
  CI and any other caller broken. Diagnose with
  `exit=$LASTEXITCODE` being `-1073741515` (that is `0xC0000135`, and PowerShell
  prints it as a negative number, so it is easy to misread as a crash).
- **Keep a `LOG_INFO` line at the top of new editor windows** so the
  log file shows when each window opens, matching the convention in  the existing main window.

## Debugging

- **Log file** is at `openck_YYYYMMDD_HHMMSS.log` next to `openck.exe`.
  The `Logger` is buffered to flush on every line. `LOG_DEBUG` is the
  lowest level; `LOG_FATAL` is the highest.
- **Crash handler** in `src/crashhandler.cpp` writes a stack trace to
  the log file before exiting. If you see a `Win32 exception
  ACCESS_VIOLATION` followed by a numbered list of `#N 0xADDR
  function+offset (file:line)`, that's the trace.
- **`OPENCK_TEST_STARFIELD_ESM` env var**: the `test_starfieldesm`
  test reads this to find a real `Starfield.esm` to parse. Point it at
  a copy you have.
- **`Data::preload` is the entry point** for parsing a single .esp
  / .esm file. It strips trailing control characters from the master
  filename before constructing the path; this is what allows
  Starfield's truncated-master names (with trailing NULs) to load.

## File conventions

- We use a single `.vcxproj`-based CMake build, no in-source
  vendoring. Vendored libs go under `external/<name>/`.
- C++17. Qt6. Microsoft Visual C++ 2019 toolchain (`msvc2019_64`).
- `LOG_INFO`, `LOG_DEBUG`, `LOG_WARNING`, `LOG_ERROR`, `LOG_FATAL`
  macros come from `libs/files/log/logger.hpp`.
- The `Q_OBJECT` macro is required for any class with signals/slots.
  CMake's `AUTOMOC` handles moc compilation automatically.
- Don't add a comment to a header file unless the comment explains
  *why*, not *what*. The user has asked repeatedly not to add filler
  comments.
