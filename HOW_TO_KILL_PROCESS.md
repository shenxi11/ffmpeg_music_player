# 关闭残留进程的方法

## 方法 1：通过任务管理器（GUI）

1. 按 `Ctrl + Shift + Esc` 打开任务管理器
2. 在"进程"或"详细信息"标签页中找到 `ffmpeg_music_player.exe`
3. 右键点击 → **结束任务** 或 **结束进程树**
4. 如果有多个，全部结束

## 方法 2：使用 PowerShell 命令

### 查看所有相关进程
```powershell
# 查看进程列表
tasklist | findstr ffmpeg_music_player

# 或使用 Get-Process
Get-Process | Where-Object {$_.ProcessName -like "*ffmpeg*"}
```

### 强制关闭进程
```powershell
# 方法 A：按名称关闭
taskkill /F /IM ffmpeg_music_player.exe

# 方法 B：按进程 ID 关闭（如果知道 PID）
taskkill /F /PID 12345

# 方法 C：使用 PowerShell
Stop-Process -Name "ffmpeg_music_player" -Force

# 方法 D：关闭所有匹配的进程
Get-Process | Where-Object {$_.ProcessName -like "*ffmpeg*"} | Stop-Process -Force
```

### 参数说明
- `/F` - 强制终止进程
- `/IM` - 按映像名称（程序名）
- `/PID` - 按进程 ID
- `-Force` - 强制停止，不提示确认

## 方法 3：创建快捷脚本

### 创建批处理文件 `kill_ffmpeg.bat`
```batch
@echo off
echo Killing ffmpeg_music_player processes...
taskkill /F /IM ffmpeg_music_player.exe 2>nul
if %errorlevel% == 0 (
    echo Process killed successfully.
) else (
    echo No process found or already terminated.
)
pause
```

### 创建 PowerShell 脚本 `kill_ffmpeg.ps1`
```powershell
Write-Host "Searching for ffmpeg_music_player processes..." -ForegroundColor Yellow

$processes = Get-Process -Name "*ffmpeg_music_player*" -ErrorAction SilentlyContinue

if ($processes) {
    Write-Host "Found $($processes.Count) process(es), terminating..." -ForegroundColor Red
    $processes | Stop-Process -Force
    Write-Host "All processes terminated." -ForegroundColor Green
} else {
    Write-Host "No ffmpeg_music_player process found." -ForegroundColor Cyan
}

Read-Host -Prompt "Press Enter to exit"
```

## 方法 4：在代码中防止多实例运行

### 添加互斥锁（推荐添加到 main.cpp）

```cpp
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 创建互斥锁，防止多实例运行
    QSystemSemaphore semaphore("FFmpegMusicPlayerSemaphore", 1);
    semaphore.acquire();

    // 使用共享内存检测是否已有实例运行
    QSharedMemory sharedMemory("FFmpegMusicPlayerInstance");
    bool isRunning = false;

    if (sharedMemory.attach()) {
        isRunning = true;
    } else {
        sharedMemory.create(1);
        isRunning = false;
    }

    semaphore.release();

    if (isRunning) {
        QMessageBox::warning(nullptr, "警告", 
            "程序已在运行中！\n请先关闭之前的实例。");
        return 0;
    }

    // ... 原有的代码 ...

    return a.exec();
}
```

## 方法 5：在 Visual Studio 中

### 调试时
1. 点击菜单栏 **调试** → **全部中断** (Ctrl+Alt+Break)
2. 或点击 **停止调试** (Shift+F5)

### 如果程序卡住
1. 点击菜单栏 **调试** → **附加到进程** (Ctrl+Alt+P)
2. 找到 `ffmpeg_music_player.exe`
3. 点击 **附加**
4. 再点击 **停止调试**

## 方法 6：重启电脑（最后手段）

如果进程完全卡死，无法关闭：
```powershell
# 查看进程详细信息
Get-Process ffmpeg_music_player | Format-List *

# 如果上述方法都不行，只能重启
shutdown /r /t 0
```

## 常用命令汇总

```powershell
# 查找进程
tasklist | findstr ffmpeg
Get-Process | Where-Object {$_.Name -like "*ffmpeg*"}

# 强制关闭
taskkill /F /IM ffmpeg_music_player.exe
Stop-Process -Name ffmpeg_music_player -Force

# 查看进程详情
Get-Process ffmpeg_music_player | Format-List *

# 查看进程的线程
Get-Process ffmpeg_music_player | Select-Object Id, Threads

# 查看进程占用的资源
Get-Process ffmpeg_music_player | Select-Object Name, CPU, WorkingSet
```

## 预防措施

### 1. 使用新修复的代码
我们刚才修复的线程清理代码应该可以防止进程残留。

### 2. 添加崩溃处理
```cpp
// 在 main.cpp 中添加
#include <csignal>

void signalHandler(int signal) {
    qDebug() << "Signal received:" << signal;
    
    // 清理资源
    QThreadPool::globalInstance()->waitForDone(1000);
    PluginManager::instance().unloadAllPlugins();
    
    exit(signal);
}

int main(int argc, char *argv[]) {
    // 注册信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGABRT, signalHandler);
    
    // ... 原有代码 ...
}
```

### 3. 添加应用程序单例检测
使用上面"方法 4"中的互斥锁代码。

## 推荐的使用流程

1. **开发时**：
   - 使用 VS 的"停止调试"
   - 如果卡住，用任务管理器结束

2. **测试时**：
   - 正常关闭窗口
   - 检查任务管理器确认进程已退出
   - 如果未退出，查看调试日志，找出哪个线程未停止

3. **如果有残留进程**：
   ```powershell
   # 快速关闭
   taskkill /F /IM ffmpeg_music_player.exe
   ```

4. **验证线程清理**：
   ```powershell
   # 运行程序前
   tasklist | findstr ffmpeg
   
   # 关闭程序后再查看
   tasklist | findstr ffmpeg
   
   # 应该看不到任何结果
   ```

## 立即执行

如果现在需要清理环境，运行：
```powershell
# 清理所有可能的残留进程
taskkill /F /IM ffmpeg_music_player.exe 2>nul
taskkill /F /IM qmake.exe 2>nul
taskkill /F /IM cmake.exe 2>nul

# 检查是否还有
tasklist | findstr -i "ffmpeg music player"
```

之后就可以安心重新编译和测试了。
