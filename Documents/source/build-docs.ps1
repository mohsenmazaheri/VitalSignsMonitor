<#
.SYNOPSIS
    Builds the project documents (Word .docx) from the Markdown-like sources in this folder.

.DESCRIPTION
    Each source file (en/*.md, fa/*.md) starts with a front-matter block (title, version, output, ...)
    followed by a small Markdown subset:

        # / ## / ###          headings (numbering is written in the text)
        - item                bullet list
        1. item               numbered list (restarts after any other block)
        | a | b |             table; the first row is the header, |---| rows are skipped
        ```                   code block (always left-to-right, monospace)
        > text                note box
        ![caption](path)      picture (path relative to the repository root) + caption
        {{pagebreak}}         page break
        **bold**, `code`      inline formatting

    The documents are produced with Microsoft Word through COM automation, so Word must be installed.
    Persian sources (lang: fa) get right-to-left paragraphs, tables and lists.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File Documents\source\build-docs.ps1
    powershell -ExecutionPolicy Bypass -File Documents\source\build-docs.ps1 -Only fa\01-RFP.md
#>
[CmdletBinding()]
param(
    # Build only these sources (paths relative to this folder). Default: all en/*.md and fa/*.md.
    [string[]] $Only,
    # Optional folder for PDF copies, e.g. to check the layout. No PDFs are written by default.
    [string] $PdfDir
)

$ErrorActionPreference = 'Stop'
$sourceDir = $PSScriptRoot
$docsDir = Split-Path $sourceDir -Parent
$repoRoot = Split-Path $docsDir -Parent

# --- Word constants ------------------------------------------------------------------------------
$wdStyleNormal = -1; $wdStyleHeading1 = -2; $wdStyleHeading2 = -3; $wdStyleHeading3 = -4
$wdStyleTitle = -63; $wdStyleSubtitle = -75; $wdStyleCaption = -35
$wdStyleListBullet = -49; $wdStyleListNumber = -50; $wdStyleToc1 = -20; $wdStyleToc2 = -21
$wdStyleHeader = -32; $wdStyleFooter = -33
$wdStyleTypeParagraph = 1
$wdAlignLeft = 0; $wdAlignCenter = 1; $wdAlignRight = 2; $wdAlignJustify = 3
$wdReadingOrderRtl = 0; $wdReadingOrderLtr = 1
$wdTableDirectionRtl = 0
$wdStory = 6; $wdPageBreak = 7; $wdFieldPage = 33
$wdFormatDocx = 16; $wdFormatPdf = 17
$wdLineSpaceMultiple = 5
$wdHeaderPrimary = 1

function Rgb([int] $r, [int] $g, [int] $b) { return $r + 256 * $g + 65536 * $b }

$colorAccent = Rgb 0 92 120      # headings, header row text
$colorHeaderFill = Rgb 223 236 240
$colorCodeFill = Rgb 243 245 247
$colorCodeBorder = Rgb 208 215 222
$colorNoteFill = Rgb 255 248 225
$colorNoteBorder = Rgb 230 180 60
$colorMuted = Rgb 90 100 110
$colorInlineCode = Rgb 150 40 40

# Language-dependent labels. Persian strings are kept here so the sources stay free of layout words.
$labels = @{
    en = @{
        contents = 'Contents'; project = 'Project'; document = 'Document'; docId = 'Document ID'
        version = 'Version'; date = 'Date'; author = 'Prepared by'; status = 'Status'
        history = 'Revision history'; histVersion = 'Version'; histDate = 'Date'; histAuthor = 'Author'
        histChange = 'Change'; page = 'Page'
    }
    fa = @{
        contents = 'فهرست مطالب'; project = 'پروژه'; document = 'سند'; docId = 'شناسه سند'
        version = 'نسخه'; date = 'تاریخ'; author = 'تهیه‌کننده'; status = 'وضعیت'
        history = 'تاریخچه تغییرات'; histVersion = 'نسخه'; histDate = 'تاریخ'; histAuthor = 'نویسنده'
        histChange = 'تغییر'; page = 'صفحه'
    }
}

# --- Source parsing ------------------------------------------------------------------------------
function Read-Source([string] $path) {
    $lines = [System.IO.File]::ReadAllLines($path, [System.Text.Encoding]::UTF8)
    if ($lines.Count -lt 3 -or $lines[0].Trim() -ne '---') { throw "$path : missing front matter" }
    $meta = @{}
    $i = 1
    while ($lines[$i].Trim() -ne '---') {
        $line = $lines[$i]
        $colon = $line.IndexOf(':')
        if ($colon -gt 0) { $meta[$line.Substring(0, $colon).Trim()] = $line.Substring($colon + 1).Trim() }
        $i++
    }
    foreach ($key in 'title', 'subtitle', 'docid', 'version', 'date', 'author', 'status', 'lang', 'output', 'header', 'history') {
        if (-not $meta.ContainsKey($key)) { throw "$path : front matter key '$key' is missing" }
    }
    return @{ Meta = $meta; Body = $lines[($i + 1)..($lines.Count - 1)] }
}

# --- Word helpers -------------------------------------------------------------------------------
$script:persianLetters = '[\u0600-\u06FF]'
$script:lre = [string][char]0x202A   # left-to-right embedding
$script:pdf = [string][char]0x202C   # pop directional formatting
$script:rlm = [string][char]0x200F   # right-to-left mark

# In a right-to-left paragraph, Latin text that starts or ends with punctuation ("--port", "a/b/")
# is reordered by the bidi algorithm. A left-to-right embedding keeps it in reading order; short
# spans also get non-breaking spaces so an option and its value stay on one line.
function Protect-Ltr([string] $text) {
    if (-not $script:rtl -or $text -match $script:persianLetters) { return $text }
    if ($text.Length -le 32) { $text = $text.Replace(' ', [string][char]0x00A0) }
    return $script:lre + $text + $script:pdf
}

# Word does not pair brackets the way the Unicode bidi algorithm does: in "(... Qt Creator) را"
# the closing bracket after a Latin word can jump to the wrong side. Right-to-left marks on the
# outer side of brackets that touch Latin text keep both brackets in the Persian reading order.
function Protect-Brackets([string] $text) {
    if (-not $script:rtl -or $text -notmatch $script:persianLetters) { return $text }
    $text = [regex]::Replace($text, '(?<=[A-Za-z0-9.\]>%])\)', ')' + $script:rlm)
    return [regex]::Replace($text, '\((?=[A-Za-z0-9<\[-])', $script:rlm + '(')
}

function Set-StyleFonts($style, [string] $latin, [double] $size, [string] $bidiFont, [double] $bidiSize) {
    $style.Font.Name = $latin
    $style.Font.Size = $size
    $style.Font.NameBi = $bidiFont
    $style.Font.SizeBi = $bidiSize
}

function Initialize-Styles($doc) {
    $rtl = $script:rtl
    $bidiFont = 'B Nazanin'
    $order = if ($rtl) { $wdReadingOrderRtl } else { $wdReadingOrderLtr }
    $start = if ($rtl) { $wdAlignRight } else { $wdAlignLeft }

    $normal = $doc.Styles.Item($wdStyleNormal)
    Set-StyleFonts $normal 'Calibri' 11 $bidiFont 13
    $normal.ParagraphFormat.SpaceBefore = 3
    $normal.ParagraphFormat.SpaceAfter = 6
    $normal.ParagraphFormat.LineSpacingRule = $wdLineSpaceMultiple
    $normal.ParagraphFormat.LineSpacing = 13.8   # 1.15 lines
    $normal.ParagraphFormat.ReadingOrder = $order
    $normal.ParagraphFormat.Alignment = if ($rtl) { $wdAlignJustify } else { $wdAlignLeft }

    $headingSizes = @{ $wdStyleHeading1 = 17; $wdStyleHeading2 = 13.5; $wdStyleHeading3 = 12 }
    foreach ($id in $wdStyleHeading1, $wdStyleHeading2, $wdStyleHeading3) {
        $h = $doc.Styles.Item($id)
        Set-StyleFonts $h 'Calibri' $headingSizes[$id] $bidiFont ($headingSizes[$id] + 2)
        $h.Font.Bold = $true
        $h.Font.BoldBi = $true
        $h.Font.Color = $colorAccent
        $h.ParagraphFormat.ReadingOrder = $order
        $h.ParagraphFormat.Alignment = $start
        $h.ParagraphFormat.SpaceBefore = if ($id -eq $wdStyleHeading1) { 18 } else { 12 }
        $h.ParagraphFormat.SpaceAfter = 6
        $h.ParagraphFormat.KeepWithNext = $true
    }
    $h1 = $doc.Styles.Item($wdStyleHeading1)
    $h1.ParagraphFormat.Borders.Item(-3).LineStyle = 1     # bottom rule under chapter headings
    $h1.ParagraphFormat.Borders.Item(-3).Color = $colorAccent

    $title = $doc.Styles.Item($wdStyleTitle)
    Set-StyleFonts $title 'Calibri Light' 30 $bidiFont 30
    $title.Font.Color = $colorAccent
    $title.Font.BoldBi = $true
    $title.ParagraphFormat.ReadingOrder = $order
    $title.ParagraphFormat.Alignment = $start
    $title.ParagraphFormat.SpaceBefore = 110
    $title.ParagraphFormat.SpaceAfter = 6
    $title.ParagraphFormat.Borders.Enable = $false

    $subtitle = $doc.Styles.Item($wdStyleSubtitle)
    Set-StyleFonts $subtitle 'Calibri Light' 15 $bidiFont 16
    $subtitle.Font.Color = $colorMuted
    $subtitle.Font.Italic = $false
    $subtitle.ParagraphFormat.ReadingOrder = $order
    $subtitle.ParagraphFormat.Alignment = $start
    $subtitle.ParagraphFormat.SpaceAfter = 30

    foreach ($id in $wdStyleListBullet, $wdStyleListNumber, $wdStyleToc1, $wdStyleToc2, $wdStyleHeader, $wdStyleFooter) {
        $s = $doc.Styles.Item($id)
        $s.ParagraphFormat.ReadingOrder = $order
    }
    foreach ($id in $wdStyleListBullet, $wdStyleListNumber) {
        $doc.Styles.Item($id).ParagraphFormat.SpaceAfter = 3
        $doc.Styles.Item($id).ParagraphFormat.Alignment = $start
    }
    $doc.Styles.Item($wdStyleToc1).Font.Bold = $true
    $doc.Styles.Item($wdStyleToc1).Font.BoldBi = $true

    $caption = $doc.Styles.Item($wdStyleCaption)
    Set-StyleFonts $caption 'Calibri' 9.5 $bidiFont 11
    $caption.Font.Color = $colorMuted
    $caption.Font.Bold = $false
    $caption.Font.Italic = $true
    $caption.ParagraphFormat.Alignment = $wdAlignCenter
    $caption.ParagraphFormat.ReadingOrder = $order
    $caption.ParagraphFormat.SpaceAfter = 10

    $code = $doc.Styles.Add('Code Block', $wdStyleTypeParagraph)
    $code.BaseStyle = ''
    Set-StyleFonts $code 'Consolas' 9.5 'Consolas' 9.5
    $code.NoProofing = $true
    $code.ParagraphFormat.ReadingOrder = $wdReadingOrderLtr
    $code.ParagraphFormat.Alignment = $wdAlignLeft
    $code.ParagraphFormat.SpaceBefore = 0
    $code.ParagraphFormat.SpaceAfter = 0
    $code.ParagraphFormat.LineSpacingRule = 0      # single
    $code.ParagraphFormat.LeftIndent = 6
    $code.ParagraphFormat.RightIndent = 6
    $code.ParagraphFormat.Shading.BackgroundPatternColor = $colorCodeFill
    # Only the outside borders: Borders.Enable would also draw a rule between the lines of a block.
    foreach ($side in -1, -2, -3, -4) {
        $code.ParagraphFormat.Borders.Item($side).LineStyle = 1
        $code.ParagraphFormat.Borders.Item($side).Color = $colorCodeBorder
    }
    $code.ParagraphFormat.Borders.DistanceFromLeft = 4
    $code.ParagraphFormat.Borders.DistanceFromTop = 2
    $code.ParagraphFormat.Borders.DistanceFromBottom = 2

    $note = $doc.Styles.Add('Note Box', $wdStyleTypeParagraph)
    $note.BaseStyle = $normal.NameLocal
    $note.ParagraphFormat.LeftIndent = 6
    $note.ParagraphFormat.RightIndent = 6
    $note.ParagraphFormat.SpaceBefore = 6
    $note.ParagraphFormat.SpaceAfter = 8
    $note.ParagraphFormat.Shading.BackgroundPatternColor = $colorNoteFill
    $note.ParagraphFormat.Borders.Enable = $true
    $note.ParagraphFormat.Borders.OutsideColor = $colorNoteBorder
    $note.ParagraphFormat.Borders.DistanceFromLeft = 6
    $note.ParagraphFormat.Borders.DistanceFromRight = 6
    $note.ParagraphFormat.Borders.DistanceFromTop = 4
    $note.ParagraphFormat.Borders.DistanceFromBottom = 4

    $table = $doc.Styles.Add('Table Text', $wdStyleTypeParagraph)
    $table.BaseStyle = $normal.NameLocal
    Set-StyleFonts $table 'Calibri' 10 $bidiFont 11.5
    $table.ParagraphFormat.SpaceBefore = 2
    $table.ParagraphFormat.SpaceAfter = 2
    $table.ParagraphFormat.LineSpacingRule = 0
    $table.ParagraphFormat.Alignment = $start

    $contents = $doc.Styles.Add('Contents Title', $wdStyleTypeParagraph)
    $contents.BaseStyle = $normal.NameLocal
    Set-StyleFonts $contents 'Calibri' 17 $bidiFont 19
    $contents.Font.Bold = $true
    $contents.Font.BoldBi = $true
    $contents.Font.Color = $colorAccent
    $contents.ParagraphFormat.Alignment = $start
    $contents.ParagraphFormat.SpaceAfter = 12
}

# Types text with **bold** and `code` spans at the selection.
function Write-Inline($sel, [string] $text) {
    $parts = [regex]::Split($text, '(\*\*[^*]+\*\*|`[^`]+`)')
    foreach ($part in $parts) {
        if ($part.Length -eq 0) { continue }
        if ($part.StartsWith('**') -and $part.EndsWith('**') -and $part.Length -gt 4) {
            $sel.Font.Bold = $true
            $sel.Font.BoldBi = $true
            $sel.TypeText($part.Substring(2, $part.Length - 4))
            $sel.Font.Reset()
        }
        elseif ($part.StartsWith('`') -and $part.EndsWith('`') -and $part.Length -gt 2) {
            $sel.Font.Name = 'Consolas'
            $sel.Font.Size = 10
            $sel.Font.Color = $colorInlineCode
            $sel.TypeText((Protect-Ltr $part.Substring(1, $part.Length - 2)))
            $sel.Font.Reset()
        }
        else {
            $sel.TypeText((Protect-Brackets $part))
        }
    }
}

function Write-Paragraph($doc, $sel, $style, [string] $text) {
    $sel.Style = $style
    if ($script:pageBreakBeforeNext) { $sel.ParagraphFormat.PageBreakBefore = $true }
    Write-Inline $sel $text
    $sel.TypeParagraph()
    if ($script:pageBreakBeforeNext) {
        $sel.ParagraphFormat.PageBreakBefore = $false   # TypeParagraph copies it to the new paragraph
        $script:pageBreakBeforeNext = $false
    }
}

# A numbered list that starts with "1." restarts the numbering; one that starts with a higher
# number continues the previous list (steps interrupted by a code block or a note).
function Write-List($doc, $sel, [string] $kind, [string[]] $items, [int] $startNumber = 1) {
    $styleId = if ($kind -eq 'number') { $wdStyleListNumber } else { $wdStyleListBullet }
    $first = $true
    foreach ($item in $items) {
        # Later items inherit style and list from the paragraph mark typed after the first one.
        if ($first) {
            $sel.Style = $doc.Styles.Item($styleId)
            if ($kind -eq 'number') {
                # Every numbered list gets its own list template with an explicit start number.
                # Restarting or continuing a shared template made Word join unrelated lists.
                $template = $doc.ListTemplates.Add($false)
                $level = $template.ListLevels.Item(1)
                $level.NumberFormat = '%1.'
                $level.NumberStyle = 0          # 1, 2, 3 (shown as Persian digits in Persian text)
                $level.StartAt = $startNumber
                $level.NumberPosition = 9
                $level.TextPosition = 27
                $level.TabPosition = 27
                $sel.Range.ListFormat.ApplyListTemplate($template, $false)
            }
        }
        $first = $false
        Write-Inline $sel $item
        $sel.TypeParagraph()
    }
}

function Write-Code($doc, $sel, [string[]] $codeLines) {
    foreach ($line in $codeLines) {
        $sel.Style = $doc.Styles.Item('Code Block')
        # In a right-to-left document typed runs are marked RTL, which reorders "|", "(" and ","
        # even inside a left-to-right paragraph; force left-to-right runs for code.
        if ($script:rtl) { $sel.LtrRun() }
        if ($line.Length -gt 0) { $sel.TypeText($line) }
        $sel.TypeParagraph()
    }
}

function Split-Row([string] $line) {
    $trimmed = $line.Trim()
    if ($trimmed.StartsWith('|')) { $trimmed = $trimmed.Substring(1) }
    if ($trimmed.EndsWith('|')) { $trimmed = $trimmed.Substring(0, $trimmed.Length - 1) }
    return , ($trimmed.Split('|') | ForEach-Object { $_.Trim() })
}

function Write-Table($doc, $sel, [object[]] $rows, [bool] $headerRow = $true) {
    $rowCount = $rows.Count
    $colCount = ($rows | ForEach-Object { $_.Count } | Measure-Object -Maximum).Maximum
    $sel.Style = $doc.Styles.Item('Table Text')
    $table = $doc.Tables.Add($sel.Range, $rowCount, $colCount)
    if ($script:rtl) { $table.TableDirection = $wdTableDirectionRtl }
    $table.Borders.Enable = $true
    $table.Borders.InsideColor = $colorCodeBorder
    $table.Borders.OutsideColor = $colorCodeBorder
    $table.TopPadding = 1
    $table.BottomPadding = 1
    $table.LeftPadding = 5
    $table.RightPadding = 5
    for ($r = 0; $r -lt $rowCount; $r++) {
        for ($c = 0; $c -lt $colCount; $c++) {
            $text = if ($c -lt $rows[$r].Count) { $rows[$r][$c] } else { '' }
            $cell = $table.Cell($r + 1, $c + 1)
            $isCode = $text -match '^`[^`]+`$'
            $plain = ($text -replace '\*\*', '') -replace '`', ''
            $cell.Range.Text = (Protect-Brackets (Protect-Ltr $plain))
            if ($isCode) {
                $cell.Range.Font.Name = 'Consolas'
                $cell.Range.Font.Size = 9.5
            }
            if ($text -match '^\*\*.*\*\*$') {
                $cell.Range.Font.Bold = $true
                $cell.Range.Font.BoldBi = $true
            }
        }
    }
    if ($headerRow) {
        $header = $table.Rows.Item(1)
        $header.HeadingFormat = -1
        $header.Shading.BackgroundPatternColor = $colorHeaderFill
        $header.Range.Font.Bold = $true
        $header.Range.Font.BoldBi = $true
        $header.Range.Font.Color = $colorAccent
    }
    # Column widths follow the length of the longest cell (capped, so one long sentence does not
    # squeeze the other columns); together they fill the text width.
    $available = $doc.PageSetup.PageWidth - $doc.PageSetup.LeftMargin - $doc.PageSetup.RightMargin
    $weights = for ($c = 0; $c -lt $colCount; $c++) {
        $longest = ($rows | ForEach-Object { if ($c -lt $_.Count) { ($_[$c] -replace '[`*]', '').Length } else { 0 } } |
            Measure-Object -Maximum).Maximum
        [Math]::Min([Math]::Max($longest, 7), 45)
    }
    $total = ($weights | Measure-Object -Sum).Sum
    $table.AllowAutoFit = $false
    for ($c = 0; $c -lt $colCount; $c++) {
        $table.Columns.Item($c + 1).Width = $available * $weights[$c] / $total
    }
    $sel.EndKey($wdStory) | Out-Null
}

function Write-Picture($doc, $sel, [string] $caption, [string] $relativePath) {
    $path = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $relativePath.Replace('/', '\')))
    if (-not (Test-Path $path)) { throw "Picture not found: $path" }
    $sel.Style = $doc.Styles.Item($wdStyleNormal)
    $sel.ParagraphFormat.Alignment = $wdAlignCenter
    $sel.ParagraphFormat.KeepWithNext = $true
    $shape = $sel.InlineShapes.AddPicture($path, $false, $true)
    $shape.LockAspectRatio = -1
    $maxWidth = $doc.PageSetup.PageWidth - $doc.PageSetup.LeftMargin - $doc.PageSetup.RightMargin
    if ($shape.Width -gt $maxWidth) { $shape.Width = $maxWidth }
    $shape.Borders.Enable = $true
    $shape.Borders.OutsideColor = $colorCodeBorder
    $sel.TypeParagraph()
    Write-Paragraph $doc $sel $doc.Styles.Item($wdStyleCaption) $caption
}

function Set-DocumentProperty($doc, [string] $name, [string] $value) {
    try {
        $props = $doc.BuiltInDocumentProperties
        $prop = [System.__ComObject].InvokeMember('Item', 'GetProperty', $null, $props, @($name))
        [System.__ComObject].InvokeMember('Value', 'SetProperty', $null, $prop, @($value)) | Out-Null
    }
    catch { Write-Warning "Could not set document property $name" }
}

function Write-TitlePage($doc, $sel, $meta) {
    $l = $script:label
    Write-Paragraph $doc $sel $doc.Styles.Item($wdStyleTitle) $meta.title
    Write-Paragraph $doc $sel $doc.Styles.Item($wdStyleSubtitle) $meta.subtitle

    $rows = @(
        , @($l.project, 'VitalSigns Monitor')
        , @($l.document, $meta.title)
        , @($l.docId, $meta.docid)
        , @($l.version, $meta.version)
        , @($l.date, $meta.date)
        , @($l.author, $meta.author)
        , @($l.status, $meta.status)
    )
    Write-Table $doc $sel $rows $false
    $metaTable = $doc.Tables.Item($doc.Tables.Count)
    $metaTable.Columns.Item(1).Shading.BackgroundPatternColor = $colorHeaderFill
    $metaTable.Columns.Item(1).Select()
    $doc.Application.Selection.Font.Bold = $true
    $doc.Application.Selection.Font.BoldBi = $true
    $sel = $doc.Application.Selection
    $sel.EndKey($wdStory) | Out-Null

    Write-Paragraph $doc $sel $doc.Styles.Item($wdStyleNormal) ''
    $sel.Style = $doc.Styles.Item($wdStyleNormal)
    $sel.Font.Bold = $true
    $sel.Font.BoldBi = $true
    $sel.Font.Color = $colorAccent
    $sel.TypeText($l.history)
    $sel.Font.Reset()
    $sel.TypeParagraph()
    $history = @(, @($l.histVersion, $l.histDate, $l.histAuthor, $l.histChange))
    foreach ($entry in $meta.history.Split(';')) {
        $history += , (Split-Row $entry)
    }
    Write-Table $doc $sel $history $true

    if ($meta.ContainsKey('notice')) {
        Write-Paragraph $doc $sel $doc.Styles.Item($wdStyleNormal) ''
        Write-Paragraph $doc $sel $doc.Styles.Item('Note Box') $meta.notice
    }
    $sel.InsertBreak($wdPageBreak)

    Write-Paragraph $doc $sel $doc.Styles.Item('Contents Title') $l.contents
    $doc.TablesOfContents.Add($sel.Range, $true, 1, 2) | Out-Null
    $sel.EndKey($wdStory) | Out-Null
    # "Page break before" on the first heading instead of a break character: a page-break
    # paragraph after a full table of contents would leave an empty page.
    $script:pageBreakBeforeNext = $true
}

function Write-HeaderFooter($doc, $meta) {
    $doc.PageSetup.DifferentFirstPageHeaderFooter = -1
    $section = $doc.Sections.Item(1)

    $header = $section.Headers.Item($wdHeaderPrimary).Range
    $header.Text = $meta.header
    $header.Font.Size = 9
    $header.Font.SizeBi = 10
    $header.Font.Color = $colorMuted
    $header.ParagraphFormat.Borders.Item(-3).LineStyle = 1
    $header.ParagraphFormat.Borders.Item(-3).Color = $colorCodeBorder

    $footer = $section.Footers.Item($wdHeaderPrimary).Range
    $footer.Text = ''
    $footer.ParagraphFormat.Alignment = $wdAlignCenter
    $footer.Font.Size = 9
    $footer.Font.Color = $colorMuted
    $footer.InsertAfter($script:label.page + ' ')
    $footer.Collapse(0)
    $footer.Fields.Add($footer, $wdFieldPage) | Out-Null
}

function Build-Document($word, [string] $sourcePath) {
    Write-Verbose "building $sourcePath"
    $source = Read-Source $sourcePath
    $meta = $source.Meta
    $script:rtl = $meta.lang -eq 'fa'
    $script:label = $labels[$meta.lang]
    # Word treats a path with forward slashes as a web address and can hang on save.
    $outPath = [System.IO.Path]::GetFullPath((Join-Path $docsDir $meta.output.Replace('/', '\')))
    New-Item -ItemType Directory -Force (Split-Path $outPath -Parent) | Out-Null

    $doc = $word.Documents.Add()
    try {
        $doc.PageSetup.PaperSize = 7          # A4
        $doc.PageSetup.TopMargin = 64
        $doc.PageSetup.BottomMargin = 60
        $doc.PageSetup.LeftMargin = 62
        $doc.PageSetup.RightMargin = 62
        if ($script:rtl) { $doc.Content.ParagraphFormat.ReadingOrder = $wdReadingOrderRtl }
        Initialize-Styles $doc
        $sel = $word.Selection

        Write-TitlePage $doc $sel $meta
        $sel = $word.Selection

        $body = $source.Body
        $i = 0
        $paragraph = New-Object System.Collections.Generic.List[string]
        $flush = {
            if ($paragraph.Count -gt 0) {
                Write-Paragraph $doc $sel $doc.Styles.Item($wdStyleNormal) ($paragraph -join ' ')
                $paragraph.Clear()
            }
        }
        while ($i -lt $body.Count) {
            $line = $body[$i]
            $trim = $line.Trim()
            if ($trim -eq '') { & $flush; $i++; continue }

            if ($trim -match '^(#{1,3})\s+(.*)$') {
                & $flush
                $level = $Matches[1].Length
                $id = @($wdStyleHeading1, $wdStyleHeading2, $wdStyleHeading3)[$level - 1]
                # A right-to-left mark at the end keeps a trailing Latin word ("... CMake") in place,
                # also in the table of contents where the page number follows the heading text.
                $heading = $Matches[2]
                if ($script:rtl) {
                    # Marks around the section number too: in the table of contents Word otherwise
                    # moves "۴-" behind the first Persian word when the heading contains Latin text.
                    $heading = [regex]::Replace($heading, '^([0-9۰-۹\-]+)\s+', $script:rlm + '$1' + $script:rlm + ' ')
                    $heading = $heading + $script:rlm
                }
                Write-Paragraph $doc $sel $doc.Styles.Item($id) $heading
                $i++; continue
            }
            if ($trim -eq '{{pagebreak}}') { & $flush; $sel.InsertBreak($wdPageBreak); $i++; continue }
            if ($trim.StartsWith('```')) {
                & $flush
                $code = New-Object System.Collections.Generic.List[string]
                $i++
                while ($i -lt $body.Count -and -not $body[$i].Trim().StartsWith('```')) { $code.Add($body[$i]); $i++ }
                $i++
                Write-Code $doc $sel $code.ToArray()
                continue
            }
            if ($trim -match '^!\[(.*)\]\((.*)\)$') {
                & $flush
                Write-Picture $doc $sel $Matches[1] $Matches[2]
                $i++; continue
            }
            if ($trim.StartsWith('|')) {
                & $flush
                $rows = @()
                while ($i -lt $body.Count -and $body[$i].Trim().StartsWith('|')) {
                    if ($body[$i] -notmatch '^\s*\|[\s:\-|]+\|\s*$') { $rows += , (Split-Row $body[$i]) }
                    $i++
                }
                Write-Table $doc $sel $rows $true
                continue
            }
            if ($trim -match '^(-|\d+\.)\s+') {
                & $flush
                $kind = if ($trim.StartsWith('-')) { 'bullet' } else { 'number' }
                $pattern = if ($kind -eq 'bullet') { '^-\s+(.*)$' } else { '^\d+\.\s+(.*)$' }
                $start = if ($kind -eq 'number') { [int]($trim -replace '^(\d+)\..*$', '$1') } else { 1 }
                $items = New-Object System.Collections.Generic.List[string]
                while ($i -lt $body.Count -and $body[$i].Trim() -match $pattern) { $items.Add($Matches[1]); $i++ }
                Write-List $doc $sel $kind $items.ToArray() $start
                continue
            }
            if ($trim.StartsWith('>')) {
                & $flush
                $note = New-Object System.Collections.Generic.List[string]
                while ($i -lt $body.Count -and $body[$i].Trim().StartsWith('>')) { $note.Add($body[$i].Trim().TrimStart('>').Trim()); $i++ }
                Write-Paragraph $doc $sel $doc.Styles.Item('Note Box') ($note -join ' ')
                continue
            }
            $paragraph.Add($trim)
            $i++
        }
        & $flush
        # The last TypeParagraph leaves an empty paragraph; drop its list or code style so the
        # document does not end with an empty bullet.
        $doc.Paragraphs.Last.Style = $doc.Styles.Item($wdStyleNormal)

        Write-Verbose "header, footer, contents"
        Write-HeaderFooter $doc $meta
        $doc.TablesOfContents.Item(1).Update()
        if ($script:rtl) {
            # Word builds the contents entries from runs without the right-to-left flag, so a
            # heading such as "۴- نصب Qt" shows its number in the wrong place. Mark them RTL.
            $doc.TablesOfContents.Item(1).Range.Select()
            $word.Selection.RtlRun()
            $word.Selection.EndKey($wdStory) | Out-Null
        }
        Set-DocumentProperty $doc 'Title' $meta.title
        Set-DocumentProperty $doc 'Subject' 'VitalSigns Monitor'
        Set-DocumentProperty $doc 'Author' $meta.author

        if (Test-Path $outPath) { Remove-Item $outPath }
        Write-Verbose "saving $outPath"
        $doc.SaveAs2($outPath, $wdFormatDocx)
        Write-Host "Built $($meta.output)"
        if ($PdfDir) {
            # SaveAs2 to PDF; ExportAsFixedFormat stalled hidden Word instances on the development PC.
            New-Item -ItemType Directory -Force $PdfDir | Out-Null
            $doc.SaveAs2((Join-Path $PdfDir ([System.IO.Path]::GetFileNameWithoutExtension($outPath) + '.pdf')), $wdFormatPdf)
        }
    }
    finally {
        $doc.Close(0)
    }
}

function New-Word {
    $before = @(Get-Process WINWORD -ErrorAction SilentlyContinue | ForEach-Object Id)
    $word = New-Object -ComObject Word.Application
    $script:wordPid = Get-Process WINWORD -ErrorAction SilentlyContinue |
        Where-Object { $before -notcontains $_.Id } | Select-Object -First 1 -ExpandProperty Id
    $word.Visible = $false
    $word.DisplayAlerts = 0
    return $word
}

# Quit() returns before the process has gone. A new Word.Application created in that window can
# attach to the dying instance and hang, so wait until the process has really exited.
function Close-Word($word) {
    $word.Quit()
    [System.Runtime.InteropServices.Marshal]::ReleaseComObject($word) | Out-Null
    [GC]::Collect()
    [GC]::WaitForPendingFinalizers()
    if ($script:wordPid) { Wait-Process -Id $script:wordPid -Timeout 60 -ErrorAction SilentlyContinue }
}

# --- Main ---------------------------------------------------------------------------------------
$sources = if ($Only) { $Only | ForEach-Object { if ([System.IO.Path]::IsPathRooted($_)) { $_ } else { Join-Path $sourceDir $_ } } }
           else { Get-ChildItem (Join-Path $sourceDir 'en'), (Join-Path $sourceDir 'fa') -Filter *.md | ForEach-Object FullName }

# One Word instance per document keeps each build independent of the previous one.
foreach ($src in $sources) {
    $word = New-Word
    try { Build-Document $word $src } finally { Close-Word $word }
}

