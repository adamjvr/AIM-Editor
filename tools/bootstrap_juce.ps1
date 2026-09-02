param(
    [string]$Destination = "",
    [string]$Version = "9.0.1"
)

$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if ([string]::IsNullOrWhiteSpace($Destination)) {
    $Destination = Join-Path $Root ".deps\JUCE"
}

if ($Version -ne "9.0.1") {
    throw "This bootstrap script has pinned hashes only for JUCE 9.0.1. Use AIM_EDITOR_JUCE_PATH for another verified checkout."
}

$Asset = "juce-9.0.1-windows.zip"
$Expected = "de0256584416764d82ef5794d26a34b06a68bc65326f24e0d91301abd74704c1"
$Url = "https://github.com/juce-framework/JUCE/releases/download/$Version/$Asset"
$TempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("aim-juce-" + [Guid]::NewGuid().ToString("N"))
$Archive = Join-Path $TempRoot $Asset
$Extract = Join-Path $TempRoot "extract"

New-Item -ItemType Directory -Force -Path $Extract | Out-Null
try {
    Write-Host "== AIM Editor JUCE bootstrap =="
    Write-Host "Version : $Version"
    Write-Host "Asset   : $Asset"
    Write-Host "Target  : $Destination"

    Invoke-WebRequest -Uri $Url -OutFile $Archive
    $Actual = (Get-FileHash -Algorithm SHA256 -Path $Archive).Hash.ToLowerInvariant()
    if ($Actual -ne $Expected) {
        throw "JUCE archive SHA-256 mismatch. Expected $Expected, got $Actual"
    }
    Write-Host "PASS: JUCE archive SHA-256 verified"

    Expand-Archive -Path $Archive -DestinationPath $Extract -Force
    $CoreModule = Get-ChildItem -Path $Extract -Directory -Recurse |
        Where-Object { $_.FullName -match '[\\/]modules[\\/]juce_core$' } |
        Select-Object -First 1
    if ($null -eq $CoreModule) {
        throw "Downloaded archive does not contain modules/juce_core"
    }

    $JuceRoot = Split-Path (Split-Path $CoreModule.FullName -Parent) -Parent
    if (-not (Test-Path (Join-Path $JuceRoot "CMakeLists.txt"))) {
        throw "Detected JUCE root has no CMakeLists.txt: $JuceRoot"
    }

    $Parent = Split-Path $Destination -Parent
    New-Item -ItemType Directory -Force -Path $Parent | Out-Null
    if (Test-Path $Destination) { Remove-Item -Recurse -Force $Destination }
    Move-Item -Path $JuceRoot -Destination $Destination

    Write-Host "PASS: JUCE $Version installed at $Destination"
    Write-Host ""
    Write-Host "Build AIM Editor with:"
    Write-Host "  `$env:AIM_EDITOR_JUCE_PATH='$Destination'; ./tools/build_and_test.ps1"
}
finally {
    if (Test-Path $TempRoot) { Remove-Item -Recurse -Force $TempRoot }
}
