<#
.SYNOPSIS
    Fake-data lint (section 4.4): fails when hardcoded game-content strings appear
    in src/ instead of coming from fixtures.
.DESCRIPTION
    Scans src/**/*.cpp and src/**/*.hpp for the regexes in
    tools/fakedata-lint-patterns.txt. A hit is excused only when covered by
    tools/fakedata-lint-allowlist.txt ("path-part || pattern-part || reason").
    Exit 0 when clean, 1 with the offending locations otherwise.
#>
param(
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = "Stop"

$srcDir = Join-Path $RepoRoot "src"
$patternsFile = Join-Path $PSScriptRoot "fakedata-lint-patterns.txt"
$allowlistFile = Join-Path $PSScriptRoot "fakedata-lint-allowlist.txt"

if (-not (Test-Path -LiteralPath $srcDir)) {
    Write-Error "src/ not found under $RepoRoot"
    exit 2
}

$patterns = Get-Content $patternsFile | Where-Object { $_ -notmatch '^\s*(#|$)' }
$allowEntries = Get-Content $allowlistFile | Where-Object { $_ -notmatch '^\s*(#|$)' }

$violations = @()
$files = Get-ChildItem $srcDir -Recurse -Include *.cpp,*.hpp -File
foreach ($file in $files) {
    $rel = $file.FullName.Substring($RepoRoot.Length + 1).Replace('\', '/')
    $lines = Get-Content -LiteralPath $file.FullName
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        foreach ($pat in $patterns) {
            $matched = $false
            try {
                $matched = $line -match $pat
            } catch {
                Write-Error "bad lint pattern: $pat ($($_.Exception.Message))"
                exit 2
            }
            if (-not $matched) { continue }

            $excused = $false
            foreach ($entry in $allowEntries) {
                $parts = $entry -split '\|\|'
                if ($parts.Count -lt 2) { continue }
                $pathPart = $parts[0].Trim()
                $patPart = $parts[1].Trim()
                if ($rel.Contains($pathPart) -and ($line -match $patPart)) {
                    $excused = $true
                    break
                }
            }
            if (-not $excused) {
                $num = $i + 1
                $violations += "${rel}:${num}: $($line.Trim())  [pattern: $pat]"
            }
        }
    }
}

if ($violations.Count -gt 0) {
    Write-Output "fakedata-lint: $($violations.Count) violation(s) in src/ (sample data must come from fixtures):"
    foreach ($v in $violations) { Write-Output "  $v" }
    exit 1
}

Write-Output "fakedata-lint: clean ($($files.Count) files scanned)"
exit 0
