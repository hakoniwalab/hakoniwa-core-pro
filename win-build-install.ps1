param(
    [Alias("OutputDir")]
    [string]$InstallPrefix = "",

    [string]$BuildDir = "build-win",

    [string]$Configuration = "Release",

    [string]$Generator = "Visual Studio 17 2022",

    [string]$Platform = "x64",

    [Nullable[int]]$AssetNum = $null,

    [Nullable[int]]$ServiceMax = $null,

    [Nullable[int]]$RecvEventMax = $null,

    [Nullable[int]]$ServiceClientMax = $null,

    [Nullable[int]]$ChannelMax = $null,

    [string]$EnableHakoTimeMeasureFlag = $env:ENABLE_HAKO_TIME_MEASURE_FLAG,

    [string]$BuildCFlags = $env:BUILD_C_FLAGS,

    [switch]$Clean
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-FullPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }
    return [System.IO.Path]::GetFullPath((Join-Path (Get-Location) $Path))
}

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Command
    )

    Write-Host ("[RUN] " + ($Command -join " "))
    if ($Command.Length -gt 1) {
        & $Command[0] $Command[1..($Command.Length - 1)]
    }
    else {
        & $Command[0]
    }
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE"
    }
}

function Get-EffectiveAssetNum {
    param([Nullable[int]]$Value)

    $defaultValue = 16
    if ($null -ne $Value) {
        if ($Value -gt $defaultValue) {
            return [int]$Value
        }
        return $defaultValue
    }

    if ($env:ASSET_NUM) {
        $envValue = [int]$env:ASSET_NUM
        if ($envValue -gt $defaultValue) {
            return $envValue
        }
    }
    return $defaultValue
}

function Get-EffectivePositiveInt {
    param(
        [Nullable[int]]$Value,
        [string]$EnvName,
        [int]$DefaultValue
    )

    if ($null -ne $Value) {
        if ($Value -gt 0) {
            return [int]$Value
        }
        return $DefaultValue
    }

    $envValueText = [Environment]::GetEnvironmentVariable($EnvName)
    if (-not [string]::IsNullOrWhiteSpace($envValueText)) {
        $envValue = [int]$envValueText
        if ($envValue -gt 0) {
            return $envValue
        }
    }
    return $DefaultValue
}

function Split-Flags {
    param([string]$Flags)

    if ([string]::IsNullOrWhiteSpace($Flags)) {
        return @()
    }
    return ($Flags -split '\s+') | Where-Object { $_ -ne "" }
}

function Copy-DirectoryContents {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if (Test-Path -LiteralPath $Path) {
        Get-ChildItem -LiteralPath $Path -Recurse -File | Sort-Object FullName | ForEach-Object {
            $_.FullName.Substring($installPrefixPath.Length).TrimStart('\', '/')
        }
    }
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-FullPath -Path $scriptDir
$buildDirPath = Resolve-FullPath -Path $BuildDir
$cmakeOptionFile = Join-Path $repoRoot "cmake-options/win-cmake-options.cmake"

if ([string]::IsNullOrWhiteSpace($InstallPrefix)) {
    if (-not [string]::IsNullOrWhiteSpace($env:INSTALL_PREFIX)) {
        $InstallPrefix = $env:INSTALL_PREFIX
    }
    else {
        $InstallPrefix = "install-win"
    }
}
$installPrefixPath = Resolve-FullPath -Path $InstallPrefix

$effectiveAssetNum = Get-EffectiveAssetNum -Value $AssetNum
$effectiveServiceMax = Get-EffectivePositiveInt -Value $ServiceMax -EnvName "SERVICE_MAX" -DefaultValue 4096
$effectiveRecvEventMax = Get-EffectivePositiveInt -Value $RecvEventMax -EnvName "RECV_EVENT_MAX" -DefaultValue 16384
$effectiveServiceClientMax = Get-EffectivePositiveInt -Value $ServiceClientMax -EnvName "SERVICE_CLIENT_MAX" -DefaultValue 1024
$effectiveChannelMax = Get-EffectivePositiveInt -Value $ChannelMax -EnvName "CHANNEL_MAX" -DefaultValue 8192

Write-Host "ASSET_NUM is $effectiveAssetNum"
Write-Host "SERVICE_MAX is $effectiveServiceMax"
Write-Host "RECV_EVENT_MAX is $effectiveRecvEventMax"
Write-Host "SERVICE_CLIENT_MAX is $effectiveServiceClientMax"
Write-Host "CHANNEL_MAX is $effectiveChannelMax"
Write-Host "Repository root : $repoRoot"
Write-Host "Build directory : $buildDirPath"
Write-Host "Install prefix  : $installPrefixPath"
Write-Host "Configuration   : $Configuration"
Write-Host "Platform        : $Platform"

if ($Clean -and (Test-Path -LiteralPath $buildDirPath)) {
    Write-Host "Cleaning build directory: $buildDirPath"
    Remove-Item -LiteralPath $buildDirPath -Recurse -Force
}

New-Item -ItemType Directory -Path $buildDirPath -Force | Out-Null
New-Item -ItemType Directory -Path $installPrefixPath -Force | Out-Null

$configureArgs = @(
    "cmake",
    "-S", $repoRoot,
    "-B", $buildDirPath,
    "-G", $Generator,
    "-A", $Platform,
    "-DHAKO_CLIENT_OPTION_FILEPATH=$cmakeOptionFile",
    "-DHAKO_DATA_MAX_ASSET_NUM=$effectiveAssetNum",
    "-DHAKO_SERVICE_MAX=$effectiveServiceMax",
    "-DHAKO_RECV_EVENT_MAX=$effectiveRecvEventMax",
    "-DHAKO_SERVICE_CLIENT_MAX=$effectiveServiceClientMax",
    "-DHAKO_PDU_CHANNEL_MAX=$effectiveChannelMax",
    "-DCMAKE_INSTALL_PREFIX=$installPrefixPath"
)
$configureArgs += Split-Flags -Flags $EnableHakoTimeMeasureFlag
$configureArgs += Split-Flags -Flags $BuildCFlags
Invoke-Checked -Command $configureArgs

Invoke-Checked -Command @(
    "cmake",
    "--build", $buildDirPath,
    "--config", $Configuration,
    "--target", "ALL_BUILD"
)

Invoke-Checked -Command @(
    "cmake",
    "--install", $buildDirPath,
    "--config", $Configuration,
    "--prefix", $installPrefixPath
)

$installedFiles = @()
$installedFiles += Copy-DirectoryContents -Path (Join-Path $installPrefixPath "include")
$installedFiles += Copy-DirectoryContents -Path (Join-Path $installPrefixPath "lib")
$installedFiles += Copy-DirectoryContents -Path (Join-Path $installPrefixPath "bin")
$installedFiles += Copy-DirectoryContents -Path (Join-Path $installPrefixPath "share")
$installedFiles += Copy-DirectoryContents -Path (Join-Path $installPrefixPath "etc")
$installedFiles = $installedFiles | Sort-Object -Unique

Write-Host ""
Write-Host "Installed to: $installPrefixPath"
if ($installedFiles.Count -gt 0) {
    Write-Host "Installed files:"
    foreach ($installedFile in $installedFiles) {
        Write-Host "  - $installedFile"
    }
}
