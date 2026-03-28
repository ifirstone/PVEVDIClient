#!/usr/bin/env bash
set -euo pipefail

# PVEClient supervisor:
# - auto restart when app exits unexpectedly
# - stop supervision after 5 consecutive user_exit events

APP_BIN="${APP_BIN:-/usr/local/PVEClient/bin/PVEClient}"
APP_ARGS="${APP_ARGS:-}"
RUNTIME_RESET_SECONDS="${RUNTIME_RESET_SECONDS:-60}"
MAX_USER_EXITS="${MAX_USER_EXITS:-5}"

BASE_STATE_DIR="${PVECLIENT_STATE_DIR:-}"
if [[ -z "${BASE_STATE_DIR}" ]]; then
  if [[ -n "${XDG_RUNTIME_DIR:-}" ]]; then
    BASE_STATE_DIR="${XDG_RUNTIME_DIR}/pveclient-state"
  else
    BASE_STATE_DIR="/tmp/pveclient-state-${UID}"
  fi
fi

mkdir -p "${BASE_STATE_DIR}"
REASON_FILE="${BASE_STATE_DIR}/exit_reason"
COUNT_FILE="${BASE_STATE_DIR}/user_exit_count"
LOG_FILE="${BASE_STATE_DIR}/supervisor.log"

log() {
  printf '[%s] %s\n' "$(date '+%F %T')" "$*" | tee -a "${LOG_FILE}"
}

read_count() {
  if [[ -f "${COUNT_FILE}" ]]; then
    cat "${COUNT_FILE}"
  else
    echo 0
  fi
}

write_count() {
  echo "$1" > "${COUNT_FILE}"
}

if [[ ! -x "${APP_BIN}" ]]; then
  log "ERROR: app binary not executable: ${APP_BIN}"
  exit 1
fi

log "Supervisor started, app=${APP_BIN}, state=${BASE_STATE_DIR}"

while true; do
  rm -f "${REASON_FILE}"
  start_ts="$(date +%s)"

  if [[ -n "${APP_ARGS}" ]]; then
    PVECLIENT_STATE_DIR="${BASE_STATE_DIR}" "${APP_BIN}" ${APP_ARGS}
  else
    PVECLIENT_STATE_DIR="${BASE_STATE_DIR}" "${APP_BIN}"
  fi

  exit_code=$?
  end_ts="$(date +%s)"
  runtime=$((end_ts - start_ts))
  reason="unknown"
  [[ -f "${REASON_FILE}" ]] && reason="$(cat "${REASON_FILE}")"

  current_count="$(read_count)"

  # Any long stable run resets consecutive user-exit counter.
  if (( runtime >= RUNTIME_RESET_SECONDS )); then
    current_count=0
  fi

  if [[ "${reason}" == "user_exit" ]]; then
    current_count=$((current_count + 1))
    write_count "${current_count}"
    log "App exited by user, code=${exit_code}, runtime=${runtime}s, consecutive=${current_count}/${MAX_USER_EXITS}"

    if (( current_count >= MAX_USER_EXITS )); then
      log "Reached max consecutive user exits, supervisor will stop"
      exit 0
    fi
  else
    write_count 0
    log "App exited reason=${reason}, code=${exit_code}, runtime=${runtime}s, restarting"
  fi

  sleep 1
done
