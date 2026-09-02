param(
    [string]$BuildDir = "",
    [string]$BuildType = "Debug",
    [switch]$Sanitize,
    [switch]$NoBootstrap
)

$ErrorActionPreference = "Stop"
$env:PYTHONDONTWRITEBYTECODE = "1"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $Root "build-local"
}

function Find-LocalJuce {
    if (-not [string]::IsNullOrWhiteSpace($env:AIM_EDITOR_JUCE_PATH) -and
        (Test-Path (Join-Path $env:AIM_EDITOR_JUCE_PATH "CMakeLists.txt"))) {
        return $env:AIM_EDITOR_JUCE_PATH
    }

    $RepoJuce = Join-Path $Root ".deps\JUCE"
    if (Test-Path (Join-Path $RepoJuce "CMakeLists.txt")) {
        return $RepoJuce
    }

    $HomeJuce = Join-Path $HOME "GitHub\JUCE"
    if (Test-Path (Join-Path $HomeJuce "CMakeLists.txt")) {
        return $HomeJuce
    }

    return $null
}

$LocalJuce = Find-LocalJuce
if ([string]::IsNullOrWhiteSpace($LocalJuce) -and -not $NoBootstrap) {
    Write-Host "== Bootstrap pinned JUCE =="
    & (Join-Path $Root "tools\bootstrap_juce.ps1")
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    $LocalJuce = Find-LocalJuce
}

if ([string]::IsNullOrWhiteSpace($LocalJuce)) {
    throw "No local JUCE 9.0.1 tree is available. Run tools\bootstrap_juce.ps1 or set AIM_EDITOR_JUCE_PATH."
}
$env:AIM_EDITOR_JUCE_PATH = $LocalJuce

$SystemPython = if ([string]::IsNullOrWhiteSpace($env:AIM_EDITOR_PYTHON)) { "python" } else { $env:AIM_EDITOR_PYTHON }
& $SystemPython (Join-Path $Root "tools\bootstrap_python_tools.py")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$ToolsPython = Join-Path $Root ".deps\python-tools\Scripts\python.exe"
if (-not (Test-Path $ToolsPython)) { throw "Project-local Python validation interpreter was not created." }

Write-Host "== Clean generated Python artifacts =="
& $ToolsPython (Join-Path $Root "tools\clean_python_artifacts.py")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "== AIM Editor build doctor =="
& $ToolsPython (Join-Path $Root "tools\build_doctor.py") --require-local-juce --strict-platform
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "`n== AIM Editor repository checks =="
& $ToolsPython (Join-Path $Root "tools\check_repository.py")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$CMakeArgs = @(
    "-S", $Root,
    "-B", $BuildDir,
    "-DAIM_EDITOR_BUILD_TESTS=ON",
    "-DAIM_EDITOR_JUCE_PATH=$LocalJuce"
)

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
