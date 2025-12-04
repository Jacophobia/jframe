#Requires -Version 5.1
<#
.SYNOPSIS
    JFrame Development Environment Setup Script for Windows

.DESCRIPTION
    This script sets up a complete development environment for JFrame on Windows
    with ZERO prerequisites. Everything is installed via package managers (winget)
    for easy global updates.

    Steps:
    1. Installs winget (if not present)
    2. Verifies/installs Visual Studio 2022 with C++ workload
    3. Installs CMake, Ninja, Git via winget
    4. Installs vcpkg package manager
    5. Prompts for FMOD installation (optional, for audio)
    6. Configures and builds JFrame with C++23

    The script is idempotent - running it multiple times is safe and will
    only install/update components that are missing or outdated.

    NOTE: Windows builds use MSVC (Visual Studio 2022) with native C++23 module support.

.PARAMETER NoBuild
    Setup environment only, skip building JFrame

.PARAMETER CI
    Non-interactive CI mode (no prompts, implies -NoBuild)

.PARAMETER Help
    Show this help message

.EXAMPLE
    .\setup.ps1
    # Full interactive setup and build

.EXAMPLE
    .\setup.ps1 -NoBuild
    # Setup only, skip build

.EXAMPLE
    .\setup.ps1 -CI
    # CI mode (non-interactive, no build)

.NOTES
    FMOD Core API must be downloaded manually from https://fmod.com/download
    See docs/Installation.md for FMOD setup instructions.
#>

[CmdletBinding()]
param(
    [switch]$NoBuild,
    [switch]$CI,
    [switch]$Help
)

# =============================================================================
# Configuration
# =============================================================================

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$VcpkgDir = if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { "C:\vcpkg" }
$BuildPreset = "windows-debug"

# CI mode implies NoBuild
if ($CI) {
    $NoBuild = $true
}

# =============================================================================
# Helper Functions
# =============================================================================

function Write-Header {
    param([string]$Message)
    Write-Host ""
    Write-Host ("=" * 70) -ForegroundColor Blue
    Write-Host "  $Message" -ForegroundColor Blue
    Write-Host ("=" * 70) -ForegroundColor Blue
    Write-Host ""
}

function Write-Success {
    param([string]$Message)
    Write-Host "[OK] $Message" -ForegroundColor Green
}

function Write-Warning {
    param([string]$Message)
    Write-Host "[WARN] $Message" -ForegroundColor Yellow
}

function Write-Error {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor Red
}

function Write-Info {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor Cyan
}

function Test-CommandExists {
    param([string]$Command)
    $null -ne (Get-Command $Command -ErrorAction SilentlyContinue)
}

function Test-AdminPrivileges {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Compare-Version {
    param(
        [string]$Version1,
        [string]$Version2
    )
    $v1 = [Version]::Parse($Version1)
    $v2 = [Version]::Parse($Version2)
    return $v1.CompareTo($v2)
}

function Refresh-Path {
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")
}

function Wait-ForUser {
    param([string]$Message)

    if ($CI) {
        Write-Info "CI mode: skipping prompt - $Message"
        return
    }

    Write-Host ""
    Write-Host $Message -ForegroundColor Yellow
    Write-Host ""
    Read-Host "Press Enter when ready to continue (or Ctrl+C to abort)"
    Write-Host ""
}

function Ask-YesNo {
    param(
        [string]$Question,
        [string]$Default = "y"
    )

    if ($CI) {
        return $true  # Always yes in CI mode
    }

    if ($Default -eq "y") {
        $prompt = "$Question [Y/n]"
    } else {
        $prompt = "$Question [y/N]"
    }

    $response = Read-Host $prompt

    if ([string]::IsNullOrEmpty($response)) {
        return ($Default -eq "y")
    }

    return ($response -match "^[yY]")
}

function Show-HelpMessage {
    Get-Help $MyInvocation.ScriptName -Detailed
}

# =============================================================================
# winget Installation
# =============================================================================

function Install-Winget {
    Write-Header "Checking winget"

    if (Test-CommandExists "winget") {
        Write-Success "winget is already installed"
        return
    }

    Write-Info "winget is not installed."
    Write-Info "winget is the Windows package manager and will install VS2022, CMake, etc."

    if (-not (Ask-YesNo "Install winget now?")) {
        Write-Error "winget is required. Please install App Installer from the Microsoft Store."
        exit 1
    }

    Write-Info "Installing winget..."

    try {
        $progressPreference = 'silentlyContinue'

        # Get latest release
        $releases = Invoke-RestMethod -Uri "https://api.github.com/repos/microsoft/winget-cli/releases/latest"
        $msixBundle = $releases.assets | Where-Object { $_.name -match "\.msixbundle$" } | Select-Object -First 1

        if ($null -eq $msixBundle) {
            throw "Could not find winget release"
        }

        $downloadPath = "$env:TEMP\winget.msixbundle"
        Invoke-WebRequest -Uri $msixBundle.browser_download_url -OutFile $downloadPath

        # Install using Add-AppxPackage
        Add-AppxPackage -Path $downloadPath

        Remove-Item $downloadPath -Force -ErrorAction SilentlyContinue

        Write-Success "winget installed successfully"
    }
    catch {
        Write-Warning "Failed to install winget automatically"
        Write-Info "Please install App Installer from the Microsoft Store:"
        Write-Info "  https://www.microsoft.com/p/app-installer/9nblggh4nns1"

        Wait-ForUser "Install App Installer from Microsoft Store, then continue"

        if (-not (Test-CommandExists "winget")) {
            Write-Error "winget still not found. Please install it and try again."
            exit 1
        }
    }
}

# =============================================================================
# Visual Studio Installation
# =============================================================================

function Test-VisualStudio {
    # Check for Visual Studio 2022 with C++ workload
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

    if (-not (Test-Path $vswhere)) {
        return $false
    }

    # Check for VS2022 with C++ Desktop workload or Build Tools
    $vsPath = & $vswhere -version "[17.0,18.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null

    if ([string]::IsNullOrEmpty($vsPath)) {
        return $false
    }

    # Verify cl.exe exists
    $clPath = Join-Path $vsPath "VC\Tools\MSVC\*\bin\Hostx64\x64\cl.exe"
    $clExe = Get-ChildItem -Path $clPath -ErrorAction SilentlyContinue | Sort-Object -Descending | Select-Object -First 1

    if ($null -eq $clExe) {
        return $false
    }

    return $true
}

function Get-MSVCVersion {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

    if (-not (Test-Path $vswhere)) {
        return $null
    }

    $vsPath = & $vswhere -version "[17.0,18.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null

    if ([string]::IsNullOrEmpty($vsPath)) {
        return $null
    }

    # Get MSVC version from directory name
    $msvcPath = Join-Path $vsPath "VC\Tools\MSVC"
    $msvcVersions = Get-ChildItem -Path $msvcPath -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending

    if ($msvcVersions.Count -gt 0) {
        return $msvcVersions[0].Name
    }

    return $null
}

function Install-VisualStudio {
    Write-Header "Checking Visual Studio 2022"

    if (Test-VisualStudio) {
        $msvcVersion = Get-MSVCVersion
        Write-Success "Visual Studio 2022 with C++ tools is installed (MSVC $msvcVersion)"

        # Check MSVC version for C++23 module support (need 19.38+)
        if ($null -ne $msvcVersion) {
            $majorMinor = $msvcVersion.Split('.')[0..1] -join '.'
            if ([double]$majorMinor -ge 14.38) {
                Write-Success "MSVC version supports C++23 modules (import std;)"
            } else {
                Write-Warning "MSVC version $msvcVersion may not fully support C++23 modules"
                Write-Info "Consider updating Visual Studio for best compatibility"
            }
        }
        return
    }

    Write-Info "Visual Studio 2022 with C++ tools is required but not found."
    Write-Info "This provides MSVC compiler with C++23 module support."
    Write-Host ""

    if ($CI) {
        Write-Error "CI mode: Visual Studio 2022 must be pre-installed"
        exit 1
    }

    Write-Host "Installation Options:" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "  Option 1: Visual Studio Community (Free, includes IDE)" -ForegroundColor Cyan
    Write-Host "    winget install Microsoft.VisualStudio.2022.Community --override `"--add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended --passive`""
    Write-Host ""
    Write-Host "  Option 2: Build Tools Only (Smaller, no IDE)" -ForegroundColor Cyan
    Write-Host "    winget install Microsoft.VisualStudio.2022.BuildTools --override `"--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --passive`""
    Write-Host ""
    Write-Host "  Option 3: Visual Studio Installer (GUI)" -ForegroundColor Cyan
    Write-Host "    Download from: https://visualstudio.microsoft.com/downloads/"
    Write-Host "    Select 'Desktop development with C++' workload"
    Write-Host ""

    $choice = Read-Host "Enter option (1/2/3) or press Enter to install Build Tools"

    switch ($choice) {
        "1" {
            Write-Info "Installing Visual Studio 2022 Community via winget..."
            Write-Info "This may take 10-20 minutes..."
            $result = winget install Microsoft.VisualStudio.2022.Community --override "--add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended --passive" --accept-package-agreements --accept-source-agreements 2>&1
            if ($LASTEXITCODE -ne 0) {
                Write-Warning "Installation may have failed. Please verify manually."
            }
        }
        "3" {
            Start-Process "https://visualstudio.microsoft.com/downloads/"
            Wait-ForUser "Please install Visual Studio 2022 with 'Desktop development with C++' workload"
        }
        default {
            Write-Info "Installing Visual Studio 2022 Build Tools via winget..."
            Write-Info "This may take 5-15 minutes..."
            $result = winget install Microsoft.VisualStudio.2022.BuildTools --override "--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --passive" --accept-package-agreements --accept-source-agreements 2>&1
            if ($LASTEXITCODE -ne 0) {
                Write-Warning "Installation may have failed. Please verify manually."
            }
        }
    }

    # Verify installation
    Refresh-Path

    if (Test-VisualStudio) {
        $msvcVersion = Get-MSVCVersion
        Write-Success "Visual Studio 2022 installed (MSVC $msvcVersion)"
    } else {
        Write-Warning "Visual Studio 2022 installation not detected."
        Wait-ForUser "Please complete Visual Studio installation and restart this script if needed"

        if (-not (Test-VisualStudio)) {
            Write-Error "Visual Studio 2022 with C++ tools is required."
            exit 1
        }
    }
}

# =============================================================================
# Build Tools Installation
# =============================================================================

function Install-CMake {
    Write-Header "Checking CMake"

    $cmakeVersion = $null
    if (Test-CommandExists "cmake") {
        $versionOutput = cmake --version 2>$null | Select-Object -First 1
        if ($versionOutput -match "(\d+\.\d+\.\d+)") {
            $cmakeVersion = $Matches[1]
        }
    }

    if ($null -ne $cmakeVersion -and (Compare-Version $cmakeVersion "3.28.0") -ge 0) {
        Write-Success "CMake $cmakeVersion already installed"
        return
    }

    if ($null -ne $cmakeVersion) {
        Write-Info "CMake $cmakeVersion is too old, upgrading..."
    } else {
        Write-Info "Installing CMake via winget..."
    }

    try {
        winget install --id Kitware.CMake --silent --accept-package-agreements --accept-source-agreements 2>&1 | Out-Null
        Write-Success "CMake installed"
        Refresh-Path
    }
    catch {
        Write-Warning "Failed to install CMake via winget"
        Write-Info "Please install CMake 3.28+ from: https://cmake.org/download/"
        Wait-ForUser "Install CMake and add it to PATH"
    }

    # Verify
    Refresh-Path
    if (Test-CommandExists "cmake") {
        $versionOutput = cmake --version 2>$null | Select-Object -First 1
        if ($versionOutput -match "(\d+\.\d+\.\d+)") {
            Write-Success "CMake $($Matches[1]) verified"
        }
    }
}

function Install-Ninja {
    Write-Header "Checking Ninja"

    if (Test-CommandExists "ninja") {
        Write-Success "Ninja already installed"
        return
    }

    Write-Info "Installing Ninja via winget..."
    try {
        winget install --id Ninja-build.Ninja --silent --accept-package-agreements --accept-source-agreements 2>&1 | Out-Null
        Write-Success "Ninja installed"
        Refresh-Path
    }
    catch {
        Write-Warning "Failed to install Ninja via winget"
        Write-Info "Ninja is optional but recommended for faster builds"
    }
}

function Install-Git {
    Write-Header "Checking Git"

    if (Test-CommandExists "git") {
        Write-Success "Git already installed"
        return
    }

    Write-Info "Installing Git via winget..."
    try {
        winget install --id Git.Git --silent --accept-package-agreements --accept-source-agreements 2>&1 | Out-Null
        Write-Success "Git installed"
        Refresh-Path
    }
    catch {
        Write-Warning "Failed to install Git via winget"
        Write-Info "Please install Git from: https://git-scm.com/download/win"
        Wait-ForUser "Install Git and add it to PATH"
    }

    # Verify
    Refresh-Path
    if (-not (Test-CommandExists "git")) {
        Write-Error "Git is required but not found"
        exit 1
    }
}

# =============================================================================
# vcpkg Setup
# =============================================================================

function Install-Vcpkg {
    Write-Header "Setting up vcpkg"

    if (Test-Path "$VcpkgDir\vcpkg.exe") {
        Write-Success "vcpkg already installed at $VcpkgDir"

        # Update vcpkg (skip in CI for speed)
        if (-not $CI) {
            Write-Info "Updating vcpkg..."
            Push-Location $VcpkgDir
            try {
                git pull --quiet 2>$null
                & .\bootstrap-vcpkg.bat -disableMetrics 2>$null | Out-Null
            }
            catch {
                Write-Warning "Failed to update vcpkg (non-fatal)"
            }
            Pop-Location
        }
        return
    }

    Write-Info "vcpkg is a C++ package manager that will install project dependencies."
    Write-Info "It will be installed to: $VcpkgDir"
    Write-Info "Update it anytime with: cd $VcpkgDir; git pull; .\bootstrap-vcpkg.bat"

    if (-not (Ask-YesNo "Install vcpkg now?")) {
        Write-Error "vcpkg is required for building JFrame."
        exit 1
    }

    Write-Info "Cloning vcpkg..."
    git clone https://github.com/microsoft/vcpkg.git $VcpkgDir

    Write-Info "Bootstrapping vcpkg..."
    Push-Location $VcpkgDir
    & .\bootstrap-vcpkg.bat -disableMetrics
    Pop-Location

    Write-Success "vcpkg installed successfully"

    # Set environment variable
    Write-Info "Setting VCPKG_ROOT environment variable..."
    [System.Environment]::SetEnvironmentVariable("VCPKG_ROOT", $VcpkgDir, "User")
    $env:VCPKG_ROOT = $VcpkgDir
}

# =============================================================================
# FMOD Setup (Optional)
# =============================================================================

function Install-Fmod {
    Write-Header "FMOD Audio Library (Optional)"

    if (Test-Path "$ScriptDir\external\fmod\core") {
        Write-Success "FMOD already installed at external\fmod\core"
        return
    }

    Write-Info "FMOD is required for audio features but is NOT installed."
    Write-Info "FMOD is proprietary and must be downloaded manually from:"
    Write-Host ""
    Write-Host "    https://fmod.com/download" -ForegroundColor White
    Write-Host ""
    Write-Info "After downloading:"
    Write-Host "    1. Extract the FMOD Core API archive"
    Write-Host "    2. Copy the contents to: $ScriptDir\external\fmod\core\"
    Write-Host ""

    if ($CI) {
        Write-Warning "CI mode: Skipping FMOD (audio features will be disabled)"
        return
    }

    if (Ask-YesNo "Would you like to install FMOD now?" "n") {
        Write-Info "Opening FMOD download page..."
        Start-Process "https://fmod.com/download"

        Wait-ForUser "Please download and extract FMOD Core API to: $ScriptDir\external\fmod\core\"

        if (Test-Path "$ScriptDir\external\fmod\core") {
            Write-Success "FMOD installation detected"
        } else {
            Write-Warning "FMOD not detected. Audio features will be disabled."
        }
    } else {
        Write-Warning "Skipping FMOD. Audio features will be disabled."
        Write-Info "You can install FMOD later and re-run this script."
    }
}

# =============================================================================
# Build JFrame
# =============================================================================

function Build-JFrame {
    Write-Header "Building JFrame"

    Push-Location $ScriptDir

    # Set environment variables
    $env:VCPKG_ROOT = $VcpkgDir

    # Clean incompatible cache
    $cacheFile = "$ScriptDir\build\$BuildPreset\CMakeCache.txt"
    if (Test-Path $cacheFile) {
        $cacheContent = Get-Content $cacheFile -Raw -ErrorAction SilentlyContinue
        if ($cacheContent -notmatch "CMAKE_TOOLCHAIN_FILE.*vcpkg") {
            Write-Info "Removing incompatible build cache..."
            Remove-Item -Recurse -Force "$ScriptDir\build\$BuildPreset" -ErrorAction SilentlyContinue
        }
    }

    # Configure
    Write-Info "Configuring with preset: $BuildPreset"
    Write-Info "Using MSVC (Visual Studio 2022)"
    Write-Info "This will download and build vcpkg dependencies (may take several minutes on first run)..."

    $configResult = cmake --preset $BuildPreset 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host $configResult
        Write-Error "CMake configuration failed"
        Write-Info "Try removing the build directory and running again:"
        Write-Host "  Remove-Item -Recurse -Force build\$BuildPreset"
        Pop-Location
        exit 1
    }
    Write-Success "Configuration complete"

    # Build (--config Debug required for multi-config generators)
    Write-Info "Building JFrame..."
    $buildResult = cmake --build --preset $BuildPreset --config Debug --parallel 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host $buildResult
        Write-Error "Build failed"
        Pop-Location
        exit 1
    }
    Write-Success "Build complete"

    # Run tests
    Write-Info "Running tests..."
    $jobs = [Math]::Max(1, $env:NUMBER_OF_PROCESSORS - 1)
    Write-Info "Running tests with $jobs parallel jobs"
    $testResult = ctest --preset $BuildPreset --build-config Debug -j $jobs 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Success "All tests passed"
    } else {
        Write-Host $testResult
        Write-Warning "Some tests failed - check output above"
    }

    Pop-Location
}

# =============================================================================
# Post-Setup Instructions
# =============================================================================

function Show-PostSetup {
    Write-Header "Setup Complete!"

    Write-Host "Your JFrame development environment is ready."
    Write-Host ""
    Write-Host "Compiler: MSVC (Visual Studio 2022) with C++23 module support"
    Write-Host "  Update with: Visual Studio Installer -> Update"
    Write-Host ""

    if (-not (Test-Path "$ScriptDir\external\fmod\core")) {
        Write-Host "NOTE: FMOD is not installed (audio features disabled)" -ForegroundColor Yellow
        Write-Host "  Install from: https://fmod.com/download"
        Write-Host "  Extract to: external\fmod\core\"
        Write-Host ""
    }

    Write-Host "Useful commands:"
    Write-Host ""
    Write-Host "  # Rebuild"
    Write-Host "  cmake --build --preset $BuildPreset --config Debug --parallel"
    Write-Host ""
    Write-Host "  # Run tests"
    Write-Host "  ctest --preset $BuildPreset --build-config Debug"
    Write-Host ""
    Write-Host "  # Clean rebuild"
    Write-Host "  Remove-Item -Recurse -Force build\$BuildPreset"
    Write-Host "  cmake --preset $BuildPreset"
    Write-Host "  cmake --build --preset $BuildPreset --config Debug"
    Write-Host ""
    Write-Host "  # Update dependencies"
    Write-Host "  winget upgrade --all                # Update winget packages"
    Write-Host "  cd $VcpkgDir; git pull              # Update vcpkg"
    Write-Host ""
}

# =============================================================================
# Main
# =============================================================================

function Main {
    if ($Help) {
        Show-HelpMessage
        exit 0
    }

    Write-Header "JFrame Development Environment Setup"

    Write-Host "Operating System: Windows $([System.Environment]::OSVersion.Version)"
    Write-Host "Architecture: $env:PROCESSOR_ARCHITECTURE"
    Write-Host "Script Directory: $ScriptDir"
    Write-Host "vcpkg Directory: $VcpkgDir"
    if ($CI) {
        Write-Host "Mode: CI (non-interactive)"
    } else {
        Write-Host "Mode: Interactive"
    }
    Write-Host ""

    # Check admin for some installations
    if (-not (Test-CommandExists "winget") -and -not (Test-AdminPrivileges)) {
        Write-Warning "winget may require administrator privileges to install"
        Write-Info "If installation fails, please run this script as Administrator"
        Write-Host ""
    }

    # Install components
    Install-Winget
    Install-Git
    Install-VisualStudio
    Install-CMake
    Install-Ninja
    Install-Vcpkg
    Install-Fmod

    # Build
    if (-not $NoBuild) {
        Build-JFrame
    } else {
        Write-Info "Skipping build (-NoBuild or -CI specified)"
    }

    Show-PostSetup
}

# Run main
Main
