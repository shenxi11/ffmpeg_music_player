#!/usr/bin/env bash
set -euo pipefail

# Start Go music server in background with PID/log management
# Usage: ./start_server.sh [config.yaml]

DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$DIR/music_server"
CONFIG="${1:-$DIR/config.yaml}"
PIDFILE="$DIR/music_server.pid"
LOGFILE="$DIR/server.log"

# Ensure binary exists
if [[ ! -x "$BIN" ]]; then
	echo "[start] Binary not found: $BIN" >&2
	echo "[start] Please build first: (cd $DIR && CGO_ENABLED=1 go build -v -o music_server main.go)" >&2
	exit 1
fi

# Export runtime lib path for local .so
export LD_LIBRARY_PATH="$DIR:${LD_LIBRARY_PATH:-}"

# If already running, do nothing
if [[ -f "$PIDFILE" ]]; then
	PID="$(cat "$PIDFILE" 2>/dev/null || true)"
	if [[ -n "${PID}" && -d "/proc/${PID}" ]]; then
		if ps -p "$PID" -o comm= 2>/dev/null | grep -q "music_server"; then
			echo "[start] Server already running (pid=$PID)"
			exit 0
		fi
	fi
fi

# Rotate log (keep one backup)
if [[ -f "$LOGFILE" ]]; then
	mv -f "$LOGFILE" "$LOGFILE.1" || true
fi

echo "[start] Starting server..."
nohup "$BIN" -config "$CONFIG" >>"$LOGFILE" 2>&1 &
PID=$!
echo "$PID" > "$PIDFILE"

sleep 0.5
if ! kill -0 "$PID" 2>/dev/null; then
	echo "[start] Failed to start. See $LOGFILE" >&2
	exit 1
fi

echo "[start] Server started (pid=$PID). Logs: $LOGFILE"

#!/bin/bash
cd "$(dirname "$0")"
CONFIG_FILE="config.yaml"
PORT=$(grep 'port:' $CONFIG_FILE | head -1 | awk '{print $2}')
nohup ./music_server -config $CONFIG_FILE > server.log 2>&1 &
echo $! > music_server.pid
echo "Go音频服务器已启动，监听端口: $PORT (日志: server.log)"
