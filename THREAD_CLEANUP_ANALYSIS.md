# 线程未退出问题分析和解决方案

## 问题诊断

### 发现的线程问题

1. **Worker 类的 std::thread**
   - `onTimeOut()` 函数在无限循环中运行
   - 虽然有 `m_stopFlag` 和 `m_breakFlag`，但没有正确设置停止标志
   
2. **PlayWidget 中的 QThread (a, b, c)**
   - 析构函数中有 `quit()` 和 `wait()`，但可能被移动的对象没有正确访问
   - 使用了 `std::shared_ptr` 管理 Worker 和 TakePcm，可能导致析构顺序问题

3. **TakePcm 的 decode() 循环**
   - `decode()` 在无限 while(1) 循环中
   - 没有检查停止标志
   - 线程可能卡在 `av_read_frame()` 阻塞调用中

4. **TranslateWidget 中的 QThread**
   - 创建了新线程但没有保存指针
   - 没有在析构时清理

5. **QtConcurrent 和 QThreadPool**
   - `QtConcurrent::run()` 启动的任务没有取消机制
   - `QThreadPool::globalInstance()` 的任务可能还在运行

## 解决方案

### 1. 为 TakePcm 添加停止标志
### 2. 正确清理 Worker 的线程
### 3. 确保所有 QThread 正确退出
### 4. 在 MainWidget 析构时清理所有资源
### 5. 等待所有异步任务完成

