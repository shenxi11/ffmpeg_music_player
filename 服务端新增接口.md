# 用户在线状态体系升级方案与接口文档

更新时间：2026-03-03

## 1. 背景与问题

原有在线状态依赖 `/users/ping` 直接刷新 Redis 时间戳，存在：

1. 缺少会话校验：仅凭 `account/username` 即可写在线，容易被伪造。
2. 缺少标准流程：客户端登录后没有统一在线会话创建和续约规范。
3. 缺少主动下线：客户端退出时无法显式通知服务端回收在线态。
4. 对接信息分散：客户端无法通过统一文档明确“何时调用什么接口”。

## 2. 详细改造 Plan

### Phase A：在线会话模型（已完成）

1. 定义在线会话 token（`session_token`），由服务端签发。
2. 建立 Redis 三类 key：
   - `user:online:account:{account}`：最后在线时间戳（TTL）。
   - `user:online:session:account:{account}`：account 当前会话 token（TTL）。
   - `user:online:session:token:{token}`：token 反查 account（TTL）。
3. 定义统一 TTL 与心跳建议间隔：
   - `online_ttl_sec = 600`
   - `heartbeat_interval_sec = 30`

### Phase B：接口体系（已完成）

1. 登录成功后返回在线会话参数（兼容字段扩展，不破坏旧客户端）。
2. 新增在线会话接口：
   - 创建会话
   - 心跳续约
   - 状态查询
   - 主动下线
3. 保留旧 `/users/ping` 作为兼容接口（仅建议旧客户端临时使用）。

### Phase C：网关与文档（已完成）

1. Docker/split Nginx 路由新增四个在线接口转发到 `auth-service`。
2. OpenAPI 补齐 presence 接口与 schema。
3. 形成客户端可直接对接的接口说明与调用时序。

## 3. 客户端接入流程（推荐）

1. 用户登录：`POST /users/login`
2. 读取返回中的：
   - `online_session_token`
   - `online_heartbeat_interval_sec`
   - `online_ttl_sec`
3. 启动心跳定时器：每 `online_heartbeat_interval_sec` 秒调用 `POST /users/online/heartbeat`
4. 前台或关键点校验可用 `GET /users/online/status`
5. 客户端退出/切换账号时调用 `POST /users/online/logout`

## 4. 接口定义

### 4.1 登录返回新增字段（兼容扩展）

接口：`POST /users/login`

新增响应字段：

- `online_session_token`：在线会话 token
- `online_heartbeat_interval_sec`：建议心跳间隔（秒）
- `online_ttl_sec`：在线 TTL（秒）

### 4.2 创建在线会话

- 方法：`POST`
- 路径：`/users/online/session/start`
- 认证：无（但需提供 account 或 username）

请求示例：

```json
{
  "account": "10001",
  "device_id": "desktop-win11"
}
```

成功响应示例：

```json
{
  "code": 0,
  "message": "success",
  "data": {
    "account": "10001",
    "session_token": "f4d7...9a1c",
    "heartbeat_interval_sec": 30,
    "online_ttl_sec": 600,
    "last_seen_at": 1772508000,
    "expire_at": 1772508600
  }
}
```

### 4.3 在线心跳（核心）

- 方法：`POST`
- 路径：`/users/online/heartbeat`
- 认证：`session_token` 必填

请求示例：

```json
{
  "account": "10001",
  "session_token": "f4d7...9a1c"
}
```

说明：

- 服务端会校验 token 与 account 的双向绑定。
- 校验通过后刷新在线时间与 TTL。

### 4.4 在线状态查询

- 方法：`GET`
- 路径：`/users/online/status`
- 参数：
  - `account` 或 `username`（二选一）
  - `session_token`（必填）

响应示例：

```json
{
  "code": 0,
  "message": "success",
  "data": {
    "account": "10001",
    "online": true,
    "last_seen_at": 1772508120,
    "ttl_remaining_sec": 598,
    "heartbeat_interval_sec": 30,
    "online_ttl_sec": 600
  }
}
```

### 4.5 主动下线

- 方法：`POST`
- 路径：`/users/online/logout`
- 认证：`session_token` 必填

请求示例：

```json
{
  "account": "10001",
  "session_token": "f4d7...9a1c"
}
```

成功响应：

```json
{
  "code": 0,
  "message": "success",
  "data": {
    "success": true
  }
}
```

## 5. 错误码与客户端处理建议

1. `401`（会话无效/过期）：跳转登录或重新创建在线会话。
2. `404`（用户不存在）：提示账号异常，停止心跳。
3. `400`（参数错误）：提示开发/请求参数问题，记录日志。

## 6. 兼容接口说明

- `POST /users/ping` 仍可用（兼容旧客户端），但不做 token 强校验。
- 新客户端应优先使用 `/users/online/*` 全套接口。
