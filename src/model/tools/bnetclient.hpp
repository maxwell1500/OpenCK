#ifndef BNETCLIENT_H
#define BNETCLIENT_H

#include <QString>
#include <QByteArray>
#include <QUrl>
#include <functional>

class QNetworkAccessManager;

// BnetClient models the Bethesda.net mods API that the real Creation Kit's
// plugin upload flow uses (login / logout / upload), following the wire
// format reverse-engineered from the CK BNet logs (Morrowind project).
//
// Request builders and response parsers are pure static functions so they
// are fully unit-testable offline; the transport hook lets a caller drive
// the real HTTPS requests (a QNetworkAccessManager POST by default).
class BnetClient
{
public:
    struct Credentials
    {
        QString email;
        QString password;
    };

    struct LoginResult
    {
        bool ok = false;
        QString token;
        QString userId;
        QString displayName;
        QString error;
    };

    // Upload payload for a plugin.
    struct Upload
    {
        QString modId;           // empty => create a new mod first
        QString game = QStringLiteral("Starfield");
        QString title;
        QString summary;
        QString description;
        QString version = QStringLiteral("1.0");
        QString pluginPath;      // name of the .esm/.esp file being attached
        QByteArray fileData;     // the plugin bytes
    };

    struct UploadResult
    {
        bool ok = false;
        QString modId;
        QString fileId;
        QString error;
    };

    // Transport hook: performs a POST to 'url' with 'body'/'contentType' and
    // optional bearer token; returns the server reply body.
    using Transport = std::function<QByteArray(
        const QUrl& url, const QByteArray& body, const QByteArray& contentType,
        const QString& bearerToken)>;

    BnetClient();
    ~BnetClient();

    void setTransport(Transport transport);
    Transport transport() const { return m_transport; }

    // --- orchestration ---------------------------------------------------
    LoginResult login(const Credentials& creds);
    bool logout(const QString& token);
    UploadResult upload(const Upload& upload);

    // --- pure wire-format helpers (unit-testable offline) ---------------
    static QUrl loginUrl();
    static QUrl logoutUrl();
    static QUrl modsUrl(const QString& modId = QString());
    static QUrl filesUrl(const QString& modId);

    static QByteArray loginBody(const Credentials& creds);
    static QByteArray logoutBody(const QString& token);
    static QByteArray createModBody(const Upload& upload);
    static QByteArray uploadBody(const Upload& upload);

    static LoginResult parseLogin(const QByteArray& body);
    static UploadResult parseUpload(const QByteArray& body);

    static QByteArray boundary();

private:
    static QByteArray makeMultipart(const Upload& upload, const QByteArray& bnd);
    static QString extractError(const QByteArray& body);

    Transport m_transport;
    QNetworkAccessManager* m_net;
};

#endif // BNETCLIENT_H
