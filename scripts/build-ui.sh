#!/usr/bin/env bash
# Native Linux Unreal build. Never downloads an engine or substitutes a mock build.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ $# -gt 1 || "${1:-}" == "--help" ]]; then
  echo 'Usage: UE_ROOT=/absolute/path/to/UnrealEngine scripts/build-ui.sh [--check]'
  exit 0
fi
if [[ $# -eq 1 && "$1" != "--check" ]]; then
  echo "Unknown option: $1" >&2
  exit 2
fi
if [[ -z "${UE_ROOT:-}" ]]; then
  echo 'NOT_RUN: UE_ROOT is unset. Supply an existing licensed Linux UE5 installation.' >&2
  exit 2
fi
BUILD="$UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh"
EDITOR="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor"
if [[ ! -f "$BUILD" || ! -x "$EDITOR" ]]; then
  echo 'NOT_RUN: UE_ROOT lacks Build.sh or executable UnrealEditor. No engine was installed.' >&2
  exit 2
fi
if [[ "${1:-}" == "--check" ]]; then
  echo 'Engine files found. This check does not establish compilation or launch success.'
  exit 0
fi
exec bash "$BUILD" DywxSurvivorsEditor Linux Development "$ROOT/DywxSurvivors.uproject" -WaitMutex
