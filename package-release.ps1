[CmdletBinding()]
param(
    [ValidatePattern('^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?$')]
    [string]$Version = '1.0.0'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repositoryDirectory = Split-Path -Parent $PSCommandPath
$releaseName = "DOTO-HighFPS-Fix-v$Version"
$buildDirectory = Join-Path $repositoryDirectory 'build'
$distDirectory = Join-Path $repositoryDirectory 'dist'
$archivePath = Join-Path $distDirectory "$releaseName.zip"
$archiveHashPath = "$archivePath.sha256"
$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) (
    'doto-high-fps-release-' + [Guid]::NewGuid().ToString('N'))
$payloadDirectory = Join-Path $temporaryRoot $releaseName

& (Join-Path $repositoryDirectory 'build-msvc.cmd')
if ($LASTEXITCODE -ne 0) {
    throw "build-msvc.cmd failed with exit code $LASTEXITCODE"
}

& (Join-Path $buildDirectory 'proxy-smoke-test.exe') (
    Join-Path $buildDirectory 'dinput8.dll')
if ($LASTEXITCODE -ne 0) {
    throw "The proxy smoke test failed with exit code $LASTEXITCODE"
}

New-Item -ItemType Directory -Path $payloadDirectory -Force | Out-Null
New-Item -ItemType Directory -Path $distDirectory -Force | Out-Null

try {
    $payloadFiles = [ordered]@{
        'dinput8.dll' = Join-Path $buildDirectory 'dinput8.dll'
        'doto-high-fps-fix.ini' = Join-Path $repositoryDirectory 'doto-high-fps-fix.ini'
        'README.txt' = Join-Path $repositoryDirectory 'README.txt'
    }

    foreach ($entry in $payloadFiles.GetEnumerator()) {
        if (-not (Test-Path -LiteralPath $entry.Value -PathType Leaf)) {
            throw "Required release file is missing: $($entry.Value)"
        }
        Copy-Item -LiteralPath $entry.Value -Destination (
            Join-Path $payloadDirectory $entry.Key)
    }

    $checksumLines = foreach ($fileName in $payloadFiles.Keys) {
        $payloadPath = Join-Path $payloadDirectory $fileName
        $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $payloadPath).Hash
        "$hash  $fileName"
    }
    Set-Content -LiteralPath (Join-Path $payloadDirectory 'SHA256SUMS.txt') (
        $checksumLines -join "`r`n") -Encoding Ascii

    if (Test-Path -LiteralPath $archivePath) {
        Remove-Item -LiteralPath $archivePath -Force
    }
    if (Test-Path -LiteralPath $archiveHashPath) {
        Remove-Item -LiteralPath $archiveHashPath -Force
    }

    Compress-Archive -LiteralPath $payloadDirectory `
        -DestinationPath $archivePath -CompressionLevel Optimal

    $archiveHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $archivePath).Hash
    Set-Content -LiteralPath $archiveHashPath (
        "$archiveHash  $([IO.Path]::GetFileName($archivePath))") -Encoding Ascii

    Write-Host "Created $archivePath"
    Write-Host "SHA-256: $archiveHash"
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}
