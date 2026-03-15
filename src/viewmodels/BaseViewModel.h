#ifndef BASEVIEWMODEL_H
#define BASEVIEWMODEL_H

#include <QObject>
#include <QString>

/**
 * @brief ViewModel 基类。
 *
 * 统一提供 MVVM 公共状态能力，所有页面级 ViewModel 都应继承该类。
 * 主要职责：
 * 1. 维护通用忙状态（isBusy），用于控制加载动画和交互禁用。
 * 2. 维护统一错误状态（errorMessage / hasError），用于界面提示。
 * 3. 约束子类通过属性与信号对外暴露状态，避免视图直接访问服务层。
 */
class BaseViewModel : public QObject
{
    Q_OBJECT

    // 通用状态属性
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY isBusyChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY hasErrorChanged)

public:
    explicit BaseViewModel(QObject* parent = nullptr);
    virtual ~BaseViewModel() = default;

    // 属性访问器
    bool isBusy() const { return m_isBusy; }
    QString errorMessage() const { return m_errorMessage; }
    bool hasError() const { return m_hasError; }

protected:
    /**
     * @brief 设置忙状态。
     * @param busy true 表示正在执行耗时任务。
     */
    void setIsBusy(bool busy);

    /**
     * @brief 设置错误消息并更新错误状态。
     * @param error 错误描述，空字符串表示清除错误。
     */
    void setErrorMessage(const QString& error);

    /**
     * @brief 清除错误状态。
     */
    void clearError();

signals:
    void isBusyChanged();
    void errorMessageChanged();
    void hasErrorChanged();

private:
    bool m_isBusy = false;     // 是否处于忙状态
    QString m_errorMessage;    // 当前错误消息
    bool m_hasError = false;   // 是否存在错误
};

#endif // BASEVIEWMODEL_H
