# OpenCK — Build & Test on Windows

## Prerequisites

1. **Visual Studio 2022** (Build Tools or Community) with the C++ workload.
2. **Qt 6.5.3 (msvc2019_64)** installed at `C:\Qt\6.5.3\msvc2019_64`
   - Download from: https://download.qt.io/archive/qt/6.5/6.5.3/
   - Or use the Qt Online Installer (select *Qt 6.5.3 → MSVC 2019 64-bit*).
   - The editor needs the Qt3D modules too (`qt3d` in the installer, or
     `modules: qt3d` in aqt/CI).
   - Format support spans Qt 5.15–6.5, but 6.5.3 is what CI uses.

## Build

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.5.3/msvc2019_64
cmake --build build --config Debug --target openck
```

Build outputs land in `build/bin/Debug/` (exe + Qt DLLs). The vendored
QtAdvancedDocking system DLL is copied next to `openck.exe` automatically
(`qtadvanceddocking-qt6d.dll` in Debug, `qtadvanceddocking-qt6.dll` in
Release) by a POST_BUILD step on `openck` and the `copy_ads_dll` test
dependency.

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

Tests write their log files to `build/test-logs/` (`OPENCK_LOG_DIR`) so
`build/bin` stays clean.

## Release gate

Same commands with `--config Release`. The full Release gate (build + 100%
CTest + installer) runs automatically on GitHub Actions — a clean CI run is
the release-parity signal (see Phase B/B5 in `docs/REMAINING_WORK_PLAN.md`).

## CI

GitHub Actions (`.github/workflows/windows-build.yml`) builds Release,
runs `all_tests`, and executes `ctest -C Release` on every push/PR. It pins
`windows-2022` (the latest image ships only VS18, which the VS17 generator
cannot target) and installs Qt 6.5.3 with the `qt3d` module.

## Distribution

The CI job builds the NSIS installer (`cpack -C Release`). To build it
locally you need NSIS installed (`makensis` on PATH):

```powershell
cmake --build build --config Release
cpack -C Release
```

Output: `build/OpenCK-*.exe` (NSIS, Windows).
