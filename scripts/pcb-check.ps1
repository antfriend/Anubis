<#
  pcb-check.ps1 - pre-fabrication release gates for the Daughterboard KiCad project.

  Runs every gate it can and prints a PASS/FAIL summary; exits nonzero if any gate fails.
  Gates:
    1. ERC             (kicad-cli)  schematic errors
    2. DRC + parity    (kicad-cli)  board errors, unconnected pads, schematic parity
    3. Gerber freshness(kicad-cli)  re-export and diff against the packaged gerbers
    4. Package integrity            required layer files, nested upload zip matches loose files
    5. Fab metadata                 gbrjob: layer count, revision set, finish set
    6. BOM/CPL consistency          every placed part has an LCSC number, designators reconcile

  Usage:
    scripts\pcb-check.ps1                          # newest fabrication\jlcpcb_* package
    scripts\pcb-check.ps1 -PackageDir <path>       # check a specific package
    scripts\pcb-check.ps1 -StaticOnly              # skip gates that need kicad-cli
#>
param(
    [string]$ProjectDir,
    [string]$PackageDir,
    [switch]$StaticOnly
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\_common.ps1"

if (-not $ProjectDir) { $ProjectDir = $PcbProjectDir }
$PcbFile = Join-Path $ProjectDir "daughterboard.kicad_pcb"
$SchFile = Join-Path $ProjectDir "daughterboard.kicad_sch"
foreach ($f in @($PcbFile, $SchFile)) {
    if (-not (Test-Path $f)) { throw "Not found: $f" }
}

if (-not $PackageDir) {
    $fabDir = Join-Path $ProjectDir "fabrication"
    $PackageDir = Get-ChildItem $fabDir -Directory -Filter "jlcpcb_*" -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending | Select-Object -First 1 -ExpandProperty FullName
    if (-not $PackageDir) { throw "No fabrication\jlcpcb_* package found under $fabDir." }
}
Write-Host "==> Checking package: $PackageDir" -ForegroundColor Cyan
Write-Host "    Board: $PcbFile" -ForegroundColor DarkGray

$script:Failures = 0
function Report {
    param([string]$Gate, [ValidateSet("PASS","FAIL","WARN","SKIP")][string]$Status, [string]$Detail)
    $color = switch ($Status) { "PASS" {"Green"} "FAIL" {"Red"} "WARN" {"Yellow"} "SKIP" {"DarkYellow"} }
    Write-Host ("[{0}] {1}" -f $Status, $Gate) -ForegroundColor $color
    if ($Detail) { Write-Host ("       {0}" -f ($Detail -replace "`n", "`n       ")) -ForegroundColor DarkGray }
    if ($Status -eq "FAIL") { $script:Failures++ }
}

# Strip volatile timestamp lines so re-exported fab files can be compared to packaged ones.
function Get-NormalizedFabContent {
    param([string]$Path)
    # TF.GenerationSoftware carries the KiCad version; TF.ProjectId's name/GUID derive
    # from the exporting filename, not board content. Neither means the board changed.
    (Get-Content $Path | Where-Object {
        $_ -notmatch 'TF\.CreationDate' -and
        $_ -notmatch 'TF\.GenerationSoftware' -and
        $_ -notmatch 'TF\.ProjectId' -and
        $_ -notmatch '^G04 Created by KiCad' -and
        $_ -notmatch '^; DRILL file KiCad'
    }) -join "`n"
}

# Expand a JLC BOM designator field like "C4,C24-C26,R21-R38" into individual refs.
function Expand-Designators {
    param([string]$Field)
    $out = @()
    foreach ($tok in ($Field -split ",")) {
        $t = $tok.Trim()
        if (-not $t) { continue }
        if ($t -match '^([A-Za-z]+)(\d+)-([A-Za-z]*)(\d+)$') {
            if ($Matches[3] -and $Matches[3] -ne $Matches[1]) { throw "Mixed-prefix designator range '$t'" }
            for ($i = [int]$Matches[2]; $i -le [int]$Matches[4]; $i++) { $out += ($Matches[1] + $i) }
        } elseif ($t -match '^[A-Za-z]+\d+$') {
            $out += $t
        } else {
            throw "Unparseable designator '$t'"
        }
    }
    return $out
}

function Get-ZipEntrySha256 {
    param($Entry)
    $sha = [System.Security.Cryptography.SHA256]::Create()
    $s = $Entry.Open()
    try { $hash = $sha.ComputeHash($s) } finally { $s.Dispose(); $sha.Dispose() }
    return ([BitConverter]::ToString($hash) -replace '-', '')
}

$KicadCli = Resolve-KicadCli
if (-not $KicadCli -and -not $StaticOnly) {
    Report "kicad-cli available" "FAIL" "kicad-cli not found. Install with: winget install KiCad.KiCad  (or rerun with -StaticOnly)"
} elseif ($KicadCli) {
    Write-Host "    kicad-cli: $KicadCli" -ForegroundColor DarkGray
}

$TmpDir = Join-Path $env:TEMP ("pcb-check-" + [guid]::NewGuid().ToString("N").Substring(0, 8))
New-Item -ItemType Directory -Path $TmpDir | Out-Null

try {
    # ---- Gate 1: ERC -------------------------------------------------------
    if ($KicadCli) {
        $ercOut = Join-Path $TmpDir "erc.rpt"
        & $KicadCli sch erc --output $ercOut --severity-error --exit-code-violations $SchFile | Out-Null
        if ($LASTEXITCODE -eq 0) {
            Report "ERC (schematic errors)" "PASS"
        } else {
            $tail = ""
            if (Test-Path $ercOut) { $tail = (Get-Content $ercOut | Select-Object -Last 12) -join "`n" }
            Report "ERC (schematic errors)" "FAIL" "kicad-cli exit $LASTEXITCODE`n$tail"
        }
    } else {
        Report "ERC (schematic errors)" "SKIP" "needs kicad-cli"
    }

    # ---- Gate 2: DRC + schematic parity ------------------------------------
    if ($KicadCli) {
        $drcOut = Join-Path $TmpDir "drc.rpt"
        & $KicadCli pcb drc --output $drcOut --severity-error --schematic-parity --exit-code-violations $PcbFile | Out-Null
        if ($LASTEXITCODE -eq 0) {
            Report "DRC + schematic parity" "PASS"
        } else {
            $tail = ""
            if (Test-Path $drcOut) {
                $tail = (Get-Content $drcOut | Select-String -Pattern '^\[|Found \d+' | Select-Object -First 15 | ForEach-Object { $_.Line }) -join "`n"
            }
            Report "DRC + schematic parity" "FAIL" "kicad-cli exit $LASTEXITCODE`n$tail"
        }
    } else {
        Report "DRC + schematic parity" "SKIP" "needs kicad-cli"
    }

    # ---- Gate 3: Gerber/drill freshness -------------------------------------
    $pkgGerberDir = Join-Path $PackageDir "gerbers"
    $pkgDrillDir  = Join-Path $PackageDir "drill"
    if ($KicadCli) {
        $freshDir = Join-Path $TmpDir "fresh"
        New-Item -ItemType Directory -Path $freshDir | Out-Null
        # --board-plot-params reuses the plot setup saved in the board file, so filenames
        # and options match whatever produced the package.
        & $KicadCli pcb export gerbers --board-plot-params --output "$freshDir\" $PcbFile | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "gerber export failed (exit $LASTEXITCODE)" }
        & $KicadCli pcb export drill --excellon-units mm --excellon-zeros-format decimal --output "$freshDir\" $PcbFile | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "drill export failed (exit $LASTEXITCODE)" }

        $stale = @()
        $missing = @()
        # The .gbrjob is all metadata (versions, dates, sizes) and is linted by gate 5.
        $packaged = @(Get-ChildItem $pkgGerberDir -File | Where-Object { $_.Extension -ne ".gbrjob" }) + @(Get-ChildItem $pkgDrillDir -File -Filter *.drl)
        # The package uses Protel extensions (.gtl/.gbl/...), kicad-cli emits .gbr; match
        # on the extension-less stem (e.g. "daughterboard-f_cu") instead of the full name.
        foreach ($pf in $packaged) {
            $stem = [IO.Path]::GetFileNameWithoutExtension($pf.Name)
            $ff = Get-ChildItem $freshDir -File | Where-Object {
                [IO.Path]::GetFileNameWithoutExtension($_.Name) -ieq $stem -and
                ($_.Extension -ieq $pf.Extension -or $_.Extension -ieq ".gbr")
            } | Select-Object -First 1
            if (-not $ff) { $missing += $pf.Name; continue }
            if ((Get-NormalizedFabContent $pf.FullName) -cne (Get-NormalizedFabContent $ff.FullName)) { $stale += $pf.Name }
        }
        if ($stale.Count -eq 0 -and $missing.Count -eq 0) {
            Report "Gerber/drill freshness" "PASS" ("{0} files match current board" -f $packaged.Count)
        } elseif ($missing.Count -gt 0 -and $stale.Count -eq 0) {
            Report "Gerber/drill freshness" "WARN" ("re-export did not produce: {0} (plot settings drift?)" -f ($missing -join ", "))
        } else {
            Report "Gerber/drill freshness" "FAIL" ("package differs from current board: {0}{1}" -f ($stale -join ", "), $(if ($missing) { "; not re-produced: " + ($missing -join ", ") } else { "" }))
        }
    } else {
        Report "Gerber/drill freshness" "SKIP" "needs kicad-cli"
    }

    # ---- Gate 4: Package integrity ------------------------------------------
    $requiredGerbers = @("F_Cu", "In1_Cu", "In2_Cu", "B_Cu", "F_Paste", "B_Paste",
                         "F_Silkscreen", "B_Silkscreen", "F_Mask", "B_Mask", "Edge_Cuts", "job")
    $probs = @()
    foreach ($layer in $requiredGerbers) {
        $hit = Get-ChildItem $pkgGerberDir -File -Filter "*-$layer.*" -ErrorAction SilentlyContinue
        if (-not $hit) { $probs += "missing gerber layer: $layer" }
    }
    $drl = Get-ChildItem $pkgDrillDir -File -Filter *.drl -ErrorAction SilentlyContinue
    if (-not $drl) { $probs += "missing drill file" }
    elseif ($drl[0].Length -lt 500) { $probs += "drill file suspiciously small ($($drl[0].Length) bytes)" }

    $uploadZip = Get-ChildItem $PackageDir -File -Filter "*gerber_drill.zip" -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $uploadZip) {
        $probs += "missing nested upload zip (*gerber_drill.zip)"
    } else {
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        $zip = [System.IO.Compression.ZipFile]::OpenRead($uploadZip.FullName)
        try {
            $loose = @{}
            foreach ($f in (@(Get-ChildItem $pkgGerberDir -File) + @(Get-ChildItem $pkgDrillDir -File))) {
                $loose[$f.Name.ToLower()] = (Get-FileHash $f.FullName -Algorithm SHA256).Hash
            }
            foreach ($entry in ($zip.Entries | Where-Object { $_.Name })) {
                $key = $entry.Name.ToLower()
                if (-not $loose.ContainsKey($key)) { $probs += "zip-only file: $($entry.Name)"; continue }
                if ((Get-ZipEntrySha256 $entry) -ne $loose[$key]) { $probs += "zip content differs: $($entry.Name)" }
                $loose.Remove($key)
            }
            foreach ($left in $loose.Keys) { $probs += "not in upload zip: $left" }
        } finally { $zip.Dispose() }
    }
    if ($probs.Count -eq 0) { Report "Package integrity" "PASS" "all layers present; upload zip matches loose files" }
    else { Report "Package integrity" "FAIL" ($probs -join "`n") }

    # ---- Gate 5: Fab metadata ------------------------------------------------
    $gbrjob = Get-ChildItem $pkgGerberDir -File -Filter *.gbrjob -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $gbrjob) {
        Report "Fab metadata (gbrjob)" "FAIL" "no .gbrjob in package"
    } else {
        $job = Get-Content $gbrjob.FullName -Raw | ConvertFrom-Json
        $meta = @()
        if ($job.GeneralSpecs.LayerNumber -ne 4) { $meta += "expected 4 copper layers, gbrjob says $($job.GeneralSpecs.LayerNumber)" }
        $rev = $job.GeneralSpecs.ProjectId.Revision
        if (-not $rev -or $rev -eq "rev?") { $meta += "board revision not set (title block) - currently '$rev'" }
        if (-not $job.GeneralSpecs.Finish -or $job.GeneralSpecs.Finish -eq "None") { $meta += "copper finish not set in board stackup - currently '$($job.GeneralSpecs.Finish)'" }
        $size = "{0} x {1} mm" -f $job.GeneralSpecs.Size.X, $job.GeneralSpecs.Size.Y
        if ($meta.Count -eq 0) { Report "Fab metadata (gbrjob)" "PASS" "rev '$rev', finish '$($job.GeneralSpecs.Finish)', $size" }
        else { Report "Fab metadata (gbrjob)" "FAIL" (($meta -join "`n") + "`nboard size: $size") }
    }

    # ---- Gate 6: BOM / CPL consistency ----------------------------------------
    $asmDir = Join-Path $PackageDir "assembly"
    $bomCsv = Get-ChildItem $asmDir -File -Filter "*bom_jlc_ready*.csv" -ErrorAction SilentlyContinue | Select-Object -First 1
    $cplCsv = Get-ChildItem $asmDir -File -Filter "*_cpl.csv" -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $bomCsv -or -not $cplCsv) {
        Report "BOM/CPL consistency" "FAIL" "missing $(if (-not $bomCsv) {'JLC BOM csv '})$(if (-not $cplCsv) {'CPL csv'}) in $asmDir"
    } else {
        $bomProbs = @()
        $bomRefs = @{}
        foreach ($row in (Import-Csv $bomCsv.FullName)) {
            $refs = Expand-Designators $row.Designator
            if ([int]$row.Quantity -ne $refs.Count) { $bomProbs += "row '$($row.Comment)': Quantity=$($row.Quantity) but $($refs.Count) designators" }
            if (-not $row.'LCSC Part #') { $bomProbs += "row '$($row.Comment)' ($($row.Designator)): empty LCSC part number" }
            foreach ($r in $refs) {
                if ($bomRefs.ContainsKey($r)) { $bomProbs += "designator $r appears in multiple BOM rows" }
                $bomRefs[$r] = $true
            }
        }
        $cplRows = Import-Csv $cplCsv.FullName
        $notInBom = @($cplRows | Where-Object { -not $bomRefs.ContainsKey($_.Designator) } | ForEach-Object { $_.Designator })
        $cplRefs = @{}
        foreach ($row in $cplRows) { $cplRefs[$row.Designator] = $true }
        $notInCpl = @($bomRefs.Keys | Where-Object { -not $cplRefs.ContainsKey($_) })
        $bottom = @($cplRows | Where-Object { $_.Layer -ne "Top" } | ForEach-Object { $_.Designator })
        $badRot = @($cplRows | Where-Object { -not ($_.Rotation -match '^-?\d+(\.\d+)?$') } | ForEach-Object { $_.Designator })

        if ($notInBom) { $bomProbs += "in CPL but not in BOM (JLC cannot place): " + ($notInBom -join ", ") }
        if ($badRot) { $bomProbs += "non-numeric rotation: " + ($badRot -join ", ") }
        if ($bottom) { $bomProbs += "bottom-side placements (board is top-assembly only): " + ($bottom -join ", ") }

        $note = "BOM refs: $($bomRefs.Count); CPL placements: $($cplRows.Count)"
        if ($notInCpl) { $note += "`nin BOM but not placed by CPL (verify hand-solder intent): " + ($notInCpl -join ", ") }
        if ($bomProbs.Count -eq 0) {
            if ($notInCpl) { Report "BOM/CPL consistency" "WARN" $note } else { Report "BOM/CPL consistency" "PASS" $note }
        } else {
            Report "BOM/CPL consistency" "FAIL" (($bomProbs -join "`n") + "`n" + $note)
        }
    }
} finally {
    Remove-Item $TmpDir -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host ""
if ($Failures -eq 0) {
    Write-Host "All gates passed." -ForegroundColor Green
    exit 0
} else {
    Write-Host "$Failures gate(s) FAILED - do not send this package to fabrication." -ForegroundColor Red
    exit 1
}
