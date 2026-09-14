<#
.SYNOPSIS
    Coverage target (§4.6): runs OpenCppCoverage over test binaries.
.DESCRIPTION
    Builds the requested test targets in a RelWithDebInfo directory (PDBs
    are required; the default Release build emits none), runs each test
    under OpenCppCoverage, and prints a per-test line-rate summary from the
    Cobertura XML. Needs OpenCppCoverage installed ( Elevate once:
    OpenCppCoverageSetup-x64 from
    https://github.com/OpenCppCoverage/OpenCppCoverage/releases ).
    Full-suite coverage is slow (instrumentation overhead on the real-data
    tests); treat it as a nightly job and use -Tests for quick slices.
.EXAMPLE
    pwsh -File tools/coverage.ps1 -Tests test_opallist,test_particlesimulation
    pwsh -File tools/coverage.ps1 -All
#>
param(
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$BuildDir = "",
    [string]$Config = "RelWithDebInfo",
    [string[]]$Tests = @(),
    [switch]$All,
    [string]$QtPrefix = "C:/Qt/6.5.3/msvc2019_64",
    [string]$QtBinDir = "",
    [string]$ReportDir = "",
    [string]$OpenCppCoverage = "C:\Program Files\OpenCppCoverage\OpenCppCoverage.exe"
)

$ErrorActionPreference = "Stop"

# Test binaries need the Qt runtime next to them.
if ([string]::IsNullOrEmpty($QtBinDir)) {
    $QtBinDir = $QtPrefix.TrimEnd('/') + "/bin"
}
$env:PATH = $QtBinDir + ";" + $env:PATH

if ([string]::IsNullOrEmpty($BuildDir)) {
    $BuildDir = Join-Path $RepoRoot "build-cov"
}
if ([string]::IsNullOrEmpty($ReportDir)) {
    $ReportDir = Join-Path ([System.IO.Path]::GetTempPath()) "opencode-coverage"
}

if (-not (Test-Path -LiteralPath $OpenCppCoverage)) {
    Write-Output "coverage: FAIL: OpenCppCoverage not found at $OpenCppCoverage"
    Write-Output "coverage: install it once (elevated) from https://github.com/OpenCppCoverage/OpenCppCoverage/releases"
    exit 2
}

if ($Tests.Count -eq 0 -and -not $All) {
    Write-Output "coverage: pass -Tests <names...> or -All"
    exit 2
}

# Normalize: callers may pass one comma-joined string (e.g. via -File).
$Tests = @($Tests | ForEach-Object { $_ -split ',' } |
    ForEach-Object { $_.Trim() } | Where-Object { $_ -ne '' })

if (-not (Test-Path -LiteralPath (Join-Path $BuildDir "CMakeCache.txt"))) {
    Write-Output "coverage: configuring $BuildDir ($Config)"
    & cmake -S $RepoRoot -B $BuildDir -G "Visual Studio 17 2022" -A x64 "-DCMAKE_PREFIX_PATH=$QtPrefix"
    if ($LASTEXITCODE -ne 0) { Write-Output "coverage: FAIL: configure failed"; exit 1 }
}

if ($All) {
    Write-Output "coverage: building all_tests ($Config) - this takes a while on first run"
    & cmake --build $BuildDir --config $Config --target all_tests
    if ($LASTEXITCODE -ne 0) { Write-Output "coverage: FAIL: build failed"; exit 1 }
    $binDir = Join-Path $BuildDir "bin\$Config"
    $Tests = Get-ChildItem $binDir -Filter "test_*.exe" | ForEach-Object {
        [System.IO.Path]::GetFileNameWithoutExtension($_.Name)
    } | Where-Object { $_ -ne "test_subrecord_diff" }
}

New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null
$binDir = Join-Path $BuildDir "bin\$Config"
$failed = @()

foreach ($t in $Tests) {
    $exe = Join-Path $binDir "$t.exe"
    if (-not (Test-Path -LiteralPath $exe)) {
        Write-Output "coverage: building $t ($Config)"
        & cmake --build $BuildDir --config $Config --target $t
        if ($LASTEXITCODE -ne 0) { Write-Output "coverage: FAIL: build of $t failed"; exit 1 }
    }
    $pdb = [System.IO.Path]::ChangeExtension($exe, ".pdb")
    if (-not (Test-Path -LiteralPath $pdb)) {
        Write-Output "coverage: FAIL: no PDB for $t (need a $Config build with debug info)"
        exit 1
    }
    $out = Join-Path $ReportDir "$t-cobertura.xml"
    Write-Output "coverage: running $t"
    & $OpenCppCoverage --sources (Join-Path $RepoRoot "src") --modules $t `
        --export_type "cobertura:$out" -- $exe
    if ($LASTEXITCODE -ne 0) {
        $failed += $t
        continue
    }
    try {
        [xml]$xml = Get-Content -LiteralPath $out
        $rate = [double]$xml.coverage.'line-rate' * 100.0
        $lines = $xml.coverage.'lines-covered'
        $total = $xml.coverage.'lines-valid'
        Write-Output ("coverage: {0}: {1:N1}% ({2}/{3} lines)" -f $t, $rate, $lines, $total)
    } catch {
        Write-Output "coverage: ${t}: ran, but the report could not be parsed"
    }
}

if ($failed.Count -gt 0) {
    Write-Output ("coverage: FAIL: {0} test(s) failed: {1}" -f $failed.Count, ($failed -join ", "))
    exit 1
}
Write-Output "coverage: PASS: reports in $ReportDir"
exit 0
