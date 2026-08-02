#include "bnetclient.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>

namespace
{
const QByteArray kJsonType = "application/json";
const QByteArray kOctetType = "application/octet-stream";
}

BnetClient::BnetClient()
    : m_net(new QNetworkAccessManager)
{
    m_transport = [this](const QUrl& url, const QByteArray& body,
                         const QByteArray& contentType, const QString& bearerToken) {
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
        if (!bearerToken.isEmpty())
            request.setRawHeader("Authorization", ("Bearer " + bearerToken).toUtf8());
        QNetworkReply* reply = m_net->post(request, body);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        const QByteArray data = reply->readAll();
        reply->deleteLater();
        return data;
    };
}

BnetClient::~BnetClient()
{
    delete m_net;
}

void BnetClient::setTransport(Transport transport)
{
    m_transport = std::move(transport);
}

QUrl BnetClient::loginUrl()
{
    return QUrl(QStringLiteral("https://login.bethesda.net/api/authenticate"));
}

QUrl BnetClient::logoutUrl()
{
    return QUrl(QStringLiteral("https://login.bethesda.net/api/logout"));
}

QUrl BnetClient::modsUrl(const QString& modId)
{
    QUrl url(QStringLiteral("https://mods.bethesda.net/api/mods"));
    if (!modId.isEmpty())
        url.setPath(url.path() + QStringLiteral("/") + modId);
    return url;
}

QUrl BnetClient::filesUrl(const QString& modId)
{
    QUrl url(QStringLiteral("https://mods.bethesda.net/api/mods"));
    url.setPath(url.path() + QStringLiteral("/") + modId + QStringLiteral("/files"));
    return url;
}

QByteArray BnetClient::loginBody(const Credentials& creds)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("email"), creds.email);
    obj.insert(QStringLiteral("password"), creds.password);
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

QByteArray BnetClient::logoutBody(const QString& token)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("token"), token);
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

QByteArray BnetClient::createModBody(const Upload& upload)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("game_id"), upload.game);
    obj.insert(QStringLiteral("title"), upload.title);
    obj.insert(QStringLiteral("summary"), upload.summary);
    obj.insert(QStringLiteral("description"), upload.description);
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

QByteArray BnetClient::boundary()
{
    return "----openck-" + QByteArray::number(
        QRandomGenerator::global()->bounded(1000000000));
}

QByteArray BnetClient::makeMultipart(const Upload& upload, const QByteArray& bnd)
{
    QByteArray body;
    auto field = [&](const QByteArray& name, const QByteArray& value) {
        body += "--" + bnd + "\r\n";
        body += "Content-Disposition: form-data; name=\"" + name + "\"\r\n\r\n";
        body += value + "\r\n";
    };
    field("game_id", upload.game.toUtf8());
    field("title", upload.title.toUtf8());
    field("summary", upload.summary.toUtf8());
    field("description", upload.description.toUtf8());
    field("version", upload.version.toUtf8());

    body += "--" + bnd + "\r\n";
    body += "Content-Disposition: form-data; name=\"file\"; filename=\""
        + upload.pluginPath.toUtf8() + "\"\r\n";
    body += "Content-Type: " + kOctetType + "\r\n\r\n";
    body += upload.fileData;
    body += "\r\n--" + bnd + "--\r\n";
    return body;
}

QByteArray BnetClient::uploadBody(const Upload& upload)
{
    return makeMultipart(upload, boundary());
}

QString BnetClient::extractError(const QByteArray& body)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return QString::fromUtf8(body).left(200);
    const QJsonObject obj = doc.object();
    for (const char* key : { "error", "message", "detail" })
    {
        if (obj.value(QLatin1String(key)).isString())
            return obj.value(QLatin1String(key)).toString();
    }
    return QString();
}

BnetClient::LoginResult BnetClient::parseLogin(const QByteArray& body)
{
    LoginResult result;
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        result.error = QStringLiteral("Response was not valid JSON");
        return result;
    }
    QJsonObject obj = doc.object();
    if (obj.contains(QStringLiteral("data")) && obj.value(QStringLiteral("data")).isObject())
        obj = obj.value(QStringLiteral("data")).toObject();

    result.token = obj.value(QStringLiteral("token")).toString();
    result.userId = obj.value(QStringLiteral("id")).toString();
    result.displayName = obj.value(QStringLiteral("displayName")).toString();
    if (result.displayName.isEmpty())
        result.displayName = obj.value(QStringLiteral("username")).toString();
    if (!result.token.isEmpty())
    {
        result.ok = true;
    }
    else
    {
        result.error = extractError(body);
        if (result.error.isEmpty())
            result.error = QStringLiteral("Login failed");
    }
    return result;
}

BnetClient::UploadResult BnetClient::parseUpload(const QByteArray& body)
{
    UploadResult result;
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        result.error = QStringLiteral("Response was not valid JSON");
        return result;
    }
    QJsonObject obj = doc.object();
    if (obj.contains(QStringLiteral("data")) && obj.value(QStringLiteral("data")).isObject())
        obj = obj.value(QStringLiteral("data")).toObject();

    result.modId = obj.value(QStringLiteral("mod_id")).toString();
    if (result.modId.isEmpty())
        result.modId = obj.value(QStringLiteral("modId")).toString();
    result.fileId = obj.value(QStringLiteral("file_id")).toString();
    if (result.fileId.isEmpty())
        result.fileId = obj.value(QStringLiteral("fileId")).toString();
    if (result.fileId.isEmpty() && obj.value(QStringLiteral("id")).isString())
        result.fileId = obj.value(QStringLiteral("id")).toString();

    if (!result.fileId.isEmpty())
        result.ok = true;
    else
    {
        result.error = extractError(body);
        if (result.error.isEmpty())
            result.error = QStringLiteral("Upload failed");
    }
    return result;
}

BnetClient::LoginResult BnetClient::login(const Credentials& creds)
{
    if (creds.email.isEmpty() || creds.password.isEmpty())
    {
        LoginResult result;
        result.error = QStringLiteral("Email and password are required");
        return result;
    }
    const QByteArray body = m_transport(loginUrl(), loginBody(creds), kJsonType, QString());
    return parseLogin(body);
}

bool BnetClient::logout(const QString& token)
{
    m_transport(logoutUrl(), logoutBody(token), kJsonType, token);
    return true;
}

BnetClient::UploadResult BnetClient::upload(const Upload& upload)
{
    UploadResult result;
    if (upload.fileData.isEmpty() && upload.pluginPath.isEmpty())
    {
        result.error = QStringLiteral("Nothing to upload");
        return result;
    }
    if (upload.title.isEmpty())
    {
        result.error = QStringLiteral("A mod title is required");
        return result;
    }

    QString modId = upload.modId;
    if (modId.isEmpty())
    {
        const QByteArray createReply = m_transport(
            modsUrl(), createModBody(upload), kJsonType, QString());
        const UploadResult created = parseUpload(createReply);
        if (created.modId.isEmpty())
        {
            result.error = created.error.isEmpty()
                ? QStringLiteral("Could not create the mod")
                : created.error;
            return result;
        }
        modId = created.modId;
    }

    const QByteArray bnd = boundary();
    const QByteArray reply = m_transport(
        filesUrl(modId), makeMultipart(upload, bnd),
        "multipart/form-data; boundary=" + bnd, QString());
    result = parseUpload(reply);
    if (result.ok && result.modId.isEmpty())
        result.modId = modId;
    return result;
}
