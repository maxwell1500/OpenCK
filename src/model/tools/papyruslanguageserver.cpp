#include "papyruslanguageserver.hpp"

#include <QJsonDocument>
#include <QJsonArray>

#include "libs/files/log/logger.hpp"

PapyrusLanguageServer::PapyrusLanguageServer(QObject* parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_nextId(0)
{
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &PapyrusLanguageServer::onReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PapyrusLanguageServer::onFinished);
}

PapyrusLanguageServer::~PapyrusLanguageServer()
{
    if (m_process->state() != QProcess::NotRunning)
    {
        m_process->terminate();
        m_process->waitForFinished(500);
    }
}

QByteArray PapyrusLanguageServer::frameMessage(const QJsonObject& body)
{
    const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
    QByteArray header = "Content-Length: " + QByteArray::number(payload.size()) + "\r\n\r\n";
    return header + payload;
}

QList<QJsonObject> PapyrusLanguageServer::parseMessages(QByteArray& buffer)
{
    QList<QJsonObject> messages;
    while (true)
    {
        const int headerEnd = buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0)
            break;

        const QByteArray header = buffer.left(headerEnd);
        int length = -1;
        for (const QByteArray& line : header.split('\n'))
        {
            const QByteArray trimmed = line.trimmed();
            if (trimmed.startsWith("Content-Length:"))
                length = trimmed.mid(15).trimmed().toInt();
        }
        if (length < 0)
        {
            // Malformed header; discard up to the next boundary to avoid a
            // tight loop.
            buffer.remove(0, headerEnd + 4);
            continue;
        }

        const int payloadStart = headerEnd + 4;
        if (buffer.size() < payloadStart + length)
            break;  // wait for the full payload

        const QJsonDocument doc = QJsonDocument::fromJson(
            buffer.mid(payloadStart, length));
        if (doc.isObject())
            messages.append(doc.object());

        buffer.remove(0, payloadStart + length);
    }
    return messages;
}

QJsonObject PapyrusLanguageServer::request(const QString& method,
                                           const QJsonValue& params, int id)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    obj.insert(QStringLiteral("id"), id);
    obj.insert(QStringLiteral("method"), method);
    obj.insert(QStringLiteral("params"), params);
    return obj;
}

QJsonObject PapyrusLanguageServer::notification(const QString& method,
                                                const QJsonValue& params)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    obj.insert(QStringLiteral("method"), method);
    obj.insert(QStringLiteral("params"), params);
    return obj;
}

bool PapyrusLanguageServer::start(const QString& serverPath,
                                  const QStringList& args)
{
    if (m_process->state() != QProcess::NotRunning)
        return false;

    m_process->start(serverPath, args);
    if (!m_process->waitForStarted(2000))
    {
        LOG_WARNING(QString("PapyrusLSP: failed to start %1").arg(serverPath));
        return false;
    }
    LOG_INFO(QString("PapyrusLSP: started %1").arg(serverPath));
    return true;
}

void PapyrusLanguageServer::initialize()
{
    QJsonObject params;
    QJsonObject clientInfo;
    clientInfo.insert(QStringLiteral("name"), QStringLiteral("OpenCK"));
    clientInfo.insert(QStringLiteral("version"), QStringLiteral("1.0"));
    params.insert(QStringLiteral("clientInfo"), clientInfo);
    params.insert(QStringLiteral("capabilities"), QJsonObject());

    const int id = nextId();
    m_pendingMethods.insert(id, QStringLiteral("initialize"));
    send(request(QStringLiteral("initialize"), params, id));
}

void PapyrusLanguageServer::didOpen(const QString& uri, const QString& languageId,
                                    const QString& text)
{
    QJsonObject textDoc;
    textDoc.insert(QStringLiteral("uri"), uri);
    textDoc.insert(QStringLiteral("languageId"), languageId);
    textDoc.insert(QStringLiteral("version"), 1);
    textDoc.insert(QStringLiteral("text"), text);

    QJsonObject params;
    params.insert(QStringLiteral("textDocument"), textDoc);
    send(notification(QStringLiteral("textDocument/didOpen"), params));
}

void PapyrusLanguageServer::didChange(const QString& uri, const QString& text)
{
    QJsonObject change;
    change.insert(QStringLiteral("text"), text);

    QJsonArray changes;
    changes.append(change);

    QJsonObject textDoc;
    textDoc.insert(QStringLiteral("uri"), uri);
    textDoc.insert(QStringLiteral("version"), 2);

    QJsonObject params;
    params.insert(QStringLiteral("textDocument"), textDoc);
    params.insert(QStringLiteral("contentChanges"), changes);
    send(notification(QStringLiteral("textDocument/didChange"), params));
}

int PapyrusLanguageServer::hover(const QString& uri, int line, int character)
{
    QJsonObject position;
    position.insert(QStringLiteral("line"), line);
    position.insert(QStringLiteral("character"), character);

    QJsonObject textDoc;
    textDoc.insert(QStringLiteral("uri"), uri);

    QJsonObject params;
    params.insert(QStringLiteral("textDocument"), textDoc);
    params.insert(QStringLiteral("position"), position);

    const int id = nextId();
    m_pendingMethods.insert(id, QStringLiteral("textDocument/hover"));
    send(request(QStringLiteral("textDocument/hover"), params, id));
    return id;
}

int PapyrusLanguageServer::completion(const QString& uri, int line, int character)
{
    QJsonObject position;
    position.insert(QStringLiteral("line"), line);
    position.insert(QStringLiteral("character"), character);

    QJsonObject textDoc;
    textDoc.insert(QStringLiteral("uri"), uri);

    QJsonObject params;
    params.insert(QStringLiteral("textDocument"), textDoc);
    params.insert(QStringLiteral("position"), position);

    const int id = nextId();
    m_pendingMethods.insert(id, QStringLiteral("textDocument/completion"));
    send(request(QStringLiteral("textDocument/completion"), params, id));
    return id;
}

bool PapyrusLanguageServer::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

void PapyrusLanguageServer::onReadyRead()
{
    m_buffer.append(m_process->readAllStandardOutput());

    const QList<QJsonObject> messages = parseMessages(m_buffer);
    for (const QJsonObject& msg : messages)
    {
        if (msg.contains(QStringLiteral("id")) && msg.contains(QStringLiteral("result")))
        {
            const int id = msg.value(QStringLiteral("id")).toInt(-1);
            m_pendingMethods.remove(id);
            emit responseReceived(id, msg);
        }
        else if (msg.contains(QStringLiteral("method")))
        {
            const QString method = msg.value(QStringLiteral("method")).toString();
            emit notificationReceived(method,
                msg.value(QStringLiteral("params")).toObject());
        }
    }
}

void PapyrusLanguageServer::onFinished(int exitCode, QProcess::ExitStatus)
{
    LOG_INFO(QString("PapyrusLSP: server exited with code %1").arg(exitCode));
    emit serverStopped(exitCode);
}

int PapyrusLanguageServer::nextId()
{
    return ++m_nextId;
}

void PapyrusLanguageServer::send(const QJsonObject& body)
{
    m_process->write(frameMessage(body));
}
