#include "msghandlerwapper.h"
#include <QtCore/QMetaType>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QCoreApplication>
#include <QtMessageHandler>
void static msgHandlerFunction(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(context)
        QMetaObject::invokeMethod(MsgHandlerWapper::instance(), "message"
            , Q_ARG(QtMsgType, type)
            , Q_ARG(QString, msg));
}
MsgHandlerWapper* MsgHandlerWapper::m_instance = 0;
MsgHandlerWapper* MsgHandlerWapper::instance()
{
    static QMutex mutex;
    if (!m_instance) {
        QMutexLocker locker(&mutex);
        if (!m_instance)
            m_instance = new MsgHandlerWapper;
    }
    return m_instance;
}
MsgHandlerWapper::MsgHandlerWapper()
    :QObject(qApp)
{
    qRegisterMetaType<QtMsgType>("QtMsgType");
    qInstallMessageHandler(msgHandlerFunction);
}