<#
.SYNOPSIS
    Nightly full-scale data-integrity gate (REMAINING.md §1.2).
.DESCRIPTION
    1. Untouched round-trip of Starfield.esm at full scale (3.8M records):
       load -> save -> snapshot-diff, via test_fullscale_roundtrip.
    2. FormIdCompactor verification on real-world plugins: compact a COPY of
       each candidate .esp in the data dir (originals are never touched) and
       verify the compacted file reloads cleanly.
    Needs a real Starfield install; skips gracefully without one. This is a
    nightly job, not a unit test — the full load takes several minutes.
.EXAMPLE
    pwsh -File tools/nightly-roundtrip.ps1
    pwsh -File tools/nightly-roundtrip.ps1 -DataDir "D:/Games/Starfield/Data"
#>
param(
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$BuildDir = "",
    [string]$Config = "Release",
    [string]$DataDir = $env:OPENCK_DATA_DIR,
    [int]$MaxCompactPlugins = 5
)

$ErrorActionPreference = "Stop"

# Test exes need Qt on PATH (Qt6Test.dll / Qt6Core.dll live next to them only
# via the Qt install dir).
$env:PATH = "C:\Qt\6.5.3\msvc2019_64\bin;" + $env:PATH

if ([string]::IsNullOrEmpty($BuildDir)) {
    $BuildDir = Join-Path $RepoRoot "build"
}
if ([string]::IsNullOrEmpty($DataDir)) {
    $DataDir = "C:/XboxGames/Starfield/Content/Data"
}
if (-not (Test-Path -LiteralPath (Join-Path $DataDir "Starfield.esm"))) {
    Write-Output "nightly: SKIP: no Starfield install at $DataDir"
    exit 0
}

$binDir = Join-Path $BuildDir "bin\$Config"
$roundtrip = Join-Path $binDir "test_fullscale_roundtrip.exe"
if (-not (Test-Path -LiteralPath $roundtrip)) {
    Write-Output "nightly: building test_fullscale_roundtrip ($Config)"
    & cmake --build $BuildDir --config $Config --target test_fullscale_roundtrip
    if ($LASTEXITCODE -ne 0) { Write-Output "nightly: FAIL: build failed"; exit 1 }
}

$workDir = Join-Path ([System.IO.Path]::GetTempPath()) "opencode-nightly"
New-Item -ItemType Directory -Force -Path $workDir | Out-Null
$failures = 0

# --- 1. Full-scale untouched round-trip ---
Write-Output "nightly: [1/2] Starfield.esm untouched round-trip (full scale)"
$saved = Join-Path $workDir "Starfield_untouched.esp"
& $roundtrip $DataDir "Starfield.esm" $saved
if ($LASTEXITCODE -ne 0) { Write-Output "nightly: FAIL: round-trip differences found"; $failures++ }
else { Write-Output "nightly: round-trip payload-identical" }

# --- 2. Compaction on real-world plugins (copies only) ---
# Candidates are copied into the data dir under a ~nightly_ name so their
# masters resolve, compacted, and removed again. Originals are never touched.
Write-Output "nightly: [2/2] FormIdCompactor on real-world plugins"
$candidates = Get-ChildItem -LiteralPath $DataDir -Filter "*.esp" -File |
    Where-Object { ($_.Name -ne "Starfield.esm") -and (-not $_.Name.StartsWith("~nightly_")) } |
    Select-Object -First $MaxCompactPlugins
if ($candidates.Count -eq 0) {
    Write-Output "nightly: no candidate .esp files for compaction check"
} else {
    foreach ($esp in $candidates) {
        $tempName = "~nightly_" + $esp.Name
        $tempPath = Join-Path $DataDir $tempName
        $compacted = Join-Path $workDir ("compact_out_" + $esp.Name)
        Copy-Item -LiteralPath $esp.FullName -Destination $tempPath -Force
        try {
            & $roundtrip $DataDir $tempName $compacted --compact
            if ($LASTEXITCODE -ne 0) {
                Write-Output "nightly: FAIL: compaction of $($esp.Name)"; $failures++
            } else {
                Write-Output "nightly: compact ok: $($esp.Name)"
            }
        } finally {
            Remove-Item -LiteralPath $tempPath -Force -ErrorAction SilentlyContinue
        }
    }
}

if ($failures -gt 0) { Write-Output "nightly: FAIL: $failures check(s) failed"; exit 1 }
Write-Output "nightly: PASS"
exit 0
