#!/usr/bin/env bash
# Build a Linux native package, never a Windows EXE. No engine installation/login.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ $# -ne 1 ]]; then echo 'Usage: UE_ROOT=/path/to/UE scripts/package-linux.sh /absolute/archive-directory' >&2; exit 2; fi
if [[ "$1" != /* ]]; then echo 'Archive path must be absolute.' >&2; exit 2; fi
bash "$ROOT/scripts/build-ui.sh"
bash "$ROOT/scripts/bootstrap-mvp-assets.sh"
UAT="$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh"
[[ -f "$UAT" ]] || { echo 'Missing Linux RunUAT.sh' >&2; exit 2; }
exec bash "$UAT" BuildCookRun "-project=$ROOT/DywxSurvivors.uproject" -noP4 -platform=Linux -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$1" -nodebuginfo -unattended -utf8output
