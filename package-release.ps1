[CmdletBinding()]
param(
    [ValidatePattern('^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?$')]
    [string]$Version = '1.3.0',

    [string]$AsiLoaderPath,

    [ValidatePattern('^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?$')]
    [string]$AsiLoaderVersion,

    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repositoryDirectory = Split-Path -Parent $PSCommandPath
$releaseName = "DOTO-HighFPS-Fix-v$Version"
$buildDirectory = Join-Path $repositoryDirectory 'build'
$distDirectory = Join-Path $repositoryDirectory 'dist'
$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) (
    'doto-high-fps-release-' + [Guid]::NewGuid().ToString('N'))
$pluginPayloadDirectory = Join-Path $temporaryRoot $releaseName
$loaderReleaseName = "$releaseName-with-Ultimate-ASI-Loader"
$loaderPayloadDirectory = Join-Path $temporaryRoot $loaderReleaseName

function Assert-X64PeFile {
    param([Parameter(Mandatory)][string]$Path)

    $stream = [IO.File]::OpenRead($Path)
    $reader = [IO.BinaryReader]::new($stream)
    try {
        if ($reader.ReadUInt16() -ne 0x5A4D) {
            throw "Not a PE file: $Path"
        }
        $stream.Position = 0x3C
        $peOffset = $reader.ReadInt32()
        if ($peOffset -lt 0 -or $peOffset -gt ($stream.Length - 6)) {
            throw "Invalid PE header offset: $Path"
        }
        $stream.Position = $peOffset
        if ($reader.ReadUInt32() -ne 0x00004550) {
            throw "Invalid PE signature: $Path"
        }
        if ($reader.ReadUInt16() -ne 0x8664) {
            throw "The optional ASI loader must be an x64 PE file: $Path"
        }
    }
    finally {
        $reader.Dispose()
        $stream.Dispose()
    }
}

function Copy-PayloadFiles {
    param(
        [Parameter(Mandatory)][Collections.IDictionary]$Files,
        [Parameter(Mandatory)][string]$Destination
    )

    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    foreach ($entry in $Files.GetEnumerator()) {
        if (-not (Test-Path -LiteralPath $entry.Value -PathType Leaf)) {
            throw "Required release file is missing: $($entry.Value)"
        }
        Copy-Item -LiteralPath $entry.Value -Destination (
            Join-Path $Destination $entry.Key)
    }
}

function Write-PayloadChecksums {
    param([Parameter(Mandatory)][string]$PayloadDirectory)

    $checksumLines = Get-ChildItem -LiteralPath $PayloadDirectory -File |
        Where-Object Name -ne 'SHA256SUMS.txt' |
        Sort-Object Name |
        ForEach-Object {
            $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash
            "$hash  $($_.Name)"
        }
    Set-Content -LiteralPath (Join-Path $PayloadDirectory 'SHA256SUMS.txt') (
        $checksumLines -join "`r`n") -Encoding Ascii
}

function Write-ReleaseArchive {
    param(
        [Parameter(Mandatory)][string]$PayloadDirectory,
        [Parameter(Mandatory)][string]$ArchivePath
    )

    $archiveHashPath = "$ArchivePath.sha256"
    if (Test-Path -LiteralPath $ArchivePath) {
        Remove-Item -LiteralPath $ArchivePath -Force
    }
    if (Test-Path -LiteralPath $archiveHashPath) {
        Remove-Item -LiteralPath $archiveHashPath -Force
    }

    Compress-Archive -LiteralPath $PayloadDirectory -DestinationPath $ArchivePath `
        -CompressionLevel Optimal

    $archiveHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $ArchivePath).Hash
    Set-Content -LiteralPath $archiveHashPath (
        "$archiveHash  $([IO.Path]::GetFileName($ArchivePath))") -Encoding Ascii
    Write-Host "Created $ArchivePath"
    Write-Host "SHA-256: $archiveHash"
}

if ($AsiLoaderPath) {
    $AsiLoaderPath = [IO.Path]::GetFullPath($AsiLoaderPath)
    if (-not (Test-Path -LiteralPath $AsiLoaderPath -PathType Leaf)) {
        throw "The ASI loader does not exist: $AsiLoaderPath"
    }
    if ([IO.Path]::GetFileName($AsiLoaderPath) -ine 'dinput8.dll') {
        throw 'The optional loader must be the x64 dinput8.dll build of Ultimate ASI Loader.'
    }
    if ([string]::IsNullOrWhiteSpace($AsiLoaderVersion)) {
        throw '-AsiLoaderVersion is required when -AsiLoaderPath is supplied.'
    }
    Assert-X64PeFile -Path $AsiLoaderPath
    $embeddedLoaderVersion = (Get-Item -LiteralPath $AsiLoaderPath).VersionInfo.FileVersion
    if (-not [string]::IsNullOrWhiteSpace($embeddedLoaderVersion) -and
        $embeddedLoaderVersion -ne $AsiLoaderVersion) {
        throw "Loader version mismatch: requested $AsiLoaderVersion, file reports $embeddedLoaderVersion."
    }
}
elseif ($AsiLoaderVersion) {
    throw '-AsiLoaderVersion requires -AsiLoaderPath.'
}

if ($SkipBuild) {
    $existingPlugin = Join-Path $buildDirectory 'DOTOHighFPSFix.asi'
    $existingLoadTest = Join-Path $buildDirectory 'asi-load-test.exe'
    if (-not (Test-Path -LiteralPath $existingPlugin -PathType Leaf) -or
        -not (Test-Path -LiteralPath $existingLoadTest -PathType Leaf)) {
        throw '-SkipBuild requires existing DOTOHighFPSFix.asi and asi-load-test.exe outputs.'
    }
    & $existingLoadTest $existingPlugin
    if ($LASTEXITCODE -ne 0) {
        throw "Existing ASI load test failed with exit code $LASTEXITCODE"
    }
}
else {
    & (Join-Path $repositoryDirectory 'build-msvc.cmd')
    if ($LASTEXITCODE -ne 0) {
        throw "build-msvc.cmd failed with exit code $LASTEXITCODE"
    }
}

New-Item -ItemType Directory -Path $distDirectory -Force | Out-Null

try {
    $pluginPayloadFiles = [ordered]@{
        'DOTOHighFPSFix.asi' = Join-Path $buildDirectory 'DOTOHighFPSFix.asi'
        'doto-high-fps-fix.ini' = Join-Path $repositoryDirectory 'doto-high-fps-fix.ini'
        'README.txt' = Join-Path $repositoryDirectory 'README.txt'
        'CHANGELOG.md' = Join-Path $repositoryDirectory 'CHANGELOG.md'
        'LICENSE' = Join-Path $repositoryDirectory 'LICENSE'
    }

    Copy-PayloadFiles -Files $pluginPayloadFiles -Destination $pluginPayloadDirectory
    Write-PayloadChecksums -PayloadDirectory $pluginPayloadDirectory
    Write-ReleaseArchive -PayloadDirectory $pluginPayloadDirectory -ArchivePath (
        Join-Path $distDirectory "$releaseName.zip")

    if ($AsiLoaderPath) {
        $loaderIntegrationDirectory = Join-Path $temporaryRoot 'ual-integration-test'
        New-Item -ItemType Directory -Path $loaderIntegrationDirectory -Force | Out-Null
        $integrationPlugin = Join-Path $loaderIntegrationDirectory 'DOTOHighFPSFix.asi'
        $integrationLoader = Join-Path $loaderIntegrationDirectory 'dinput8.dll'
        $integrationTest = Join-Path $loaderIntegrationDirectory 'asi-load-test.exe'
        Copy-Item -LiteralPath (Join-Path $buildDirectory 'DOTOHighFPSFix.asi') `
            -Destination $integrationPlugin
        Copy-Item -LiteralPath $AsiLoaderPath -Destination $integrationLoader
        Copy-Item -LiteralPath (Join-Path $buildDirectory 'asi-load-test.exe') `
            -Destination $integrationTest

        & $integrationTest $integrationPlugin $integrationLoader
        if ($LASTEXITCODE -ne 0) {
            throw "Ultimate ASI Loader integration test failed with exit code $LASTEXITCODE"
        }

        Copy-PayloadFiles -Files $pluginPayloadFiles -Destination $loaderPayloadDirectory
        Copy-Item -LiteralPath $AsiLoaderPath -Destination (
            Join-Path $loaderPayloadDirectory 'dinput8.dll')
        Copy-Item -LiteralPath (
            Join-Path $repositoryDirectory 'third-party\Ultimate-ASI-Loader-LICENSE.txt') `
            -Destination (Join-Path $loaderPayloadDirectory 'Ultimate-ASI-Loader-LICENSE.txt')

        $loaderHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $AsiLoaderPath).Hash
        $loaderNotice = @(
            'Ultimate ASI Loader (third-party component)'
            "Version: $AsiLoaderVersion"
            'Upstream: https://github.com/ThirteenAG/Ultimate-ASI-Loader'
            "SHA-256 (dinput8.dll): $loaderHash"
            'License: MIT; see Ultimate-ASI-Loader-LICENSE.txt'
            ''
            'The high-FPS fix is DOTOHighFPSFix.asi. The dinput8.dll file is the'
            'external loader and may be omitted when a compatible x64 ASI loader'
            'is already installed.'
        )
        Set-Content -LiteralPath (
            Join-Path $loaderPayloadDirectory 'Ultimate-ASI-Loader.txt') `
            -Value ($loaderNotice -join "`r`n") -Encoding Ascii

        Write-PayloadChecksums -PayloadDirectory $loaderPayloadDirectory
        Write-ReleaseArchive -PayloadDirectory $loaderPayloadDirectory -ArchivePath (
            Join-Path $distDirectory "$loaderReleaseName.zip")
    }
}
finally {
    $resolvedTemporaryRoot = [IO.Path]::GetFullPath($temporaryRoot)
    $resolvedSystemTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    $temporaryLeaf = Split-Path -Leaf $resolvedTemporaryRoot
    if ($resolvedTemporaryRoot.StartsWith($resolvedSystemTemp,
            [StringComparison]::OrdinalIgnoreCase) -and
        $temporaryLeaf.StartsWith('doto-high-fps-release-',
            [StringComparison]::OrdinalIgnoreCase) -and
        (Test-Path -LiteralPath $resolvedTemporaryRoot)) {
        Remove-Item -LiteralPath $resolvedTemporaryRoot -Recurse -Force
    }
}
