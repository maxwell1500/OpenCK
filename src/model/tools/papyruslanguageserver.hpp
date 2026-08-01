#ifndef PAPYRUSLANGUAGESERVER_H
#define PAPYRUSLANGUAGESERVER_H

#include <QObject>
#include <QProcess>
#include <QByteArray>
#include <QJsonObject>

// PapyrusLanguageServer wraps an external Papyrus LSP server (the Antlr4
// based language server shipped with vscodepapyrus) over the Language Server
// Protocol. It owns the child process, speaks the LSP Content-Length framed
// JSON-RPC transport, and exposes the standard requests the editor needs:
// initialize, textDocument/didOpen, didChange, hover, and completion.
//
// The transport (framing + message building) is separated so it can be
// unit-tested without launching a real server.
class PapyrusLanguageServer : public QObject
{
    Q_OBJECT

public:
    explicit PapyrusLanguageServer(QObject* parent = nullptr);
    ~PapyrusLanguageServer() override;

    // Builds an LSP Content-Length framed JSON-RPC message. Exposed for
    // testing the transport.
    static QByteArray frameMessage(const QJsonObject& body);

    // Parses a raw Content-Length framed message, returning the message
    // bodies. Incomplete or trailing data is left for the next call via
    // the 'buffer' accumulator.
    static QList<QJsonObject> parseMessages(QByteArray& buffer);

    // Builds a JSON-RPC request/notification body.
    static QJsonObject request(const QString& method, const QJsonValue& params,
                               int id);
    static QJsonObject notification(const QString& method,
                                    const QJsonValue& params);

    // Launches the server executable. Returns false if it cannot start.
    bool start(const QString& serverPath, const QStringList& args = {});

    // Sends the initialize request (returns the request id) and waits for
    // the response via the readyRead signal flow.
    void initialize();

    // Notifies the server that a document was opened / changed.
    void didOpen(const QString& uri, const QString& languageId,
                 const QString& text);
    void didChange(const QString& uri, const QString& text);

    // Requests hover info at a position. Returns the request id.
    int hover(const QString& uri, int line, int character);

    // Requests completion at a position. Returns the request id.
    int completion(const QString& uri, int line, int character);

    bool isRunning() const;

signals:
    // Emitted with the parsed response body for a request id.
    void responseReceived(int id, const QJsonObject& body);
    // Emitted for server push notifications (publishDiagnostics etc).
    void notificationReceived(const QString& method, const QJsonObject& params);
    void serverStopped(int exitCode);

private slots:
    void onReadyRead();
    void onFinished(int exitCode, QProcess::ExitStatus status);

private:
    int nextId();
    void send(const QJsonObject& body);

    QProcess* m_process;
    QByteArray m_buffer;
    int m_nextId;
    QHash<int, QString> m_pendingMethods;  // id -> method (for diagnostics)
};

#endif // PAPYRUSLANGUAGESERVER_H
