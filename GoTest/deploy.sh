#!/bin/bash
set -e


# 1. 自动安装依赖（支持Ubuntu/Debian）
echo "[1/5] 检查并安装Go、MySQL客户端、Redis客户端..."
if ! command -v go >/dev/null 2>&1; then
  echo "未检测到Go，正在安装..."
  sudo apt update && sudo apt install -y golang-go
fi
if ! command -v mysql >/dev/null 2>&1; then
  echo "未检测到mysql客户端，正在安装..."
  sudo apt install -y mysql-client
fi
if ! command -v redis-cli >/dev/null 2>&1; then
  echo "未检测到redis-cli，正在安装..."
  sudo apt install -y redis-tools
fi
if ! command -v git >/dev/null 2>&1; then
  echo "未检测到git，正在安装..."
  sudo apt install -y git
fi


# 2. 进入项目目录
cd "$(dirname "$0")"

# 3. 读取配置
CONFIG_FILE="config.yaml"
if [ ! -f "$CONFIG_FILE" ]; then
  echo "配置文件 $CONFIG_FILE 不存在，请先创建！"
  exit 1
fi


# 4. 安装Go依赖并编译项目
echo "[2/5] 安装Go依赖..."
if [ -f go.mod ]; then
  go mod tidy
else
  go mod init music_server && go mod tidy
fi
echo "[3/5] 编译Go服务..."
if [ -f main.go ]; then
  go build -o music_server main.go
else
  echo "main.go 未找到！"
  exit 1
fi

# 5. 检查静态资源和上传目录
STATIC_DIR=$(grep static_dir $CONFIG_FILE | awk '{print $2}' | tr -d '"')
UPLOAD_DIR=$(grep upload_dir $CONFIG_FILE | awk '{print $2}' | tr -d '"')
[ ! -d "$STATIC_DIR" ] && mkdir -p "$STATIC_DIR"
[ ! -d "$UPLOAD_DIR" ] && mkdir -p "$UPLOAD_DIR"

chmod +x start_server.sh stop_server.sh

echo "部署完成，可用 ./start_server.sh 启动服务。"
