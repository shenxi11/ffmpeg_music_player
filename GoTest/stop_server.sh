#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "$0")" && pwd)"
PIDFILE="$DIR/music_server.pid"

if [[ ! -f "$PIDFILE" ]]; then
  echo "[stop] No PID file. Nothing to stop."
  exit 0
fi

PID="$(cat "$PIDFILE" 2>/dev/null || true)"
if [[ -z "$PID" ]]; then
  echo "[stop] Empty PID file, removing."
  rm -f "$PIDFILE"
  exit 0
fi

if ! kill -0 "$PID" 2>/dev/null; then
  echo "[stop] Process $PID not running, cleaning PID file."
  rm -f "$PIDFILE"
  exit 0
fi

echo "[stop] Stopping server (pid=$PID)..."
kill "$PID" || true

# Wait up to 10 seconds for graceful shutdown
for i in {1..20}; do
  if ! kill -0 "$PID" 2>/dev/null; then
    break
  fi
  sleep 0.5
done

if kill -0 "$PID" 2>/dev/null; then
  echo "[stop] Forcing kill (pid=$PID)"
  kill -9 "$PID" || true
fi

rm -f "$PIDFILE"
echo "[stop] Server stopped."

#!/bin/bash
cd "$(dirname "$0")"
if [ -f music_server.pid ]; then
  kill $(cat music_server.pid) && rm music_server.pid
  echo "Go音频服务器已关闭。"
else
  echo "未找到music_server.pid，请手动检查进程。"
fi
