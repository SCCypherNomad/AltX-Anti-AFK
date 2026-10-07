# Copyright (c) 2026 SCCypherNomad. SPDX-License-Identifier: MIT
param(
    [Parameter(Mandatory = $true)][string]$TccPath,
    [switch]$StartupOnly
)
$ErrorActionPreference = 'Stop'
$repoRootPath = Split-Path $PSScriptRoot -Parent
$testExePath = Join-Path $repoRootPath 'dist\AltXTimerIntegration.exe'
New-Item -ItemType Directory -Force -Path (Split-Path $testExePath -Parent) | Out-Null
$compilerPath = (Resolve-Path -LiteralPath $TccPath).Path
& $compilerPath '-Wall' (Join-Path $repoRootPath 'tests\AltXTimerIntegration.c') '-luser32' '-lgdi32' '-o' $testExePath
if ($LASTEXITCODE -ne 0) { throw 'Test compilation failed.' }
if ($StartupOnly) {
    Write-Output 'Startup-only test: real 10-second first timer, input cleanup and shutdown. The 10-minute repeat is not remeasured. Keyboard input is intercepted.'
    & $testExePath '--startup-only'
} else {
    Write-Output 'Testing the real 10-second and 10-minute timers. Keyboard input is intercepted.'
    & $testExePath
}
if ($LASTEXITCODE -ne 0) { throw 'Integration tests failed.' }
