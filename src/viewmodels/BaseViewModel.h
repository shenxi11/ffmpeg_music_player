#ifndef BASEVIEWMODEL_H
#define BASEVIEWMODEL_H

#include <QObject>
#include <QString>

/**
 * @brief ViewModel基类
 * 
 * 提供MVVM模式的基础设施，所有ViewModel继承此类
 * 
 * 职责:
 * - 提供通用的忙状态管理
 * - 提供统一的错误消息处理
 * - 强制子类使用Q_PROPERTY暴露状态
 * 
 * 使用示例:
 * @code
 * class PlaybackViewModel : public BaseViewModel {
 *     Q_OBJECT
 *     Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
 * public:
 *     Q_INVOKABLE void play() {
 *         setIsBusy(true);
 *         // ... 执行播放操作
 *         setIsBusy(false);
 *     }
 * };
 * @endcode
 */
class BaseViewModel : public QObject
{
    Q_OBJECT
    
    // 通用状态属性
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY isBusyChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY hasErrorChanged)
    
public:
    explicit BaseViewModel(QObject *parent = nullptr);
    
    virtual ~BaseViewModel() = default;
    
    // 属性访问器
    bool isBusy() const { return m_isBusy; }
    QString errorMessage() const { return m_errorMessage; }
    bool hasError() const { return m_hasError; }
    
protected:
    /**
     * @brief 设置忙状态
     * @param busy true表示正在执行耗时操作（显示加载动画）
     * 
     * 子类在执行网络请求、文件IO等操作时调用此方法
     */
    void setIsBusy(bool busy);
    
    /**
     * @brief 设置错误消息
     * @param error 错误描述，空字符串表示清除错误
     * 
     * 子类在操作失败时调用此方法
     */
    void setErrorMessage(const QString& error);
    
    /**
     * @brief 清除错误状态
     */
    void clearError();
    
signals:
    void isBusyChanged();
    void errorMessageChanged();
    void hasErrorChanged();
    
private:
    bool m_isBusy;          // 是否正在执行操作
    QString m_errorMessage;  // 错误消息
    bool m_hasError;        // 是否有错误
};

#endif // BASEVIEWMODEL_H
