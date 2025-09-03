#!/bin/bash
cd "$(dirname "$0")"
if [ -f music_server.pid ]; then
  kill $(cat music_server.pid) && rm music_server.pid
  echo "Go音频服务器已关闭。"
else
  echo "未找到music_server.pid，请手动检查进程。"
fi
