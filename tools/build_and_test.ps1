param(
    [string]$BuildDir = "",
    [string]$BuildType = "Debug",
    [switch]$Sanitize
)

$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $Root "build-local"
}

Write-Host "== AIM Editor build doctor =="
python (Join-Path $Root "tools\build_doctor.py")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "`n== AIM Editor repository checks =="
python (Join-Path $Root "tools\check_repository.py")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$CMakeArgs = @(
    "-S", $Root,
    "-B", $BuildDir,
    "-DAIM_EDITOR_BUILD_TESTS=ON"
)

$LocalJuce = $env:AIM_EDITOR_JUCE_PATH
if ([string]::IsNullOrWhiteSpace($LocalJuce)) {
    $RepoJuce = Join-Path $Root ".deps\JUCE"
    if (Test-Path (Join-Path $RepoJuce "CMakeLists.txt")) {
        $LocalJuce = $RepoJuce
    }
}
if ([string]::IsNullOrWhiteSpace($LocalJuce)) {
    $HomeJuce = Join-Path $HOME "GitHub\JUCE"
    if (Test-Path (Join-Path $HomeJuce "CMakeLists.txt")) {
        $LocalJuce = $HomeJuce
    }
}
if (-not [string]::IsNullOrWhiteSpace($LocalJuce)) {
    $CMakeArgs += "-DAIM_EDITOR_JUCE_PATH=$LocalJuce"
}
if ($Sanitize) {
    $CMakeArgs += "-DAIM_EDITOR_ENABLE_SANITIZERS=ON"
}

# Multi-config Visual Studio is the normal Windows path. Ninja remains usable
# when explicitly selected through CMAKE_GENERATOR in the environment.
if ([string]::IsNullOrWhiteSpace($env:CMAKE_GENERATOR)) {
    $CMakeArgs += @("-A", "x64")
}

Write-Host "`n== Configure =="
& cmake @CMakeArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "`n== Build =="
cmake --build $BuildDir --config $BuildType --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "`n== Tests =="
ctest --test-dir $BuildDir -C $BuildType --output-on-failure
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "`nPASS: AIM Editor local Windows build/test pipeline"
