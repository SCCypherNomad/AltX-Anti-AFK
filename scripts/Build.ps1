# Copyright (c) 2026 SCCypherNomad. SPDX-License-Identifier: MIT
param(
    [Parameter(Mandatory = $true)][string]$TccPath,
    [string]$OutputPath
)
$ErrorActionPreference = 'Stop'
$repoRootPath = Split-Path $PSScriptRoot -Parent
if (-not $OutputPath) { $OutputPath = Join-Path $repoRootPath 'dist\AltXTimer.exe' }
$compilerPath = (Resolve-Path -LiteralPath $TccPath).Path
$outputFullPath = [IO.Path]::GetFullPath($OutputPath)
New-Item -ItemType Directory -Force -Path (Split-Path $outputFullPath -Parent) | Out-Null
& $compilerPath '-Wall' '-Wl,-subsystem=gui' (Join-Path $repoRootPath 'src\AltXTimer.c') (Join-Path $repoRootPath 'assets\AltXTimer.o') '-luser32' '-lgdi32' '-o' $outputFullPath
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed.' }
$versionInfo = [Diagnostics.FileVersionInfo]::GetVersionInfo($outputFullPath)
if ($versionInfo.CompanyName -ne 'SCCypherNomad' -or $versionInfo.FileVersion -ne '1.1.1.0' -or $versionInfo.ProductName -ne 'AltX Anti-AFK') {
    throw 'Unexpected public release version or author metadata.'
}
Write-Output "Built $outputFullPath"
