<#
.SYNOPSIS
    Checks that the medical-style documents match the code and the tests.

.DESCRIPTION
    Reads docs/medical/*.md, the RFP source and tests/**/*.cpp and fails if a link is broken.
    The list of checks is in docs/medical/README.md. Links are checked, not whether a test
    really proves its requirement: that still takes a review.

    Works with Windows PowerShell 5.1 and PowerShell 7 (Windows and Linux).

.PARAMETER Root
    Repository root. Default: the parent of this script's folder.
#>
[CmdletBinding()]
param(
    [string] $Root
)

$ErrorActionPreference = 'Stop'
# Not a parameter default: $PSScriptRoot is empty there in Windows PowerShell 5.1.
if (-not $Root) { $Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path) }
$script:problems = New-Object System.Collections.Generic.List[string]

function Add-Problem([string] $message) { $script:problems.Add($message) }

function Read-Text([string] $relativePath) {
    $path = Join-Path $Root $relativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing file: $relativePath"
    }
    return [System.IO.File]::ReadAllLines($path, [System.Text.Encoding]::UTF8)
}

# Rows of Markdown tables as arrays of trimmed cells.
function Get-TableRows([string[]] $lines) {
    foreach ($line in $lines) {
        $trimmed = $line.Trim()
        if (-not $trimmed.StartsWith('|') -or $trimmed -match '^\|[\s:\-|]+\|$') { continue }
        $cells = $trimmed.Trim('|').Split('|') | ForEach-Object { $_.Trim() }
        , @($cells)
    }
}

function Split-List([string] $cell) {
    if ($cell -eq '-' -or $cell -eq '') { return @() }
    return @($cell.Split(',') | ForEach-Object { $_.Trim() })
}

# --- Read everything ----------------------------------------------------------------------------
$srsLines = Read-Text 'docs/medical/SOFTWARE-REQUIREMENTS.md'
$riskLines = Read-Text 'docs/medical/RISK-ANALYSIS.md'
$traceLines = Read-Text 'docs/medical/TRACEABILITY.md'
$rfpLines = Read-Text 'Documents/source/en/01-RFP.md'

$requirements = @{}
$manualTests = @{}
foreach ($row in (Get-TableRows $srsLines)) {
    if ($row[0] -match '^SRS-\d{3}$') {
        if ($requirements.ContainsKey($row[0])) { Add-Problem "$($row[0]) is defined twice" }
        $requirements[$row[0]] = $row[1]
    }
    elseif ($row[0] -match '^MT-\d{2}$') {
        $manualTests[$row[0]] = $true
    }
}

$risks = @{}
$openRiskNotes = @{}
foreach ($row in (Get-TableRows $riskLines)) {
    if ($row[0] -notmatch '^R-\d{2}$') { continue }
    if ($row.Count -eq 3) { $openRiskNotes[$row[0]] = $true; continue } # table of open risks
    if ($row.Count -ne 11) { Add-Problem "$($row[0]) must have 11 columns, found $($row.Count)"; continue }
    if ($risks.ContainsKey($row[0])) { Add-Problem "$($row[0]) is defined twice" }
    $risks[$row[0]] = @{
        S = [int]$row[3]; P = [int]$row[4]; Score = [int]$row[5]
        Controls = Split-List $row[6]
        SAfter = [int]$row[7]; PAfter = [int]$row[8]; Residual = [int]$row[9]
        Status = $row[10]
    }
}

$trace = @{}
$notImplemented = @{}
foreach ($row in (Get-TableRows $traceLines)) {
    if ($row[0] -match '^SRS-\d{3}$') {
        if ($row.Count -ne 5) { Add-Problem "$($row[0]) in the matrix must have 5 columns, found $($row.Count)"; continue }
        if ($trace.ContainsKey($row[0])) { Add-Problem "$($row[0]) is in the matrix twice" }
        $trace[$row[0]] = @{
            Sources = Split-List $row[1]; Risks = Split-List $row[2]
            Code = Split-List $row[3]; Verification = Split-List $row[4]
        }
    }
    elseif ($row[0] -match '^FR-[A-Z]+-\d{2}$') {
        $notImplemented[$row[0]] = $true
    }
}

$tests = @{}
$testRoot = Join-Path $Root 'tests'
foreach ($file in (Get-ChildItem -LiteralPath $testRoot -Recurse -Filter '*.cpp')) {
    $text = [System.IO.File]::ReadAllText($file.FullName)
    foreach ($match in [regex]::Matches($text, '(?m)^TEST(?:_F|_P)?\(\s*(\w+)\s*,\s*(\w+)\s*\)')) {
        $tests["$($match.Groups[1].Value).$($match.Groups[2].Value)"] = $true
    }
}

$rfpItems = @{}
foreach ($row in (Get-TableRows $rfpLines)) {
    if ($row[0] -match '^FR-[A-Z]+-\d{2}$') { $rfpItems[$row[0]] = $true }
}

# --- Requirements <-> matrix ----------------------------------------------------------------------
foreach ($id in $requirements.Keys) {
    if (-not $trace.ContainsKey($id)) { Add-Problem "$id has no row in the matrix" }
}
foreach ($id in $trace.Keys) {
    if (-not $requirements.ContainsKey($id)) { Add-Problem "$id is in the matrix but is not a requirement" }
}

# --- Each matrix row ----------------------------------------------------------------------------
$tracedFr = @{}
foreach ($id in $trace.Keys) {
    $entry = $trace[$id]

    if ($entry.Sources.Count -eq 0 -and $entry.Risks.Count -eq 0) {
        Add-Problem "$id has neither a source nor a risk"
    }
    foreach ($source in $entry.Sources) {
        if (-not $rfpItems.ContainsKey($source)) { Add-Problem "$id names $source, which is not in the RFP" }
        $tracedFr[$source] = $true
    }
    foreach ($risk in $entry.Risks) {
        if (-not $risks.ContainsKey($risk)) { Add-Problem "$id names $risk, which is not in the risk table" }
        elseif ($risks[$risk].Controls -notcontains $id) { Add-Problem "$id names $risk, but $risk does not list $id as a control" }
    }
    if ($entry.Code.Count -eq 0) { Add-Problem "$id names no code" }
    foreach ($path in $entry.Code) {
        if (-not (Test-Path -LiteralPath (Join-Path $Root $path))) { Add-Problem "$id names $path, which does not exist" }
    }
    if ($entry.Verification.Count -eq 0) { Add-Problem "$id has no verification" }
    foreach ($item in $entry.Verification) {
        if ($item -match '^MT-\d{2}$') {
            if (-not $manualTests.ContainsKey($item)) { Add-Problem "$id names $item, which is not defined" }
        }
        elseif (-not $tests.ContainsKey($item)) {
            Add-Problem "$id names the test $item, which does not exist"
        }
    }
}

# --- Each risk ----------------------------------------------------------------------------------
foreach ($id in $risks.Keys) {
    $risk = $risks[$id]
    if ($risk.Score -ne $risk.S * $risk.P) { Add-Problem "$id score $($risk.Score) is not $($risk.S) x $($risk.P)" }
    if ($risk.Residual -ne $risk.SAfter * $risk.PAfter) { Add-Problem "$id residual $($risk.Residual) is not $($risk.SAfter) x $($risk.PAfter)" }
    if ($risk.SAfter -ne $risk.S) { Add-Problem "$id changes the severity: software controls lower the probability only" }
    if ($risk.PAfter -gt $risk.P) { Add-Problem "$id gets more probable after its controls" }
    if ($risk.Residual -ge 9) { Add-Problem "$id stays unacceptable after its controls (residual $($risk.Residual))" }
    if ($risk.Status -ne 'Controlled' -and $risk.Status -ne 'Open') { Add-Problem "$id has the unknown status '$($risk.Status)'" }
    if ($risk.Controls.Count -eq 0 -and $risk.Status -ne 'Open') { Add-Problem "$id has no control and is not open" }
    foreach ($control in $risk.Controls) {
        if (-not $requirements.ContainsKey($control)) { Add-Problem "$id names $control, which is not a requirement" }
        elseif ($trace.ContainsKey($control) -and $trace[$control].Risks -notcontains $id) {
            Add-Problem "$id lists $control as a control, but the matrix row of $control does not name $id"
        }
    }
    if ($risk.Status -eq 'Open' -and -not $openRiskNotes.ContainsKey($id)) {
        Add-Problem "$id is open but missing from the table of open risks"
    }
    if ($risk.Status -ne 'Open' -and $openRiskNotes.ContainsKey($id)) {
        Add-Problem "$id is in the table of open risks but its status is $($risk.Status)"
    }
}

# --- RFP coverage -------------------------------------------------------------------------------
foreach ($item in $rfpItems.Keys) {
    if (-not $tracedFr.ContainsKey($item) -and -not $notImplemented.ContainsKey($item)) {
        Add-Problem "$item of the RFP is neither traced nor listed as not implemented"
    }
    if ($tracedFr.ContainsKey($item) -and $notImplemented.ContainsKey($item)) {
        Add-Problem "$item is traced and also listed as not implemented"
    }
}

# --- Result -------------------------------------------------------------------------------------
if ($script:problems.Count -gt 0) {
    Write-Host "Traceability check FAILED with $($script:problems.Count) problem(s):"
    $script:problems | Sort-Object | ForEach-Object { Write-Host "  $_" }
    exit 1
}
$manualOnly = @($trace.Keys | Where-Object { @($trace[$_].Verification | Where-Object { $_ -notmatch '^MT-' }).Count -eq 0 }).Count
$open = @($risks.Keys | Where-Object { $risks[$_].Status -eq 'Open' }).Count
Write-Host "Traceability check passed: $($requirements.Count) requirements, $($risks.Count) risks ($open open), $($tests.Count) tests, $($rfpItems.Count) RFP items, $manualOnly requirements verified by manual tests only."
