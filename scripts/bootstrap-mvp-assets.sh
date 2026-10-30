#!/usr/bin/env bash
# Headless UE asset authoring is possible only with an installed engine and built editor module.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ -z "${UE_ROOT:-}" ]]; then echo 'NOT_RUN: UE_ROOT unset; no engine installed by this script.' >&2; exit 2; fi
EDITOR="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd"
if [[ ! -x "$EDITOR" ]]; then EDITOR="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor"; fi
if [[ ! -x "$EDITOR" ]]; then echo 'NOT_RUN: Linux Unreal Editor executable is missing.' >&2; exit 2; fi
RECEIPT="$ROOT/Saved/MVP/material-bootstrap.json"
mkdir -p "$ROOT/Saved/MVP"
# Preserve any prior receipt, but never let it masquerade as success of this invocation.
if [[ -f "$RECEIPT" ]]; then mv -f -- "$RECEIPT" "$RECEIPT.previous"; fi
"$EDITOR" "$ROOT/DywxSurvivors.uproject" -run=pythonscript "-script=$ROOT/scripts/bootstrap-mvp-assets.py" -unattended -nop4 -nosplash -NullRHI -utf8output
[[ -f "$RECEIPT" ]] || { echo 'MVP bootstrap did not produce a success receipt.' >&2; exit 1; }
for asset in M_Surface M_VFX; do [[ -f "$ROOT/Content/MVP/$asset.uasset" ]] || exit 1; done
for cue in palm sword thunder hit pickup dash upgrade boss win loss; do [[ -f "$ROOT/Content/MVP/Audio/$cue.uasset" ]] || exit 1; done
echo 'Material/audio source assets created. This is not a rendered gameplay test.'
