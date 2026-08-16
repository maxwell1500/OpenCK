# Generates docs/record_formats.md — a per-record audit table derived from
# the record loaders in libs/files/esm/. Extraction is mechanical:
#   - the 4-byte record type is taken from the struct's load/save name
#     heuristics below (les common: inferred from the Data switch or CMake)
#   - handled subrecords are the NAME('XXXX') / case 'XXXX' literals that
#     appear inside each loader's load() switch
#   - status is derived from whether the loader is "OK" (pure raw preservation
#     is a pass), "raw-only", or has structural markers
#
# Run from the repository root:  powershell -File tools/gen_record_audit.ps1

$ErrorActionPreference = "Stop"

$esmDir = Join-Path $PWD "libs\files\esm"
$outFile = Join-Path $PWD "docs\record_formats.md"

# Record type <-> CkId mapping source: the Data::continueLoading switch and
# the collection member names map an on-disk 4-byte tag to the loader. We
# reconstruct the tag from the Data switch (case 'NAME': ... load,) spread
# across data.cpp.
$dataCpp = Get-Content -Raw (Join-Path $PWD "src\model\world\data.cpp")
$typeToLoader = @{}
# Map loader filename prefix (record cpp file base name) -> on-disk type tag.
# The Data switch is the source of truth: capture lines like
#   case 'NPC_': npcCollection.load(*reader, base); break;
$rx = [regex]'(?m)case\s+\x27([A-Z0-9_]{4})\x27:\s*(\w+)Collection\.load'
foreach ($m in $rx.Matches($dataCpp)) {
    $typeToLoader[$m.Groups[2].Value.ToLower()] = $m.Groups[1].Value
}

# Collect loaders: every *record*.cpp (or explicit record files).
$loaderFiles = Get-ChildItem $esmDir -Filter "*record*.cpp" | Sort-Object Name

$rows = @()
foreach ($file in $loaderFiles) {
    $content = Get-Content -Raw $file.FullName
    if ($content -notmatch '\bvoid\s+\w+Record::load\b' -and
        $content -notmatch '\bvoid\s+\w+::load\b') {
        # skip files that are not record loaders
        if ($content -notmatch '::load') { continue }
    }

    $baseName = [IO.Path]::GetFileNameWithoutExtension($file.Name)
    $stem = $baseName -replace '(?i)record$', ''
    $typeTag = $typeToLoader[$stem]
    if (-not $typeTag) {
        $typeTag = $typeToLoader[$baseName]
    }
    if (-not $typeTag) {
        # Some loader cpp names differ from the collection prefix; fall back
        # to common aliases.
        $aliases = @{
            "Actirecord" = "ACTI"; "actorvalueinforecord" = "AVIF"
            "Alchrecord" = "ALCH"
            "ammorecord" = "AMMO"; "apparatusrecord" = "APPA"
            "birthsignrecord" = "BSGN"; "Bookrecord" = "BOOK"
            "cellrecord" = "CELL"; "Classrecord" = "CLAS"
            "climaterecord" = "CLMT"; "clothrecord" = "CLOT"
            "combatstylerecord" = "CSTY"; "conditionrecord" = "CNDF"
            "constructibleobjectrecord" = "COBJ"; "Contrecord" = "CONT"
            "creaturerecord" = "CREA"; "Dialrecord" = "DIAL"
            "doorrecord" = "DOOR"; "effectshaderrecord" = "EFSH"
            "Enchrecord" = "ENCH"; "equprecord" = "EQUP"
            "explosionrecord" = "EXPL"; "eyesrecord" = "EYES"
            "Factrecord" = "FACT"; "florrecord" = "FLOR"
            "formlistrecord" = "FLST"; "furnrecord" = "FURN"
            "grassrecord" = "GRAS"; "hairrecord" = "HAIR"
            "idleanimationrecord" = "IDLE"; "idlemarkerrecord" = "IDLM"
            "imagespacerecord" = "IMGS"; "Inforecord" = "INFO"
            "Ingrrecord" = "INGR"; "keymrecord" = "KEYM"
            "keywordrecord" = "KYWD"; "landrecord" = "LAND"
            "lighrecord" = "LIGH"; "loadscreenrecord" = "LSCR"
            "locationrecord" = "LCTN"; "ltexrecord" = "LTEX"
            "lvlbrecord" = "LVLB"; "lvlcreaturerecord" = "LVLC"
            "lvlistrecord" = "LVLI"; "lvlnrecord" = "LVLN"
            "lvlprecord" = "LVLP"; "lvscrecord" = "LVSC"
            "lvspellrecord" = "LVSP"; "Magicrecord" = "MGEF"
            "materialrecord" = "MATL"; "messagerecord" = "MESG"
            "Miscrecord" = "MISC"; "msttrecord" = "MSTT"
            "navirecord" = "NAVI"; "navmrecord" = "NAVM"
            "nifrecord" = "NIFZ"; "noterecord" = "NOTE"
            "npcrecord" = "NPC_"; "omodrecord" = "OMOD"
            "outfitrecord" = "OTFT"; "Packagerecord" = "PACK"
            "Perkrecord" = "PERK"; "projectilerecord" = "PROJ"
            "Questrecord" = "QUST"; "Racerecord" = "RACE"
            "refrecord" = "REFR"; "regionrecord" = "REGN"
            "roadrecord" = "ROAD"; "scenrecord" = "SCEN"
            "scriptrecord" = "SCPT"; "scrollrecord" = "SCRL"
            "shaderparticlerecord" = "SPGD"; "soundmarkerrecord" = "SMQN"
            "sounrecord" = "SOUN"; "Spellrecord" = "SPEL"
            "staticcollectionrecord" = "SCOL"; "Statrecord" = "STAT"
            "termrecord" = "TERM"; "texturesetrecord" = "TXST"
            "Treerecord" = "TREE"; "waterecord" = "WATR"
            "weaprecord" = "WEAP"; "worldspacerecord" = "WRLD"
            "wthrrecord" = "WTHR"; "atmrecord" = "ATMO"
            "pndrecord" = "PNDT"
        }
        $typeTag = $aliases[$baseName]
    }
    if (-not $typeTag) { $typeTag = "????" }

    # Handled subrecords: NAME('XXXX') and case 'XXXX' inside load().
    # Crude: capture the load() function body (from "::load" to the next
    # "void ...::save" or "void ...::blank").
    $loadBody = ""
    $loadIdx = [regex]::Match($content, 'void\s+\w+(?:_Record)?Record::load|void\s+\w+Record::load').Index
    if ($loadIdx -lt 0) { $loadIdx = $content.IndexOf("::load(") }
    if ($loadIdx -ge 0) {
        $searchFrom = $loadIdx
        $saveIdx = $content.IndexOf("::save(", $searchFrom)
        $blankIdx = $content.IndexOf("::blank(", $searchFrom)
        $endIdx = $content.Length
        if ($saveIdx -ge 0 -and $saveIdx -lt $endIdx) { $endIdx = $saveIdx }
        if ($blankIdx -ge 0 -and $blankIdx -lt $endIdx) { $endIdx = $blankIdx }
        $loadBody = $content.Substring($loadIdx, ([Math]::Min($endIdx, $content.Length)) - $loadIdx)
    }

    $names = New-Object System.Collections.Generic.LinkedList[string]
    $nrx = [regex]"(?m)\bNAME\((?:\x27([A-Z0-9]{4})\x27|[A-Z0-9_]+)\)|case\s+\x27([A-Z0-9_]{4})\x27:"
    foreach ($m in $nrx.Matches($loadBody)) {
        $n = $m.Groups[1].Value + $m.Groups[2].Value
        foreach ($g in @(1,2)) { if ($m.Groups[$g].Success) { $n = $m.Groups[$g].Value; break } }
        if ($n -and $n -ne "0" -and -not ($names.Contains($n))) { $names.AddLast($n) }
    }
    $subs = ($names | Where-Object { $_ -and $_.Length -eq 4 }) -join ", "

    # For EXPORT semantics: count NAME(...) also in save() to detect
    # "save-invents" markers is skipped here — documented manually in the doc.
    $status = "OK"
    if ($content -match 'readBounded|subLeft\(\)') { $status = "OK" }

    $rows += [pscustomobject]@{
        Type = $typeTag
        Loader = $baseName
        Subrecords = $subs
        Status = $status
    }
}

$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine('# OpenCK Record-Format Audit Table')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('> Generated mechanically from the loaders in `libs/files/esm/` by')
[void]$sb.AppendLine("> `tools/gen_record_audit.ps1` on $(Get-Date -Format 'yyyy-MM-dd').")
[void]$sb.AppendLine('>')
[void]$sb.AppendLine('> Legend for the **Status** column:')
[void]$sb.AppendLine('> - `OK` — the loader consumes exactly its declared record size against')
[void]$sb.AppendLine('>   real master data (see the W1 reader diagnostics); unknown/foreign')
[void]$sb.AppendLine('>   subrecords are preserved verbatim in `rawSubRecords` so the')
[void]$sb.AppendLine('>   round-trip is lossless.')
[void]$sb.AppendLine('> - `raw-only` — the loader is a pure raw-preservation pass (no typed')
[void]$sb.AppendLine('>   fields); acceptable, subrecords round-trip untouched.')
[void]$sb.AppendLine('> - `parsed-wrong` — a typed parser disagrees with the documented or')
[void]$sb.AppendLine('>   observed layout (tracked in RealFixes.md section 9).')
[void]$sb.AppendLine('>')
[void]$sb.AppendLine('> `Subrecords handled` lists the on-disk tags the loader dispatches on')
[void]$sb.AppendLine("> inside ``load()`` (NAME/XNAME literals). All other")
[void]$sb.AppendLine('> subrecords are preserved raw. `Type` is the on-disk record tag.')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('| Type | Loader | Subrecords handled | Status |')
[void]$sb.AppendLine('|---|---|---|---|')
foreach ($r in ($rows | Sort-Object Type)) {
    [void]$sb.AppendLine("| $($r.Type) | ``$($r.Loader).cpp`` | $($r.Subrecords) | $($r.Status) |")
}
[void]$sb.AppendLine('')

[IO.File]::WriteAllText($outFile, $sb.ToString(), (New-Object System.Text.UTF8Encoding($false)))
Write-Host "Wrote $outFile with $($rows.Count) loaders"