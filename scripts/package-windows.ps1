param(
    [Parameter(Mandatory = $true)][string]$EngineRoot,
    [Parameter(Mandatory = $true)][string]$ArchiveDirectory,
    [Parameter(Mandatory = $true)][string]$EvidenceDirectory
)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path $PSScriptRoot -Parent
$Project = Join-Path $ProjectRoot 'DywxSurvivors.uproject'
$Build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Uat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
if (!(Test-Path $Build) -or !(Test-Path $Uat)) { throw 'A complete installed UE 5.8 toolchain is required.' }
New-Item -ItemType Directory -Force $ArchiveDirectory, $EvidenceDirectory | Out-Null
$EvidenceDirectory = (Resolve-Path $EvidenceDirectory).Path
$ArchiveDirectory = (Resolve-Path $ArchiveDirectory).Path
& $Build DywxSurvivorsEditor Win64 Development $Project -WaitMutex "-log=$EvidenceDirectory\UBT-Editor.log" -NoUBA
if ($LASTEXITCODE -ne 0) { throw "Editor build failed: $LASTEXITCODE" }
$env:uebp_LogFolder = Join-Path $EvidenceDirectory 'UAT'
& $Uat -NoCompileUAT BuildCookRun "-project=$Project" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$ArchiveDirectory" -prereqs -nodebuginfo -unattended -utf8output 2>&1 | Tee-Object -FilePath "$EvidenceDirectory\UAT-console.log"
if ($LASTEXITCODE -ne 0) { throw "BuildCookRun failed: $LASTEXITCODE" }
