#!/usr/bin/env bash
set -euo pipefail

# Right-click menu entry should call this script.
# Behavior:
# 1) If supervisor is running: do nothing (it will keep app alive).
# 2) If supervisor is not running: start supervisor, which starts PVEClient.

SUPERVISOR_BIN="${SUPERVISOR_BIN:-/home/pveclient/bin/pveclient-supervisor.sh}"
APP_BIN="${APP_BIN:-/usr/local/PVEClient/bin/PVEClient}"

if pgrep -u "${UID}" -f "pveclient-supervisor.sh" >/dev/null 2>&1; then
  exit 0
fi

if [[ ! -x "${SUPERVISOR_BIN}" ]]; then
  echo "ERROR: supervisor not executable: ${SUPERVISOR_BIN}" >&2
  exit 1
fi

APP_BIN="${APP_BIN}" "${SUPERVISOR_BIN}" >/dev/null 2>&1 &
