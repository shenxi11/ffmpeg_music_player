# Git 工作规范（单人开发简化版）

## 1. 目标

这份规范只解决一个问题：
让当前仓库在单人开发场景下，既能保持 `main` 稳定，又不会因为功能交叉、PR 混杂、冲突过多而把开发节奏拖慢。

适用范围：
- 当前 Qt / QML / C++ 客户端仓库
- 单人开发，但需要通过 GitHub PR 做 review、回溯和阶段性合并

---

## 2. 分支原则

### 2.1 `main` 是稳定基线

`main` 的职责：
- 保持相对稳定
- 保持能编译
- 作为所有新功能的起点

默认工作习惯：
- 平时不长期停留在功能分支
- 一个主题功能合并完成后，回到 `main`

### 2.2 功能分支只承担“施工”

功能分支用于承载一个清晰主题，例如：
- `feature/local-only-entry-mode`
- `feature/user-profile-redesign`
- `fix/download-cover-sync`
- `refactor/playback-state-sync`

一个分支只做一个主主题，不要混多个大主题。

---

## 3. 什么时候必须开新分支

满足以下任意一条，就开新分支：

1. 一个需求会改多个文件
例如：
- 用户主页
- 欢迎页离线直进
- 下载封面同步
- 播放页布局重做

2. 你准备提一个独立 PR
只要要发 PR，就应该有单独分支。

3. 这次改动会碰核心主链
尤其是这些高冲突区域：
- `src/app/main_widget*`
- `src/viewmodels/PlaybackViewModel*`
- `src/network/httprequest_v2*`
- `qml/components/...` 的主页面与列表

4. 这次改动预计要做超过半天
这种功能通常会经历：
- 开发
- 联调
- review 修复
- 合并前再清理

不适合直接堆在 `main`。

---

## 4. 什么时候可以不再开子分支

如果你已经在一个主题分支里，以下两类改动可以继续在当前分支完成：

1. 同一主题下的小收尾
例如：
- 用户主页做完后，补一个头像刷新 bug
- 离线直进做完后，补一个欢迎页文案细节

2. 当前 PR 的 review 修复
review 修复默认继续在当前 PR 分支上做，不必再拆子分支。

---

## 5. 不推荐的做法

以下做法会显著增加你这个仓库的维护成本：

1. 在一个分支里同时做多个大主题
例如同时混：
- 图标替换
- 用户主页
- 离线模式
- 下载页修复
- 播放页布局

2. 合并完成后还长期停留在旧功能分支

3. 一个功能又拆成很多非常细的小分支
单人开发里，这会让管理成本大于收益。

---

## 6. 推荐分支命名

统一使用这三类前缀即可：

- `feature/`：新功能
- `fix/`：缺陷修复
- `refactor/`：重构

示例：
- `feature/local-only-entry-mode`
- `feature/user-profile-redesign`
- `fix/download-cover-hover-state`
- `fix/local-artist-persist`
- `refactor/player-page-layout`

---

## 7. 推荐提交粒度

每个 commit 最好满足“一句话能说清”的标准。

示例：
- `feat: add local-only entry mode from welcome screen`
- `fix: persist local artist metadata after playback`
- `fix: sync download cover metadata after playback`
- `refactor: normalize player page stage layout`

不推荐：
- 一个 commit 同时包含无关的 UI、网络、资料页、下载链改动

---

## 8. 标准工作流

### 8.1 开始一个新主题

```bash
git checkout main
git pull
git checkout -b feature/xxx
```

### 8.2 开发和自测

开发过程中建议至少保证：
- 关键页面能打开
- 主要链路不报错
- Debug 编译通过

当前仓库常用构建命令：

```bash
cmake --build E:/FFmpeg_whisper/ffmpeg_music_player_build --target ffmpeg_music_player --config Debug
```

### 8.3 提交和推送

```bash
git add <files>
git commit -m "feat: xxx"
git push origin feature/xxx
```

### 8.4 提 PR

PR 描述只围绕当前主题写，不混入其它历史改动。

### 8.5 review 修复

review 修复继续在当前功能分支处理，不再额外开子分支。

### 8.6 合并完成后

```bash
git checkout main
git pull
git branch
```

如果旧分支确认不再使用：

```bash
git branch -d feature/xxx
git push origin --delete feature/xxx
```

---

## 9. 当前仓库的特殊建议

这个仓库有几个高冲突区域，做功能时要特别注意：

### 9.1 高冲突文件区域

- `src/app/main_widget.cpp`
- `src/app/main_widget.h`
- `src/app/main_widget.library_connections.cpp`
- `src/app/main_widget.menu_auth.cpp`
- `src/app/main_widget.playback_connections.cpp`
- `src/viewmodels/PlaybackViewModel.cpp`
- `src/network/httprequest_v2.cpp`
- `qml/components/...` 下的主列表和播放页

只要需求会碰这些文件，优先单开分支，不建议顺手混做别的主题。

### 9.2 文档、脚本、实验性内容不要混进功能 PR

以下内容默认不应混入客户端功能 PR：

- `agent/...`
- `说明文档/...`
- `打印日志.txt`
- `毕业论文/...`

除非这次 PR 本身就是在提交这些内容。

### 9.3 功能分支合并前一定做一次编译

至少保证：

```bash
cmake --build E:/FFmpeg_whisper/ffmpeg_music_player_build --target ffmpeg_music_player --config Debug
```

如果有 QML 改动，建议额外检查：
- 关键页面是否能打开
- 是否有资源路径缺失
- 是否出现绑定错误或 hover / 状态逻辑异常

---

## 10. 适合当前项目的一句话规则

可以直接记这一句：

**`main` 负责稳定，功能分支负责施工；一个分支只做一个主题，合并后立刻回 `main`。**

---

## 11. 常用命令速查

### 新功能开始

```bash
git checkout main
git pull
git checkout -b feature/xxx
```

### 查看当前状态

```bash
git status
git branch
git log --oneline -5
```

### 提交当前主题

```bash
git add <files>
git commit -m "feat: xxx"
git push origin feature/xxx
```

### 合并后回到主线

```bash
git checkout main
git pull
```

### 删除已经完成的功能分支

```bash
git branch -d feature/xxx
git push origin --delete feature/xxx
```

### 检查 PR 是否有冲突（本地思路）

```bash
git fetch origin main
git checkout feature/xxx
git merge origin/main --no-commit --no-ff
```

如果只是演练，检查完可以：

```bash
git merge --abort
```

如果确定要解冲突并继续，就在当前 merge 状态下修完、编译、提交 merge commit。

---

## 12. 当前推荐执行方式

以后默认按这个节奏工作：

1. 回到 `main`
2. 拉最新
3. 新需求单开分支
4. 一个分支只做一个主题
5. review 修复继续在当前分支
6. 合并完成后回到 `main`

这样对当前这个播放器项目来说，复杂度和收益是最平衡的。
