<#
.SYNOPSIS
    Final build gate (section 4.6): fake-data lint, zero-warning build, full CTest run.
.DESCRIPTION
    1. Runs tools/fakedata-lint.ps1 (fails fast on hardcoded sample data).
    2. Builds the `all_tests` target and fails on any MSVC warning outside
       external/ vendored code.
    3. Runs the full CTest suite with --output-on-failure.
    Memory-leak coverage comes from the in-suite instrumentation
    (_CrtCheckMemory / HeapValidate in the materialization-matrix test);
    coverage reporting needs OpenCppCoverage and is out of scope here.
.EXAMPLE
    pwsh -File tools/gate.ps1
    pwsh -File tools/gate.ps1 -Clean -Config Release
#>
param(
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$BuildDir = "",
    [string]$Config = "Release",
    [switch]$Clean,
    [string]$ExtraCMakeArgs = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrEmpty($BuildDir)) {
    $BuildDir = Join-Path $RepoRoot "build"
}
$logDir = Join-Path $BuildDir "gate-logs"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$buildLog = Join-Path $logDir "gate-build.log"

function Fail($msg) {
    Write-Output "gate: FAIL: $msg"
    exit 1
}

# --- 1. Lint ---
Write-Output "gate: [1/3] fake-data lint"
& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "fakedata-lint.ps1") -RepoRoot $RepoRoot
if ($LASTEXITCODE -ne 0) { Fail "fake-data lint reported violations" }
Write-Output "gate: lint clean"

# --- 2. Configure + build ---
if ($Clean -and (Test-Path -LiteralPath $BuildDir)) {
    Write-Output "gate: removing $BuildDir"
    Remove-Item -LiteralPath $BuildDir -Recurse -Force
}
if (-not (Test-Path -LiteralPath (Join-Path $BuildDir "CMakeCache.txt"))) {
    Write-Output "gate: configuring $BuildDir"
    $cfgArgs = @("-S", $RepoRoot, "-B", $BuildDir)
    if (-not [string]::IsNullOrEmpty($ExtraCMakeArgs)) {
        $cfgArgs += $ExtraCMakeArgs.Split(' ', [System.StringSplitOptions]::RemoveEmptyEntries)
    }
    & cmake $cfgArgs
    if ($LASTEXITCODE -ne 0) { Fail "cmake configure failed" }
}

Write-Output "gate: [2/3] building all_tests ($Config)"
& cmake --build $BuildDir --config $Config --target all_tests > $buildLog 2>&1
if ($LASTEXITCODE -ne 0) { Fail "build failed (see $buildLog)" }

$warnings = Get-Content -LiteralPath $buildLog |
    Where-Object { $_ -match 'warning C[0-9]+' -and $_ -notmatch 'external\\' }
if ($warnings.Count -gt 0) {
    Write-Output "gate: $($warnings.Count) compiler warning(s) outside external/:"
    foreach ($w in ($warnings | Select-Object -First 20)) { Write-Output "  $w" }
    Fail "zero-warning gate violated (see $buildLog)"
}
Write-Output "gate: build clean, zero warnings"

# --- 3. Tests ---
Write-Output "gate: [3/3] ctest ($Config)"
Push-Location $BuildDir
try {
    & ctest -C $Config --output-on-failure
    if ($LASTEXITCODE -ne 0) { Fail "ctest reported failures" }
} finally {
    Pop-Location
}

Write-Output "gate: PASS: lint clean, zero-warning build, all tests green"
exit 0
