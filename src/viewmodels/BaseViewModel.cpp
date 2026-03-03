#include "BaseViewModel.h"

BaseViewModel::BaseViewModel(QObject *parent)
    : QObject(parent), m_isBusy(false), m_hasError(false)
{}

void BaseViewModel::setIsBusy(bool busy)
{
    if (m_isBusy != busy) {
        m_isBusy = busy;
        emit isBusyChanged();
    }
}

void BaseViewModel::setErrorMessage(const QString& error)
{
    if (m_errorMessage != error) {
        m_errorMessage = error;
        m_hasError = !error.isEmpty();
        emit errorMessageChanged();
        emit hasErrorChanged();
    }
}

void BaseViewModel::clearError()
{
    setErrorMessage(QString());
}
