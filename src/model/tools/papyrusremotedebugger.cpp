#include "papyrusremotedebugger.hpp"

#include <QHostAddress>
#include <QJsonDocument>

#include "libs/files/log/logger.hpp"

PapyrusRemoteDebugger::PapyrusRemoteDebugger(QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::readyRead,
            this, &PapyrusRemoteDebugger::onReadyRead);
    connect(m_socket, &QTcpSocket::connected,
            this, &PapyrusRemoteDebugger::onConnected);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &PapyrusRemoteDebugger::onDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred,
            this, &PapyrusRemoteDebugger::onError);
}

void PapyrusRemoteDebugger::connectToHost(const QString& host, quint16 port)
{
    m_socket->connectToHost(host, port);
}

void PapyrusRemoteDebugger::disconnectFromHost()
{
    m_socket->disconnectFromHost();
}

bool PapyrusRemoteDebugger::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

QString PapyrusRemoteDebugger::buildCommand(const QString& command,
                                            const QVariantMap& args)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("cmd"), command);
    for (auto it = args.cbegin(); it != args.cend(); ++it)
        obj.insert(it.key(), QJsonValue::fromVariant(it.value()));
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

QVariantMap PapyrusRemoteDebugger::parseResponse(const QString& line)
{
    QVariantMap map;
    const QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
    if (doc.isObject())
        map = doc.object().toVariantMap();
    return map;
}

void PapyrusRemoteDebugger::setBreakpoint(const QString& scriptName, int line)
{
    QVariantMap args;
    args.insert(QStringLiteral("script"), scriptName);
    args.insert(QStringLiteral("line"), line);
    send(buildCommand(QStringLiteral("setBreakpoint"), args));
}

void PapyrusRemoteDebugger::clearBreakpoint(const QString& scriptName, int line)
{
    QVariantMap args;
    args.insert(QStringLiteral("script"), scriptName);
    args.insert(QStringLiteral("line"), line);
    send(buildCommand(QStringLiteral("clearBreakpoint"), args));
}

void PapyrusRemoteDebugger::requestLocals()
{
    send(buildCommand(QStringLiteral("getLocals")));
}

void PapyrusRemoteDebugger::requestStack()
{
    send(buildCommand(QStringLiteral("getStack")));
}

void PapyrusRemoteDebugger::watch(const QString& expression)
{
    QVariantMap args;
    args.insert(QStringLiteral("expr"), expression);
    send(buildCommand(QStringLiteral("watch"), args));
}

void PapyrusRemoteDebugger::step()
{
    send(buildCommand(QStringLiteral("step")));
}

void PapyrusRemoteDebugger::stepOver()
{
    send(buildCommand(QStringLiteral("stepOver")));
}

void PapyrusRemoteDebugger::stepOut()
{
    send(buildCommand(QStringLiteral("stepOut")));
}

void PapyrusRemoteDebugger::resume()
{
    send(buildCommand(QStringLiteral("resume")));
}

void PapyrusRemoteDebugger::pause()
{
    send(buildCommand(QStringLiteral("pause")));
}

void PapyrusRemoteDebugger::setWatchValue(const QString& expression,
                                          const QString& value)
{
    QVariantMap args;
    args.insert(QStringLiteral("expr"), expression);
    args.insert(QStringLiteral("value"), value);
    send(buildCommand(QStringLiteral("setWatchValue"), args));
}

void PapyrusRemoteDebugger::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    while (m_buffer.contains('\n'))
    {
        const int newline = m_buffer.indexOf('\n');
        const QByteArray line = m_buffer.left(newline).trimmed();
        m_buffer.remove(0, newline + 1);
        if (line.isEmpty())
            continue;

        const QVariantMap response = parseResponse(QString::fromUtf8(line));
        emit responseReceived(response);
    }
}

void PapyrusRemoteDebugger::onConnected()
{
    LOG_INFO("PapyrusRemoteDebugger: connected");
    emit connected();
}

void PapyrusRemoteDebugger::onDisconnected()
{
    LOG_INFO("PapyrusRemoteDebugger: disconnected");
    emit disconnected();
}

void PapyrusRemoteDebugger::onError(QAbstractSocket::SocketError error)
{
    emit socketError(m_socket->errorString());
    Q_UNUSED(error);
}

void PapyrusRemoteDebugger::send(const QString& line)
{
    if (!isConnected())
    {
        LOG_WARNING("PapyrusRemoteDebugger: not connected, dropping command");
        return;
    }
    m_socket->write(line.toUtf8());
    m_socket->write("\n", 1);
}
