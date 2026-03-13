#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

find "$ROOT" -type d -name build -prune -print -exec rm -rf {} +

printf "Removed build/ folders under %s\n" "$ROOT"
