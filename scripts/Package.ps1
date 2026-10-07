# Copyright (c) 2026 SCCypherNomad. SPDX-License-Identifier: MIT
param(
    [Parameter(Mandatory = $true)][string]$TccPath,
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$repoRootPath = Split-Path $PSScriptRoot -Parent
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $repoRootPath 'dist\release' }
$releaseDirectoryPath = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $releaseDirectoryPath | Out-Null
& (Join-Path $PSScriptRoot 'Build.ps1') -TccPath $TccPath
$packageWorkPath = Join-Path $repoRootPath ('dist\package-' + [Guid]::NewGuid().ToString('N'))
$portablePath = Join-Path $packageWorkPath 'portable'
$sourcePath = Join-Path $packageWorkPath 'source'
New-Item -ItemType Directory -Force -Path $portablePath, $sourcePath | Out-Null
Copy-Item -LiteralPath (Join-Path $repoRootPath 'dist\AltXTimer.exe') -Destination $portablePath
Copy-Item -LiteralPath (Join-Path $repoRootPath 'README.md') -Destination (Join-Path $portablePath 'ReadMe.txt')
Copy-Item -LiteralPath (Join-Path $repoRootPath 'LICENSE') -Destination (Join-Path $portablePath 'LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $repoRootPath 'THIRD_PARTY_NOTICES.txt') -Destination $portablePath

foreach ($itemName in @('README.md', 'LICENSE', 'THIRD_PARTY_NOTICES.txt', 'CONTRIBUTING.md', '.gitignore', '.gitattributes', 'src', 'assets', 'scripts', 'tests', 'docs')) {
    $itemPath = Join-Path $repoRootPath $itemName
    if (Test-Path -LiteralPath $itemPath) { Copy-Item -LiteralPath $itemPath -Destination $sourcePath -Recurse }
}
$portableZipPath = Join-Path $releaseDirectoryPath 'AltX-windows-x64.zip'
$sourceZipPath = Join-Path $releaseDirectoryPath 'AltX-source.zip'
Compress-Archive -Path (Join-Path $portablePath '*') -DestinationPath $portableZipPath -Force
Compress-Archive -Path (Join-Path $sourcePath '*') -DestinationPath $sourceZipPath -Force
$hashLines = @($portableZipPath, $sourceZipPath) | ForEach-Object {
    $fileHash = Get-FileHash -Algorithm SHA256 -LiteralPath $_
    '{0}  {1}' -f $fileHash.Hash.ToLowerInvariant(), (Split-Path $_ -Leaf)
}
$hashLines | Set-Content -LiteralPath (Join-Path $releaseDirectoryPath 'SHA256SUMS.txt') -Encoding ascii
Write-Output "Release files are in $releaseDirectoryPath"
