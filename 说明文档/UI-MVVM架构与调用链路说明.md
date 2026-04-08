# UI-MVVM 架构与调用链路说明（当前版）

## 1. 文档目标
本文档用于固化当前 Qt Widgets + QML 混合客户端的 UI 分层方式，作为后续开发协作和毕业论文“系统实现”“客户端架构设计”章节的直接参考。正文只描述当前运行版本，不再混写已废弃的旧链路。

## 2. 当前 UI 组织方式
当前客户端采用“`QWidget` 壳层 + `QQuickWidget` 页面 + ViewModel 驱动业务状态”的组合模式：

- `main.cpp` 负责应用启动、欢迎页校验与主窗口装配；
- `MainWidget` 是桌面壳层，负责页面切换、导航、全局连接和运行模式控制；
- 各功能页面通过 `QQuickWidget` 加载对应 QML 页面；
- 业务状态和请求入口统一下沉到 ViewModel；
- 音视频播放状态继续由服务层与播放 ViewModel 统一提供。

## 3. 当前 View 与 ViewModel 对应关系
| View / 页面 | 入口文件 | 主 ViewModel / 桥接对象 | 备注 |
|---|---|---|---|
| 主窗口壳层 | `src/app/main_widget.cpp` | `src/viewmodels/MainShellViewModel.*` | 聚合搜索、账号、歌单、推荐、下载、欢迎页返回等主链 |
| 播放详情页 | `src/ui/widgets/playback/play_widget.cpp` | `src/viewmodels/PlaybackViewModel.*` | 管理当前曲目、封面、歌词、队列快照与播放状态 |
| 登录页 | `src/ui/widgets/auth/loginwidget.cpp` + `src/ui/qml_bridge/auth/loginwidget_qml.h` | `src/viewmodels/LoginViewModel.*` | 负责登录、注册、密码重置与在线会话初始化 |
| 欢迎页 | `src/ui/widgets/auth/server_welcome_dialog.cpp` | `src/viewmodels/ServerWelcomeViewModel.*` | 负责 `/client/ping` / `/client/bootstrap` 验证与离线直进 |
| 用户入口 / 头像弹窗 | `src/ui/widgets/user/user_widget.cpp` + `src/ui/qml_bridge/user/userwidget_qml.h` | `UserWidgetQml` + `MainWidget` | 负责登录入口、资料弹窗和头像显示 |
| 个人主页 | `src/ui/widgets/user/user_profile_widget.cpp` + `qml/components/user/UserProfilePage.qml` | `UserProfileWidget` + `MainShellViewModel` | 负责资料刷新、用户名修改、头像上传、内容预览 |
| 本地音乐列表 | `src/ui/widgets/library/music_list_widget_local.cpp` | `src/viewmodels/LocalMusicListViewModel.*` | 与本地缓存、播放列表、封面回写联动 |
| 在线音乐列表 | `src/ui/widgets/library/music_list_widget_net.cpp` | `src/viewmodels/OnlineMusicListViewModel.*` | 在线搜索、流地址解析、下载、收藏/歌单动作 |
| 本地与下载容器 | `src/ui/widgets/library/local_and_download_widget.cpp` | 本地列表 + 下载模型桥接 | 承载“本地音乐 / 下载音乐 / 正在下载”页签 |
| 视频列表页 | `src/ui/widgets/video/video_list_widget.cpp` | `src/viewmodels/VideoListViewModel.*` | 在线视频列表与视频播放入口 |
| 设置页 | `src/ui/widgets/settings/settings_widget.cpp` | `src/viewmodels/SettingsViewModel.*` | 配置与欢迎页返回入口 |

## 4. 当前主调用链

### 4.1 启动与欢迎页链路
1. `main.cpp` 初始化应用、日志、资源与主题；
2. 启动 `ServerWelcomeDialog`；
3. 欢迎页通过 `ServerWelcomeViewModel` 校验服务端配置，或走 `enterLocalOnly()` 直接进入本地模式；
4. `main.cpp` 将 `localOnlyMode` 传给 `MainWidget`；
5. `MainWidget` 完成全局页面装配、连接信号并根据运行模式执行 UI 收口。

### 4.2 主窗口与主壳 ViewModel 链路
`MainWidget` 只负责页面切换与 UI 协调；所有与账号、资料、推荐、歌单、收藏、历史、欢迎页返回相关的业务操作都通过 `MainShellViewModel` 发起。典型模式为：

- Widget/QML 发出业务信号；
- `MainWidget` 负责转发与局部协调；
- `MainShellViewModel` 调用 `HttpRequestV2`、`SettingsManager`、`PluginManager`、`LocalMusicCache` 等底层能力；
- 结果通过信号回到 `MainWidget` 和对应页面。

### 4.3 播放链路
播放相关 UI 当前统一依赖 `PlaybackViewModel`：

- 播放、暂停、切歌、播放模式切换由 `PlayWidget` 与控制栏发起；
- `PlaybackViewModel` 调用 `AudioService` 管理播放列表与当前会话；
- `PlaybackViewModel` 订阅 `AudioService` 信号，统一输出 `currentTitle/currentArtist/currentAlbumArt/currentUrl/isPlaying/playlistSnapshot`；
- `PlayWidget`、`PlaylistHistory`、列表页封面动作等都围绕同一播放状态源刷新。

### 4.4 个人资料链路
当前用户资料链路已从“头像弹窗零散入口”收敛为一套完整 MVVM 流程：

1. `UserProfilePage.qml` 发出刷新、保存用户名、选择头像等信号；
2. `UserProfileWidget` 将信号转为 QWidget 层事件；
3. `MainWidget` 调用 `MainShellViewModel::requestUserProfile/updateUsername/uploadAvatar`；
4. `MainShellViewModel` 调用 `HttpRequestV2`，并通过 `SettingsManager`/`OnlinePresenceManager` 回写缓存与在线会话；
5. 结果再同步回个人主页、顶部头像和用户弹窗。

### 4.5 离线本地模式链路
离线直进不是单纯跳过欢迎页，而是完整的运行模式：

- 欢迎页点击“`不连接，直接进入`”后，`MainWidget` 进入 `m_localOnlyMode`；
- `MainWidget::applyLocalOnlyModeUi()` 负责隐藏在线导航、禁用搜索、屏蔽账号/个人主页和服务端动作；
- 同时保留本地与下载、本地播放、设置页和返回欢迎页能力；
- 业务入口层仍有 `m_localOnlyMode` 保护，避免误触在线请求。

## 5. 当前分层约束
1. View 不直接访问 `HttpRequestV2`、`SettingsManager`、`DownloadManager`、`AudioService` 等业务单例；
2. ViewModel 负责把服务端返回值转换成页面可消费的数据结构和状态；
3. 页面之间共享的播放状态只认 `PlaybackViewModel`，不允许每个页面维护独立真相；
4. 主壳层运行模式（在线 / 离线本地）只认 `MainWidget` 的统一控制，不允许功能页自行推断。

## 6. 与 3 月版本相比的关键变化
- 新增“欢迎页服务验证 + 离线直进”双入口；
- 新增个人主页页面，资料刷新、用户名修改、头像上传形成完整链路；
- 列表封面动作统一收敛到 `SongCoverAction` 与统一播放状态；
- 下载页、本地页、历史页与播放队列之间的封面/歌手/时长回写更完整；
- 主窗口从单纯页面壳层演化为“运行模式 + 资料流 + 播放流 + 列表流”的聚合层。

## 7. 论文引用建议
若后续论文需要描述客户端前端架构，可直接引用以下结论：

- 当前客户端采用 Widgets 与 QML 混合架构，以 `MainWidget` 作为壳层，以 `MainShellViewModel` 和 `PlaybackViewModel` 作为两条主状态轴；
- 资料、歌单、推荐、历史、收藏、欢迎页验证等业务状态统一汇聚到 `MainShellViewModel`；
- 播放状态、队列快照、封面与歌词联动统一汇聚到 `PlaybackViewModel`；
- 通过这种分层，页面视觉调整、业务请求和播放状态同步可以在不破坏总架构的前提下持续迭代。