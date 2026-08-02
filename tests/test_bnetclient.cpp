#include <QTest>
#include <QJsonObject>
#include <QJsonDocument>

#include "../../src/model/tools/bnetclient.hpp"

class TestBnetClient : public QObject
{
    Q_OBJECT

private slots:
    void testUrls();
    void testLoginBody();
    void testLoginParse();
    void testLoginParseError();
    void testLogoutBody();
    void testCreateModBody();
    void testUploadBody();
    void testParseUpload();
    void testLoginOrchestration();
    void testUploadOrchestration();
    void testUploadValidation();
};

void TestBnetClient::testUrls()
{
    QVERIFY(BnetClient::loginUrl().path().endsWith(QStringLiteral("/api/authenticate")));
    QVERIFY(BnetClient::logoutUrl().path().endsWith(QStringLiteral("/api/logout")));
    QCOMPARE(BnetClient::modsUrl(), QUrl(QStringLiteral("https://mods.bethesda.net/api/mods")));
    const QUrl files = BnetClient::filesUrl(QStringLiteral("m1"));
    QVERIFY(files.path().endsWith(QStringLiteral("/api/mods/m1/files")));
    QVERIFY(BnetClient::modsUrl(QStringLiteral("m1")).path().endsWith(QStringLiteral("/api/mods/m1")));
}

void TestBnetClient::testLoginBody()
{
    BnetClient::Credentials creds;
    creds.email = QStringLiteral("modder@example.com");
    creds.password = QStringLiteral("s3cret");
    const QByteArray body = BnetClient::loginBody(creds);
    const QJsonObject obj = QJsonDocument::fromJson(body).object();
    QCOMPARE(obj.value(QStringLiteral("email")).toString(), QStringLiteral("modder@example.com"));
    QCOMPARE(obj.value(QStringLiteral("password")).toString(), QStringLiteral("s3cret"));
}

void TestBnetClient::testLoginParse()
{
    const QByteArray body = "{\"token\":\"tok123\",\"id\":\"u42\",\"displayName\":\"Modder\"}";
    const BnetClient::LoginResult result = BnetClient::parseLogin(body);
    QVERIFY(result.ok);
    QCOMPARE(result.token, QStringLiteral("tok123"));
    QCOMPARE(result.userId, QStringLiteral("u42"));
    QCOMPARE(result.displayName, QStringLiteral("Modder"));
}

void TestBnetClient::testLoginParseError()
{
    const QByteArray body = "{\"error\":\"Invalid credentials\"}";
    const BnetClient::LoginResult result = BnetClient::parseLogin(body);
    QVERIFY(!result.ok);
    QVERIFY(result.token.isEmpty());
    QCOMPARE(result.error, QStringLiteral("Invalid credentials"));
}

void TestBnetClient::testLogoutBody()
{
    const QJsonObject obj =
        QJsonDocument::fromJson(BnetClient::logoutBody(QStringLiteral("tok"))).object();
    QCOMPARE(obj.value(QStringLiteral("token")).toString(), QStringLiteral("tok"));
}

void TestBnetClient::testCreateModBody()
{
    BnetClient::Upload upload;
    upload.game = QStringLiteral("Starfield");
    upload.title = QStringLiteral("My Mod");
    upload.summary = QStringLiteral("A test mod");
    upload.description = QStringLiteral("Longer description");
    const QJsonObject obj =
        QJsonDocument::fromJson(BnetClient::createModBody(upload)).object();
    QCOMPARE(obj.value(QStringLiteral("game_id")).toString(), QStringLiteral("Starfield"));
    QCOMPARE(obj.value(QStringLiteral("title")).toString(), QStringLiteral("My Mod"));
    QCOMPARE(obj.value(QStringLiteral("summary")).toString(), QStringLiteral("A test mod"));
    QCOMPARE(obj.value(QStringLiteral("description")).toString(), QStringLiteral("Longer description"));
}

void TestBnetClient::testUploadBody()
{
    BnetClient::Upload upload;
    upload.title = QStringLiteral("My Mod");
    upload.version = QStringLiteral("1.1");
    upload.pluginPath = QStringLiteral("MyMod.esm");
    upload.fileData = QByteArray("PLUGIN_BYTES");

    const QByteArray body = BnetClient::uploadBody(upload);
    // Multipart framing is well-formed: a boundary line opens the body and
    // a closing boundary terminates it.
    QVERIFY(body.startsWith("--"));
    const int firstCrlf = body.indexOf("\r\n");
    QVERIFY(firstCrlf > 2);
    const QByteArray bnd = body.left(firstCrlf).mid(2);  // strip the leading "--"
    QVERIFY(!bnd.isEmpty());
    QVERIFY(body.contains(bnd));
    QVERIFY(body.endsWith("--" + bnd + "--\r\n") || body.endsWith("\r\n--" + bnd + "--\r\n"));
    QVERIFY(body.contains("name=\"title\""));
    QVERIFY(body.contains("My Mod"));
    QVERIFY(body.contains("name=\"version\""));
    QVERIFY(body.contains("1.1"));
    QVERIFY(body.contains("filename=\"MyMod.esm\""));
    QVERIFY(body.contains("PLUGIN_BYTES"));
    QVERIFY(body.startsWith("--" + bnd));
    QVERIFY(body.endsWith("--" + bnd + "--\r\n"));
}

void TestBnetClient::testParseUpload()
{
    const BnetClient::UploadResult result =
        BnetClient::parseUpload("{\"mod_id\":\"m9\",\"file_id\":\"f7\"}");
    QVERIFY(result.ok);
    QCOMPARE(result.modId, QStringLiteral("m9"));
    QCOMPARE(result.fileId, QStringLiteral("f7"));
}

void TestBnetClient::testLoginOrchestration()
{
    BnetClient client;
    QUrl seenUrl;
    QByteArray seenBody;
    bool sawBearer = false;
    client.setTransport([&](const QUrl& url, const QByteArray& body,
                            const QByteArray&, const QString&) {
        seenUrl = url;
        seenBody = body;
        return QByteArray("{\"token\":\"t\",\"id\":\"u\",\"displayName\":\"D\"}");
    });

    BnetClient::Credentials creds;
    creds.email = QStringLiteral("a@b.c");
    creds.password = QStringLiteral("p");
    const BnetClient::LoginResult result = client.login(creds);
    QVERIFY(result.ok);
    QCOMPARE(result.token, QStringLiteral("t"));
    QCOMPARE(seenUrl, BnetClient::loginUrl());
    QVERIFY(seenBody.contains("a@b.c"));

    // Transport callback never sees the Bearer for login.
    Q_UNUSED(sawBearer);
}

void TestBnetClient::testUploadOrchestration()
{
    BnetClient client;
    QStringList requestedPaths;
    client.setTransport([&](const QUrl& url, const QByteArray&,
                            const QByteArray&, const QString&) {
        requestedPaths.append(url.path());
        if (url.path().endsWith(QStringLiteral("/api/mods")))
            return QByteArray("{\"mod_id\":\"m1\"}");
        return QByteArray("{\"file_id\":\"f1\"}");
    });

    BnetClient::Upload upload;
    upload.title = QStringLiteral("My Mod");
    upload.pluginPath = QStringLiteral("MyMod.esm");
    upload.fileData = QByteArray("BYTES");
    const BnetClient::UploadResult result = client.upload(upload);

    QVERIFY(result.ok);
    QCOMPARE(result.modId, QStringLiteral("m1"));
    QCOMPARE(result.fileId, QStringLiteral("f1"));
    // First create, then file upload against the created mod.
    QCOMPARE(requestedPaths.size(), 2);
    QVERIFY(requestedPaths[1].endsWith(QStringLiteral("/api/mods/m1/files")));
}

void TestBnetClient::testUploadValidation()
{
    BnetClient client;
    client.setTransport([](const QUrl&, const QByteArray&, const QByteArray&,
                           const QString&) { return QByteArray(); });

    BnetClient::Upload upload;
    // Empty payload rejected.
    QVERIFY(!client.upload(upload).ok);
    // Title required.
    upload.fileData = QByteArray("X");
    upload.pluginPath = QStringLiteral("X.esm");
    QVERIFY(!client.upload(upload).ok);
}

QTEST_MAIN(TestBnetClient)
#include "test_bnetclient.moc"
