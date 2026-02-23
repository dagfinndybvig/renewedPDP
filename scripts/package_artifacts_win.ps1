# package_artifacts_win.ps1
# Packages the Windows build outputs into a ZIP file.
#
# Run from the repository root (or from any location — the script finds the repo root):
#   powershell -ExecutionPolicy Bypass -File scripts\package_artifacts_win.ps1
#
# Output: renewedPDP-windows-artifacts.zip at the repository root.
# To build first: cd src && nmake /f Makefile.win

param(
    [string]$OutputName = "renewedPDP-windows-artifacts.zip"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$outputPath = Join-Path $repoRoot $OutputName

$artifacts = @(
    "aa\aa.exe",
    "bp\bp.exe",
    "cl\cl.exe",
    "cs\cs.exe",
    "ia\ia.exe",
    "iac\iac.exe",
    "pa\pa.exe"
)

$missing = @()
foreach ($rel in $artifacts) {
    if (-not (Test-Path (Join-Path $repoRoot $rel))) {
        $missing += $rel
    }
}

if ($missing.Count -gt 0) {
    Write-Error ("Missing build artifacts:`n" +
        ($missing | ForEach-Object { "  - $_" } | Out-String) +
        "Run: cd src && nmake /f Makefile.win")
    exit 1
}

if (Test-Path $outputPath) {
    Remove-Item $outputPath -Force
}

# Run from repo root so archive entries use relative paths (aa/aa.exe, etc.)
Push-Location $repoRoot
try {
    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [System.IO.Compression.ZipFile]::Open($outputPath, [System.IO.Compression.ZipArchiveMode]::Create)
    foreach ($rel in $artifacts) {
        $entryName = $rel.Replace('\', '/')
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $zip, (Join-Path $repoRoot $rel), $entryName,
            [System.IO.Compression.CompressionLevel]::Optimal
        ) | Out-Null
    }
    $zip.Dispose()
} finally {
    Pop-Location
}

Write-Host "Created: $outputPath"
Write-Host "Contents:"
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::OpenRead($outputPath)
$zip.Entries | ForEach-Object { Write-Host "  $($_.FullName)" }
$zip.Dispose()
