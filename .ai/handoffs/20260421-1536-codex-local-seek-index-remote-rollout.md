# Goal

完成“桌面端本地在线音乐 seek 延迟优化”这轮端到端落地，重点补齐服务端 `/music/local/seek-index` 的正式部署与远程验收，并把仓库协作记录回写到 `.ai/*`。

# Completed

- 核对并纠正了 repo-local `cloudmusic-server-workflow` skill，把虚拟机唯一开发目录固定为 `shen@192.168.1.208:/home/shen/microservice-deploy`。
- 复核当前客户端仓库状态，确认 `src/audio/AudioCacheManager*`、`src/audio/AudioSession*`、`src/audio/AudioDecoder*` 的 seek-index 客户端接入已经在本地脏工作树中落地，并且 `E:/FFmpeg_whisper/ffmpeg_music_player_build` 下的 `Debug` 构建已通过。
- 在虚拟机开发仓库 `shen@192.168.1.208:/home/shen/microservice-deploy` 上继续完成服务端修复：
  - 把 `internal/media/handler/seek_index.go` 的探测路径从“单次 `ffprobe -show_packets` 全量 JSON”改成“两次轻量 ffprobe：duration + CSV packets”。
  - 保留现有内存/磁盘缓存协议，但给 `buildSeekIndexVersion()` 增加算法版本盐 `csv-v2`，避免旧版 unsupported 缓存把新实现永久卡死。
  - 更新 `internal/media/handler/seek_index_test.go` 的 fake ffprobe 脚本以匹配新探测方式。
- 在虚拟机开发仓库通过单测：
  - `go test ./internal/media/handler -run SeekIndex -count=1`
- 服务端开发仓库新增并推送了两个提交：
  - `68d8908 fix(media): speed up local seek index probing`
  - `906b01c fix(media): invalidate stale seek index cache`
- 远程正式目录 `root@122.51.73.110:/root/CloudMusic` 由于直连 GitHub `git fetch` 仍受 TLS 中断影响，采用 git bundle 快进到 `906b01c`，然后用从虚拟机构建出来的 `media_server` 二进制注入现有 `cloudmusic:local` 镜像，只重建 `media-service` 完成正式部署。
- 远程正式环境已验证：
  - `GET http://127.0.0.1:8080/health` 返回 `status=ok`
  - `GET /music/local/seek-index?music_path=暗号/暗号.mp3` 返回 `supported=true`
  - 实际返回 `182` 个 seek 点，日志显示首次未命中缓存生成耗时约 `838ms`
  - `GET /music/local/seek-index?music_path=../bad.mp3` 返回 `400`

# Changed Scope

- 本仓库：
  - `.codex/skills/cloudmusic-server-workflow/SKILL.md`
  - `.ai/ownership.yaml`
  - `.ai/tasks.yaml`
  - `.ai/handoffs/20260421-1536-codex-local-seek-index-remote-rollout.md`
- 虚拟机开发仓库 `shen@192.168.1.208:/home/shen/microservice-deploy`：
  - `internal/media/handler/seek_index.go`
  - `internal/media/handler/seek_index_test.go`
- 远程正式部署目录 `root@122.51.73.110:/root/CloudMusic`

# Open Work

- 在桌面客户端里连到远程正式环境 `122.51.73.110:8080`，对服务器本地 MP3/AAC 实际拖动进度条做体验回归，确认：
  - `seek_index_hit=true`
  - 单次拖动不会再出现多轮明显可感知缓冲
  - 暖缓存恢复出声时间接近计划目标
- 如果后续要把这套部署恢复成“纯脚本构建”而不是二进制注入，需要继续处理远程正式服务器对 GitHub / Docker Hub 的外网不稳定问题。
- 远程正式机上这次部署留下了 bundle 和临时二进制：
  - `/root/cloudmusic-main-906b01c.bundle`
  - `/root/media_server_906b01c`
  需要后续清理。

# Risks

- 当前客户端仓库工作树非常脏，且 `playback-and-audio` 周边文件已有大量并行改动；后续会话不要在没看 `.ai/*` 的情况下直接重写 `src/audio/` 或 `qml/components/playback/`。
- 远程正式目录 ` /root/CloudMusic ` 现在是通过 bundle 快进到新提交，因此相对 `origin/main` 仍显示 `ahead`；这是因为远程机器对 GitHub 直连 `git fetch` 不稳定，不是代码未推送。
- 这次远程服务更新使用的是“虚拟机构建 media_server + 注入现有镜像”的保守路径，不是一次完整的 Dockerfile 重建；如果要追求完全可重放的正式部署，需要先修好远程构建网络链路。

# Next Entry Point

- 客户端继续验证时，先看：
  - `src/audio/AudioCacheManager.cpp`
  - `src/audio/AudioSession.cpp`
  - `src/audio/AudioDecoder.cpp`
- 服务端继续排查或增强时，先看虚拟机开发仓库：
  - `/home/shen/microservice-deploy/internal/media/handler/seek_index.go`
  - `/home/shen/microservice-deploy/internal/media/handler/seek_index_test.go`
- 正式环境接口入口：
  - `http://122.51.73.110:8080/music/local/seek-index?music_path=<urlencoded-relative-path>`
- 本轮远程验收样本：
  - `music_path=暗号/暗号.mp3`
