# UI-MVVM 架构与调用链路说明（当前版）

更新时间：2026-05-07

## 1. 文档目标
本文档用于固化当前 Qt Widgets + QML 混合客户端的 UI 分层方式，作为后续开发协作和毕业论文“系统实现”“客户端架构设计”章节的直接参考。正文只描述当前运行版本，不把历史 QML 主壳、旧用户入口或旧 Agent sidecar 链路作为当前真相。

## 2. 当前 UI 组织方式
当前客户端采用“`QWidget` 壳层 + `QQuickWidget` 页面 + ViewModel 驱动业务状态”的组合模式：

- `main.cpp` 负责应用启动、欢迎页校验与主窗口装配；
- `MainWidget` 是桌面壳层，负责左侧贯穿导航、顶部工具栏、页面切换、账号入口、播放层和运行模式控制；
- 各功能页面通过 QWidget 宿主加载对应 QML 页面；
- QML 负责视觉呈现、局部状态和交互信号；
- 业务状态和请求入口统一下沉到 ViewModel；
- 音视频播放状态由服务层与 `PlaybackViewModel` 统一提供。

## 3. 当前 View 与 ViewModel 对应关系

| View / 页面 | 入口文件 | 主 ViewModel / 桥接对象 | 备注 |
|---|---|---|---|
| 主窗口壳层 | `src/app/main_widget.cpp` | `MainShellViewModel` | 聚合搜索、账号、歌单、推荐、热歌榜、评论、下载、欢迎页返回等主链 |
| 左侧账号入口 | `src/ui/qml_bridge/user/userwidget_qml.h` + `qml/components/user/UserWidget.qml` | `UserWidgetQml` + `MainWidget` | 当前入口位于左侧导航顶部，弹窗使用独立 QML Window |
| 个人主页 | `src/ui/widgets/user/user_profile_widget.cpp` + `qml/components/user/UserProfilePage.qml` | `UserProfileWidget` + `MainShellViewModel` | 资料刷新、用户名修改、头像上传、内容预览 |
| 推荐与热歌榜 | `src/ui/widgets/library/recommend_music_widget.cpp` + `RecommendMusicWidget.qml` | `MainShellViewModel` | 推荐列表、相似推荐、热歌榜周期切换 |
| 播放详情页 | `src/ui/widgets/playback/play_widget.cpp` | `PlaybackViewModel` | 当前曲目、封面、歌词、队列快照、播放状态 |
| 播放控制栏 | `qml/components/playback/ProcessSlider.qml` | `PlayWidget` + `SettingsManager` | 控制按钮、进度条、桌面歌词、评论、播放页风格入口 |
| 评论面板 | `CommentPanelQml` + `CommentPanel.qml` | `MainShellViewModel` | 在线歌曲评论读取、发布、回复、删除 |
| 登录页 | `LoginWidgetQml` + `LoginWindow.qml` | `LoginViewModel` | 登录、注册、密码重置与在线会话初始化 |
| 欢迎页 | `server_welcome_dialog.cpp` | `ServerWelcomeViewModel` | `/client/ping` / `/client/bootstrap` 验证与离线直进 |
| 本地音乐列表 | `music_list_widget_local.cpp` | `LocalMusicListViewModel` | 本地缓存、播放列表、封面回写联动 |
| 在线音乐列表 | `music_list_widget_net.cpp` | `OnlineMusicListViewModel` | 在线搜索、流地址解析、下载、收藏、歌单动作 |
| 本地与下载容器 | `local_and_download_widget.cpp` | 本地列表 + 下载模型桥接 | 承载本地音乐、下载音乐、正在下载页签 |
| 视频列表页 | `video_list_widget.cpp` | `VideoListViewModel` | 在线视频列表与视频播放入口 |
| 设置页 | `settings_widget.cpp` + `Settings.qml` | `SettingsViewModel` | 服务端、路径、播放页样式、桌面歌词、欢迎页返回 |

## 4. 当前主调用链

### 4.1 启动与欢迎页链路
1. `main.cpp` 初始化应用、日志、资源与主题；
2. 启动 `ServerWelcomeDialog`；
3. 欢迎页通过 `ServerWelcomeViewModel` 校验服务端配置，或走 `enterLocalOnly()` 直接进入本地模式；
4. `main.cpp` 将 `localOnlyMode` 传给 `MainWidget`；
5. `MainWidget` 完成页面装配、全局连接和运行模式收口。

### 4.2 主窗口与左侧导航链路
`MainWidget` 当前是主壳唯一真实入口。它负责：

- `leftWidget / MainLeftPanel`：左侧贯穿导航栏；
- `UserWidgetQml`：左侧导航顶部账号入口；
- `topWidget`：右侧内容区顶部搜索、菜单和窗口控制；
- `computeContentRect()`：右侧内容页布局；
- `updateSideNavLayout()`：账号入口、导航按钮、歌单区域定位；
- `PlayWidget`：收起时作为右侧内容区底部控制层，展开时覆盖全窗口播放页。

### 4.3 账号与个人资料链路
当前账号入口已从右上角移动到左侧导航顶部：

1. `UserWidget.qml` 显示头像和用户名；
2. `UserWidgetQml` 捕获点击并打开独立 `UserPopupWindow.qml`；
3. 登录、个人主页、退出登录信号进入 `MainWidget`；
4. `MainWidget` 调用 `MainShellViewModel` 执行资料刷新、用户名修改、头像上传或退出登录；
5. 结果回写 `SettingsManager`、`OnlinePresenceManager`、个人主页和账号入口。

### 4.4 推荐、热歌榜与列表动作链路
推荐页和热歌榜共用推荐页面宿主：

- `RecommendMusicWidget.qml` 发出推荐刷新、热歌榜刷新、歌曲动作信号；
- `MainWidget` 转发到 `MainShellViewModel::requestRecommendations()` 或 `requestHotChart()`；
- `MainShellViewModel` 调用 `HttpRequestV2`；
- 结果通过 `recommendationListReady`、`hotChartReady`、`hotChartRequestFailed` 回到 QML；
- 榜单歌曲复用在线歌曲播放、下载、收藏、加歌单和评论入口。

### 4.5 播放、评论与播放页样式链路
播放相关 UI 当前统一依赖 `PlaybackViewModel` 和 `PlayWidget`：

- 播放、暂停、切歌、进度、音量、播放模式由 `ProcessSlider.qml` 和 `PlayWidget` 协同触发；
- `PlaybackViewModel` 订阅 `AudioService`，统一输出当前歌曲、封面、歌手、时长、播放状态和队列；
- 评论入口绑定当前在线歌曲上下文，本地歌曲不进入在线评论链；
- 播放页风格通过 `PlayerStylePopup.qml` 选择，落到 `SettingsManager::playerPageStyle`，由 `qml/theme/PlayerStyle.js` 提供视觉 token；
- 展开播放页时主控制栏背景透明，使用播放页背景作为控制区背景。

### 4.6 桌面歌词与设置链路
桌面歌词配置由设置页集中维护：

- `Settings.qml` 读取和写入 `SettingsViewModel` 暴露的桌面歌词属性；
- `SettingsViewModel` 转发到 `SettingsManager`；
- `PlayWidget` 和桌面歌词窗口消费配置；
- 当前支持显示启动、置顶、覆盖任务栏、字体、字号、粗体、透明度、行模式、对齐、颜色预设、已播放/未播放/描边颜色等配置。

### 4.7 离线本地模式链路
离线直进是完整运行模式：

- 欢迎页点击“不连接，直接进入”后，`MainWidget` 进入 `m_localOnlyMode`；
- `applyLocalOnlyModeUi()` 隐藏在线导航、禁用搜索、屏蔽账号/个人主页和服务端动作；
- 本地、下载、本地播放、设置页和返回欢迎页能力继续可用；
- 业务入口层仍有 `m_localOnlyMode` 保护，避免误触在线请求。

## 5. 当前分层约束
1. View 不直接访问 `HttpRequestV2`、`SettingsManager`、`DownloadManager`、`AudioService` 等业务单例；
2. ViewModel 负责把服务端返回值转换成页面可消费的数据结构和状态；
3. 页面之间共享的播放状态只认 `PlaybackViewModel`；
4. 在线 / 离线本地运行模式只认 `MainWidget` 的统一控制；
5. `qml/MainView.qml` 不作为当前主窗口真相；
6. 当前仓库没有 `src/agent` 主链目录，旧 Agent / 自动化工具接口不应写成当前运行链。

## 6. 当前版本关键变化
- 主窗口改为左侧导航贯穿全高，账号入口移到左侧导航顶部；
- 左侧导航品牌图标和“云音乐”标题已移除；
- 播放页控制栏布局重构，进度条位于控制按钮下方；
- 播放页展开后保留全屏样式，控制栏背景透明；
- 播放页风格切换改为圆形衣服图标按钮；
- 推荐页接入在线热歌榜；
- 播放页接入在线音乐评论；
- 桌面歌词设置项和播放页风格 token 更完整；
- 全局控件视觉从方正风格调整为更柔和的圆角系统。

## 7. 论文引用建议
当前客户端可概括为：Widgets 主壳负责窗口和运行模式，QML 负责页面呈现，ViewModel 负责业务状态聚合，底层服务负责网络、播放、下载、本地缓存和配置。该分层让 UI 视觉调整、业务请求和播放状态同步可以在不破坏总架构的前提下持续迭代。
