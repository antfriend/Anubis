<#
  pcb-release.ps1 - build a complete, self-consistent JLCPCB fabrication package
  for the Daughterboard and gate it with pcb-check.ps1.

  Produces Daughterboard\fabrication\jlcpcb_<date>_<rev>\
    gerbers\   all copper/mask/paste/silk/edge layers + .gbrjob
    drill\     merged Excellon drill + PDF drill map
    assembly\  kicad pos/bom, JLC CPL (top-assembly), JLC BOM draft carried
               forward from the previous package + reconcile report
    reports\   full ERC and DRC reports (errors + warnings)
  plus the nested JLC upload zip and an outer zip of the whole package,
  then runs pcb-check.ps1 against it. Exit code is nonzero if any gate fails.

  Usage:
    scripts\pcb-release.ps1 -Rev r13
    scripts\pcb-release.ps1 -Rev r13 -Force     # overwrite an existing package dir
#>
param(
    [Parameter(Mandatory = $true)][string]$Rev,
    [string]$ProjectDir,
    [switch]$Force
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\_common.ps1"

if (-not $ProjectDir) { $ProjectDir = $PcbProjectDir }
$PcbFile = Join-Path $ProjectDir "daughterboard.kicad_pcb"
$SchFile = Join-Path $ProjectDir "daughterboard.kicad_sch"
foreach ($f in @($PcbFile, $SchFile)) {
    if (-not (Test-Path $f)) { throw "Not found: $f" }
}

$KicadCli = Resolve-KicadCli
if (-not $KicadCli) { throw "kicad-cli not found. Install with: winget install KiCad.KiCad" }

$FabDir  = Join-Path $ProjectDir "fabrication"
$PkgName = "jlcpcb_{0}_{1}" -f (Get-Date -Format "yyyy-MM-dd"), $Rev
$PkgDir  = Join-Path $FabDir $PkgName
if (Test-Path $PkgDir) {
    if ($Force) { Remove-Item $PkgDir -Recurse -Force }
    else { throw "$PkgDir already exists. Use -Force to overwrite." }
}

# Previous package supplies the JLC BOM (LCSC mapping) to carry forward.
$PrevPkg = Get-ChildItem $FabDir -Directory -Filter "jlcpcb_*" -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -ne $PkgName } | Sort-Object Name -Descending | Select-Object -First 1

$GerberDir = Join-Path $PkgDir "gerbers"
$DrillDir  = Join-Path $PkgDir "drill"
$AsmDir    = Join-Path $PkgDir "assembly"
$RptDir    = Join-Path $PkgDir "reports"
foreach ($d in @($GerberDir, $DrillDir, $AsmDir, $RptDir)) { New-Item -ItemType Directory -Path $d -Force | Out-Null }

Write-Host "==> Building fabrication package $PkgName" -ForegroundColor Cyan
Write-Host "    kicad-cli: $KicadCli" -ForegroundColor DarkGray

function Invoke-Kicad {
    param([string]$What, [string[]]$KicadArgs)
    & $KicadCli @KicadArgs | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "$What failed (kicad-cli exit $LASTEXITCODE)" }
    Write-Host "    $What" -ForegroundColor DarkGray
}

# ---- Gerbers, drill, reports -----------------------------------------------
# --board-plot-params reuses the plot setup saved in the board file. Note the
# output uses KiCad's native .gbr extensions (r12 and older used Protel ones).
Invoke-Kicad "gerbers exported" @("pcb", "export", "gerbers", "--board-plot-params", "--output", "$GerberDir\", $PcbFile)
Invoke-Kicad "drill exported" @("pcb", "export", "drill", "--excellon-units", "mm", "--excellon-zeros-format", "decimal", "--generate-map", "--map-format", "pdf", "--output", "$DrillDir\", $PcbFile)
Invoke-Kicad "ERC report written" @("sch", "erc", "--output", (Join-Path $RptDir "Daughterboard_erc_report.txt"), $SchFile)
Invoke-Kicad "DRC report written" @("pcb", "drc", "--schematic-parity", "--output", (Join-Path $RptDir "Daughterboard_drc_report.txt"), $PcbFile)

# ---- Assembly outputs -------------------------------------------------------
$PosCsv = Join-Path $AsmDir "Daughterboard_kicad_pos.csv"
Invoke-Kicad "position file exported" @("pcb", "export", "pos", "--format", "csv", "--units", "mm", "--exclude-dnp", "--output", $PosCsv, $PcbFile)
Invoke-Kicad "kicad BOM exported" @("sch", "export", "bom", "--output", (Join-Path $AsmDir "Daughterboard_kicad_bom.csv"), $SchFile)

# Carry the LCSC-mapped JLC BOM forward from the previous package; new or removed
# parts must be reconciled by hand (see the reconcile report).
$inv = [System.Globalization.CultureInfo]::InvariantCulture
$bomRefs = @{}
$JlcBom = $null
if ($PrevPkg) {
    $prevBom = Get-ChildItem (Join-Path $PrevPkg.FullName "assembly") -File -Filter "*bom_jlc_ready*.csv" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($prevBom) {
        $JlcBom = Join-Path $AsmDir "Daughterboard_jlcpcb_bom_jlc_ready_draft.csv"
        Copy-Item $prevBom.FullName $JlcBom
        Write-Host "    JLC BOM carried forward from $($PrevPkg.Name)" -ForegroundColor DarkGray
        foreach ($row in (Import-Csv $JlcBom)) {
            foreach ($tok in ($row.Designator -split ",")) {
                $t = $tok.Trim()
                if ($t -match '^([A-Za-z]+)(\d+)-[A-Za-z]*(\d+)$') {
                    for ($i = [int]$Matches[2]; $i -le [int]$Matches[3]; $i++) { $bomRefs[$Matches[1] + $i] = $true }
                } elseif ($t) { $bomRefs[$t] = $true }
            }
        }
    }
}
if (-not $JlcBom) {
    Write-Host "    WARNING: no previous JLC BOM found to carry forward - create one by hand." -ForegroundColor Yellow
}

# Build JLC CPLs from the position file: Mid Y is sign-flipped from KiCad PosY.
$posRows = Import-Csv $PosCsv
function ConvertTo-CplRow {
    param($Row)
    [pscustomobject]@{
        "Designator" = $Row.Ref
        "Mid X"      = ([double]::Parse($Row.PosX, $inv)).ToString("0.0000", $inv)
        "Mid Y"      = (-[double]::Parse($Row.PosY, $inv)).ToString("0.0000", $inv)
        "Layer"      = $inv.TextInfo.ToTitleCase($Row.Side)
        "Rotation"   = ([double]::Parse($Row.Rot, $inv)).ToString("0.00", $inv)
    }
}
$cplAll = $posRows | ForEach-Object { ConvertTo-CplRow $_ }
$cplAll | Export-Csv (Join-Path $AsmDir "Daughterboard_jlcpcb_cpl_all.csv") -NoTypeInformation
if ($bomRefs.Count -gt 0) {
    $cplAll | Where-Object { $bomRefs.ContainsKey($_.Designator) } |
        Export-Csv (Join-Path $AsmDir "Daughterboard_jlcpcb_cpl.csv") -NoTypeInformation
}

# Reconcile carried BOM against what is actually on the board now.
if ($bomRefs.Count -gt 0) {
    $posRefs = @{}
    foreach ($r in $posRows) { $posRefs[$r.Ref] = $true }
    $bomOnly = @($bomRefs.Keys | Where-Object { -not $posRefs.ContainsKey($_) } | Sort-Object)
    $posOnly = @($posRefs.Keys | Where-Object { -not $bomRefs.ContainsKey($_) } | Sort-Object)
    $lines = @("BOM reconcile report - $PkgName - $(Get-Date -Format s)", "")
    $lines += "In carried JLC BOM but NOT on the current board (delete from BOM or investigate):"
    $lines += $(if ($bomOnly) { $bomOnly | ForEach-Object { "  $_" } } else { "  (none)" })
    $lines += ""
    $lines += "On the board but NOT in the JLC BOM (add LCSC mapping, or confirm hand-solder):"
    $lines += $(if ($posOnly) { $posOnly | ForEach-Object { "  $_" } } else { "  (none)" })
    $rpt = Join-Path $AsmDir "Daughterboard_bom_reconcile_report.txt"
    Set-Content $rpt ($lines -join "`r`n") -Encoding utf8
    if ($bomOnly -or $posOnly) {
        Write-Host "    BOM reconcile: $($bomOnly.Count) stale BOM ref(s), $($posOnly.Count) unmapped board ref(s) - review $rpt" -ForegroundColor Yellow
    } else {
        Write-Host "    BOM reconcile: clean" -ForegroundColor DarkGray
    }
}

# ---- Zips --------------------------------------------------------------------
# .NET ZipFile instead of Compress-Archive: the PS 5.1 module can hold a handle on
# a just-written zip, which breaks packing it into the outer archive.
Add-Type -AssemblyName System.IO.Compression.FileSystem
$uploadZip = Join-Path $PkgDir "Daughterboard_jlcpcb_gerber_drill.zip"
$zip = [System.IO.Compression.ZipFile]::Open($uploadZip, "Create")
try {
    foreach ($f in (@(Get-ChildItem $GerberDir -File) + @(Get-ChildItem $DrillDir -File))) {
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $f.FullName, $f.Name) | Out-Null
    }
} finally { $zip.Dispose() }
Write-Host "    upload zip written" -ForegroundColor DarkGray

$outerZip = Join-Path $FabDir "$PkgName.zip"
if (Test-Path $outerZip) { Remove-Item $outerZip -Force }
[System.IO.Compression.ZipFile]::CreateFromDirectory($PkgDir, $outerZip)
Write-Host "    package zip written" -ForegroundColor DarkGray

# ---- Gate the fresh package ---------------------------------------------------
Write-Host ""
& "$PSScriptRoot\pcb-check.ps1" -ProjectDir $ProjectDir -PackageDir $PkgDir
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "Package built at $PkgDir but FAILED release gates - do not upload it." -ForegroundColor Red
    exit 1
}
Write-Host ""
Write-Host "Release package ready: $PkgDir" -ForegroundColor Green
