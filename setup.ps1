#Requires -Version 5.1
<#
.SYNOPSIS
    JFrame Development Environment Setup Script for Windows

.DESCRIPTION
    This script sets up a complete development environment for JFrame on Windows:

    1. Installs winget (if not present)
    2. Installs Visual Studio 2022 Build Tools with C++ and modules support
    3. Installs CMake, Ninja, and Git
    4. Installs vcpkg package manager
    5. Configures and builds JFrame

    The script is idempotent - running it multiple times is safe and will
    only install/update components that are missing or outdated.

.PARAMETER NoBuild
    Setup environment only, skip building JFrame

.PARAMETER UseNinja
    Use Ninja generator instead of Visual Studio generator (faster builds)

.PARAMETER Help
    Show this help message

.EXAMPLE
    .\setup.ps1
    # Full setup and build

.EXAMPLE
    .\setup.ps1 -NoBuild
    # Setup only, skip build

.EXAMPLE
    .\setup.ps1 -UseNinja
    # Full setup and build using Ninja generator

.NOTES
    FMOD Core API must be downloaded manually from https://fmod.com/download
    See docs/Installation.md for FMOD setup instructions.
#>

[CmdletBinding()]
param(
    [switch]$NoBuild,
    [switch]$UseNinja,
    [switch]$Help
)

# =============================================================================
# Configuration
# =============================================================================

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$VcpkgDir = if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { "C:\vcpkg" }
$BuildPreset = if ($UseNinja) { "windows-ninja-debug" } else { "windows-debug" }

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

function Get-VSWherePath {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        return $vswhere
    }
    return $null
}

function Get-VSInstallPath {
    $vswhere = Get-VSWherePath
    if ($null -eq $vswhere) {
        return $null
    }
    $path = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
    return $path
}

function Get-MSVCVersion {
    $vsPath = Get-VSInstallPath
    if ($null -eq $vsPath) {
        return $null
    }

    $vcToolsVersion = Get-Content "$vsPath\VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt" -ErrorAction SilentlyContinue
    return $vcToolsVersion?.Trim()
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

    Write-Info "Installing winget..."

    # winget is included with App Installer from Microsoft Store
    # For automated installation, we can use the GitHub release
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
        Write-Host ""
        Write-Info "After installation, re-run this script."
        exit 1
    }
}

# =============================================================================
# Visual Studio Installation
# =============================================================================

function Install-VisualStudio {
    Write-Header "Checking Visual Studio 2022"

    $vsPath = Get-VSInstallPath
    $msvcVersion = Get-MSVCVersion

    if ($null -ne $vsPath -and $null -ne $msvcVersion) {
        Write-Info "Found Visual Studio at: $vsPath"
        Write-Info "MSVC version: $msvcVersion"

        # Check if version is adequate (19.38+)
        $majorMinor = $msvcVersion.Split('.')[0..1] -join '.'
        if ((Compare-Version $majorMinor "19.38") -ge 0) {
            Write-Success "Visual Studio 2022 with adequate MSVC version installed"

            # Check for C++ Modules component
            $vswhere = Get-VSWherePath
            $hasModules = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Modules.x86.x64 -property installationPath 2>$null

            if ($null -eq $hasModules) {
                Write-Warning "C++ Modules component not found"
                Write-Info "This is required for 'import std;' support"
                Write-Info ""
                Write-Info "To install:"
                Write-Info "  1. Open Visual Studio Installer"
                Write-Info "  2. Click 'Modify' on your VS 2022 installation"
                Write-Info "  3. Go to 'Individual Components'"
                Write-Info "  4. Search for 'C++ Modules'"
                Write-Info "  5. Check 'C++ Modules for v143 build tools'"
                Write-Info "  6. Click 'Modify'"
                Write-Host ""
            } else {
                Write-Success "C++ Modules component installed"
            }

            return
        } else {
            Write-Warning "MSVC version $msvcVersion is too old. Need 19.38+"
            Write-Info "Please update Visual Studio via Visual Studio Installer"
        }
    }

    # Install Visual Studio Build Tools
    Write-Info "Installing Visual Studio 2022 Build Tools..."
    Write-Info "This may take a while..."

    # Use winget to install VS Build Tools with required components
    $vsComponents = @(
        "--add", "Microsoft.VisualStudio.Workload.VCTools",
        "--add", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
        "--add", "Microsoft.VisualStudio.Component.VC.CMake.Project",
        "--add", "Microsoft.VisualStudio.Component.VC.Modules.x86.x64",
        "--add", "Microsoft.VisualStudio.Component.Windows11SDK.22621"
    )

    try {
        $result = winget install --id Microsoft.VisualStudio.2022.BuildTools --silent --accept-package-agreements --accept-source-agreements @vsComponents 2>&1

        if ($LASTEXITCODE -eq 0 -or $result -match "already installed") {
            Write-Success "Visual Studio 2022 Build Tools installed"
        } else {
            throw "winget returned exit code $LASTEXITCODE"
        }
    }
    catch {
        Write-Warning "Automated installation may have issues"
        Write-Info "If Visual Studio is not properly installed, please:"
        Write-Info "  1. Download Visual Studio 2022 from:"
        Write-Info "     https://visualstudio.microsoft.com/downloads/"
        Write-Info "  2. During installation, select:"
        Write-Info "     - Desktop development with C++"
        Write-Info "     - C++ Modules for v143 build tools (Individual Components)"
        Write-Host ""
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
        Write-Success "CMake $cmakeVersion is installed"
        return
    }

    if ($null -ne $cmakeVersion) {
        Write-Info "CMake $cmakeVersion is too old. Need 3.28+"
    }

    Write-Info "Installing CMake..."
    try {
        winget install --id Kitware.CMake --silent --accept-package-agreements --accept-source-agreements 2>&1 | Out-Null
        Write-Success "CMake installed"

        # Refresh PATH
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")
    }
    catch {
        Write-Warning "Failed to install CMake via winget"
        Write-Info "Please install CMake manually from: https://cmake.org/download/"
    }
}

function Install-Ninja {
    Write-Header "Checking Ninja"

    if (Test-CommandExists "ninja") {
        Write-Success "Ninja is already installed"
        return
    }

    Write-Info "Installing Ninja..."
    try {
        winget install --id Ninja-build.Ninja --silent --accept-package-agreements --accept-source-agreements 2>&1 | Out-Null
        Write-Success "Ninja installed"

        # Refresh PATH
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")
    }
    catch {
        Write-Warning "Failed to install Ninja via winget"
        Write-Info "Please install Ninja manually"
    }
}

function Install-Git {
    Write-Header "Checking Git"

    if (Test-CommandExists "git") {
        Write-Success "Git is already installed"
        return
    }

    Write-Info "Installing Git..."
    try {
        winget install --id Git.Git --silent --accept-package-agreements --accept-source-agreements 2>&1 | Out-Null
        Write-Success "Git installed"

        # Refresh PATH
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")
    }
    catch {
        Write-Warning "Failed to install Git via winget"
        Write-Info "Please install Git manually from: https://git-scm.com/download/win"
    }
}

# =============================================================================
# vcpkg Setup
# =============================================================================

function Install-Vcpkg {
    Write-Header "Setting up vcpkg"

    if (Test-Path "$VcpkgDir\vcpkg.exe") {
        Write-Success "vcpkg already installed at $VcpkgDir"

        # Update vcpkg
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
        return
    }

    Write-Info "Installing vcpkg to $VcpkgDir..."

    # Clone vcpkg
    git clone https://github.com/microsoft/vcpkg.git $VcpkgDir

    # Bootstrap vcpkg
    Push-Location $VcpkgDir
    & .\bootstrap-vcpkg.bat -disableMetrics
    Pop-Location

    Write-Success "vcpkg installed successfully"

    # Set environment variable
    Write-Info "Setting VCPKG_ROOT environment variable..."
    [System.Environment]::SetEnvironmentVariable("VCPKG_ROOT", $VcpkgDir, "User")
    $env:VCPKG_ROOT = $VcpkgDir

    Write-Info "Consider adding vcpkg to your PATH:"
    Write-Host ""
    Write-Host "  [System.Environment]::SetEnvironmentVariable('Path', `$env:Path + ';$VcpkgDir', 'User')"
    Write-Host ""
}

# =============================================================================
# Build JFrame
# =============================================================================

function Build-JFrame {
    Write-Header "Building JFrame"

    Push-Location $ScriptDir

    # Set VCPKG_ROOT
    $env:VCPKG_ROOT = $VcpkgDir

    # Check for FMOD
    if (-not (Test-Path "$ScriptDir\external\fmod\core")) {
        Write-Warning "FMOD not found in external\fmod\core"
        Write-Info "Audio features will not work without FMOD."
        Write-Info "Download FMOD Core API from: https://fmod.com/download"
        Write-Info "See docs\Installation.md for setup instructions."
        Write-Host ""
    }

    # Clean incompatible cache
    $cacheFile = "$ScriptDir\build\$BuildPreset\CMakeCache.txt"
    if (Test-Path $cacheFile) {
        $cacheContent = Get-Content $cacheFile -Raw -ErrorAction SilentlyContinue
        if ($cacheContent -notmatch "CMAKE_TOOLCHAIN_FILE.*vcpkg") {
            Write-Info "Removing incompatible build cache..."
            Remove-Item -Recurse -Force "$ScriptDir\build\$BuildPreset" -ErrorAction SilentlyContinue
        }
    }

    # Find VS Developer environment
    $vsPath = Get-VSInstallPath
    if ($null -eq $vsPath) {
        Write-Error "Visual Studio not found. Please install Visual Studio 2022."
        Pop-Location
        exit 1
    }

    # Import VS environment if not already in Developer Command Prompt
    $vcvarsall = "$vsPath\VC\Auxiliary\Build\vcvars64.bat"
    if ((Test-Path $vcvarsall) -and -not $env:VSCMD_VER) {
        Write-Info "Setting up Visual Studio environment..."

        # Run vcvars64.bat and capture environment
        $envOutput = cmd /c "`"$vcvarsall`" >nul 2>&1 && set"
        foreach ($line in $envOutput) {
            if ($line -match "^(.+?)=(.*)$") {
                [System.Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], "Process")
            }
        }
    }

    # Configure
    Write-Info "Configuring with preset: $BuildPreset"
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

    # Build
    Write-Info "Building JFrame..."
    $buildResult = cmake --build --preset $BuildPreset 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host $buildResult
        Write-Error "Build failed"
        Pop-Location
        exit 1
    }
    Write-Success "Build complete"

    # Run tests
    Write-Info "Running tests..."
    $testResult = ctest --preset $BuildPreset --output-on-failure 2>&1
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

    if (-not (Test-Path "$ScriptDir\external\fmod\core")) {
        Write-Host "IMPORTANT: FMOD is not installed" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "To enable audio features:"
        Write-Host "  1. Download FMOD Core API from: https://fmod.com/download"
        Write-Host "  2. Extract and copy to: external\fmod\core\"
        Write-Host "  3. Re-run this script or rebuild manually"
        Write-Host ""
    }

    Write-Host "Useful commands (run from Developer Command Prompt):"
    Write-Host ""
    Write-Host "  # Rebuild"
    Write-Host "  cmake --build --preset $BuildPreset"
    Write-Host ""
    Write-Host "  # Run tests"
    Write-Host "  ctest --preset $BuildPreset"
    Write-Host ""
    Write-Host "  # Clean rebuild"
    Write-Host "  Remove-Item -Recurse -Force build\$BuildPreset"
    Write-Host "  cmake --preset $BuildPreset"
    Write-Host "  cmake --build --preset $BuildPreset"
    Write-Host ""
    Write-Host "For more information, see:"
    Write-Host "  - docs\Installation.md"
    Write-Host "  - docs\Getting-Started.md"
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
    Write-Host ""

    # Check admin for winget installation
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

    # Build
    if (-not $NoBuild) {
        Build-JFrame
    } else {
        Write-Info "Skipping build (-NoBuild specified)"
    }

    Show-PostSetup
}

# Run main
Main
