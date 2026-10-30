param([Parameter(Mandatory = $true)][string]$EngineRoot)
$ErrorActionPreference = 'Stop'
$Root = Split-Path $PSScriptRoot -Parent
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project = Join-Path $Root 'DywxSurvivors.uproject'
$Script = Join-Path $PSScriptRoot 'bootstrap-mvp-assets.py'
$Receipt = Join-Path $Root 'Saved\MVP\material-bootstrap.json'
if (!(Test-Path $Editor)) { throw 'UnrealEditor-Cmd.exe is missing. Install UE 5.8 first.' }
if (Test-Path $Receipt) { Remove-Item $Receipt }
& $Editor $Project -run=pythonscript "-script=$Script" -unattended -nop4 -nosplash -NullRHI -utf8output
if ($LASTEXITCODE -ne 0 -or !(Test-Path $Receipt)) { throw 'MVP material bootstrap failed. Inspect Saved/Logs; do not cook a diagnostic-material build.' }
foreach ($Asset in @('M_Surface.uasset', 'M_VFX.uasset')) {
    if (!(Test-Path (Join-Path $Root "Content\MVP\$Asset"))) { throw "Missing required material $Asset" }
}
foreach ($Cue in @('palm','sword','thunder','hit','pickup','dash','upgrade','boss','win','loss')) {
    if (!(Test-Path (Join-Path $Root "Content\MVP\Audio\$Cue.uasset"))) { throw "Missing required cooked-source audio $Cue" }
}
Write-Host 'MVP material assets generated; GPU appearance remains to be tested in UE.'
