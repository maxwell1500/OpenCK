#ifndef PAPYRUSREMOTEDEBUGGER_H
#define PAPYRUSREMOTEDEBUGGER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonValue>

// PapyrusRemoteDebugger implements the wire protocol used by the real
// PapyrusRemoteDebugger.exe against a running game (the Creation Kit
// attaches on TCP port 20548 by default). The protocol is line-oriented
// JSON-RPC-ish commands over a TCP socket. This client provides the
// breakpoint / locals / watch / step primitives the debugger UI needs.
class PapyrusRemoteDebugger : public QObject
{
    Q_OBJECT

public:
    // The default port the game listens on for Papyrus debugging.
    static constexpr quint16 kDefaultPort = 20548;

    explicit PapyrusRemoteDebugger(QObject* parent = nullptr);

    // Connects to the game's debugger socket. Emits connected() on success.
    void connectToHost(const QString& host = QStringLiteral("localhost"),
                       quint16 port = kDefaultPort);

    void disconnectFromHost();
    bool isConnected() const;

    // Builds a protocol command line (exposed for tests).
    static QString buildCommand(const QString& command,
                                const QVariantMap& args = {});

    // Parses a single received protocol response line (exposed for tests).
    static QVariantMap parseResponse(const QString& line);

    // Commands sent to the game.
    void setBreakpoint(const QString& scriptName, int line);
    void clearBreakpoint(const QString& scriptName, int line);
    void requestLocals();
    void requestStack();
    void watch(const QString& expression);
    void step();
    void stepOver();
    void stepOut();
    void resume();
    void pause();
    void setWatchValue(const QString& expression, const QString& value);

signals:
    void connected();
    void disconnected();
    void responseReceived(const QVariantMap& response);
    void socketError(const QString& error);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);

private:
    void send(const QString& line);

    QTcpSocket* m_socket;
    QByteArray m_buffer;
};

#endif // PAPYRUSREMOTEDEBUGGER_H
