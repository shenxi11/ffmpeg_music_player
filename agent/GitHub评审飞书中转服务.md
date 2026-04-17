# GitHub 评审飞书中转服务

## 目标

这份说明面向当前项目的联调环境，解决下面这个问题：

- GitHub Copilot / PR review 通过 GitHub Actions 转发飞书时，会被 `Approve and run workflows` 卡住
- 希望改成 GitHub 直接把 webhook 打到本地/自部署服务，再由服务把摘要转发到飞书机器人

当前实现位置：

- `agent/src/music_agent/github_review_relay.py`

## 能力

当前服务支持：

- `GET /healthz`
- `POST /webhooks/github`

当前已处理的 GitHub 事件：

- `pull_request_review`
- `pull_request_review_comment`
- `ping`

当前默认策略：

- 只转发 reviewer login 中包含 `copilot` 的 review
- 默认不转发逐条 inline review comment

## 环境变量

可直接写到 `agent/.env`，或放到系统环境变量：

```env
GITHUB_REVIEW_RELAY_HOST=127.0.0.1
GITHUB_REVIEW_RELAY_PORT=8771
GITHUB_REVIEW_RELAY_TIMEOUT_SECONDS=10

FEISHU_BOT_WEBHOOK=https://open.feishu.cn/open-apis/bot/v2/hook/xxxx
GITHUB_WEBHOOK_SECRET=your_github_webhook_secret

RELAY_NOTIFY_COPILOT_ONLY=true
RELAY_NOTIFY_REVIEW_COMMENTS=false
```

说明：

- `FEISHU_BOT_WEBHOOK` 必填
- `GITHUB_WEBHOOK_SECRET` 建议配置，用于校验 GitHub webhook 签名
- `RELAY_NOTIFY_COPILOT_ONLY=true` 时，只转发 Copilot review
- `RELAY_NOTIFY_REVIEW_COMMENTS=false` 时，不转发逐条 review comment

## 启动方式

在 `agent/` 目录执行：

```powershell
.venv\Scripts\python.exe -m pip install -e .
.venv\Scripts\music-agent-github-review-relay.exe
```

或：

```powershell
.venv\Scripts\python.exe -m music_agent.github_review_relay
```

默认监听：

- `http://127.0.0.1:8771`

## GitHub 侧配置

仓库：

- `Settings -> Webhooks -> Add webhook`

建议配置：

- Payload URL:
  - `http://<你的可访问地址>:8771/webhooks/github`
- Content type:
  - `application/json`
- Secret:
  - 与 `GITHUB_WEBHOOK_SECRET` 保持一致
- SSL verification:
  - 按你的实际部署环境选择
- Events:
  - 选择 `Let me select individual events`
  - 勾选：
    - `Pull request reviews`
    - `Pull request review comments`

## 本机直连提醒

如果服务只跑在你本机：

- GitHub 公网 webhook 不能直接访问 `127.0.0.1`

这时需要任选一种方式：

1. 部署到一台公网可访问的服务器
2. 用反向隧道工具把本地端口暴露出去
3. 部署到你自己的云主机或内网穿透地址

## 健康检查

```powershell
curl http://127.0.0.1:8771/healthz
```

预期返回字段：

- `feishuWebhookConfigured`
- `githubWebhookSecretConfigured`
- `copilotOnly`
- `reviewCommentsEnabled`

## 联调建议

1. 先用 `ping` 事件验证 GitHub webhook 能否打到服务
2. 再在 PR 上触发一次 Copilot review
3. 看服务端日志和飞书群是否收到消息
4. 如果 review comment 太吵，保持：
   - `RELAY_NOTIFY_REVIEW_COMMENTS=false`

## 当前边界

- 当前只转发摘要文本，不做富卡片
- 当前没有做去重与聚合
- 当前没有持久化消息记录
- 当前不替代 GitHub Actions，只是绕过 Actions 审批门槛做通知
