# OpenCK — Build & Test on Windows

## Prerequisites

1. **Visual Studio 2022** (Build Tools or Community) with the C++ workload.
2. **Qt 6.5.3 (msvc2019_64)** installed at `C:\Qt\6.5.3\msvc2019_64`
   - Download from: https://download.qt.io/archive/qt/6.5/6.5.3/
   - Or use the Qt Online Installer (select *Qt 6.5.3 → MSVC 2019 64-bit*).
   - Format support spans Qt 5.15–6.5, but 6.5.3 is what CI uses.

## Build

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.5.3/msvc2019_64
cmake --build build --config Debug --target openck
```

Build outputs land in `build/bin/Debug/` (exe + Qt DLLs).

## Test

Every `test_*.exe` in `build/bin/Debug/` must exit 0:

```powershell
cmake --build build --config Debug --target all_tests
$env:Path = "C:\Qt\6.5.3\msvc2019_64\bin;build\bin\Debug;$env:Path"
Get-ChildItem build\bin\Debug\test_*.exe | ForEach-Object {
    & $_.FullName
    "exit=$LASTEXITCODE $($_.Name)"
}
```

CTest (registered suite — some tests only register when a real game data
install is present, see `docs/REMAINING_WORK_PLAN.md` Phase A/A6):

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

## CI

GitHub Actions (`.github/workflows/windows-build.yml`) builds Release,
runs `all_tests`, and executes `ctest -C Release` on every push/PR.
Run the release gate locally with `-DCMAKE_BUILD_TYPE=Release` (or
`-G "Visual Studio 17 2022"` multi-config + `--config Release`).

## Distribution

Package the installer:

```powershell
cmake --build build --config Release
cpack -C Release
```

Output: `build/OpenCK-*.exe` (NSIS, Windows).