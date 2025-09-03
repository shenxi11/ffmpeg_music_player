#!/bin/bash
cd "$(dirname "$0")"
CONFIG_FILE="config.yaml"
PORT=$(grep 'port:' $CONFIG_FILE | head -1 | awk '{print $2}')
nohup ./music_server -config $CONFIG_FILE > server.log 2>&1 &
echo $! > music_server.pid
echo "Go音频服务器已启动，监听端口: $PORT (日志: server.log)"
