#include <QTest>
#include <QTemporaryFile>

#include "../../libs/files/audio/fuzparser.hpp"
#include "../../libs/files/log/logger.hpp"

class TestFuzParser : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testParseBasic();
    void testParseLipAndAudio();
    void testParseRealForm();
    void testWrongMagic();
    void testLoadFile();
};

void TestFuzParser::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_fuzparser_log.txt"));
}

static QByteArray makeFuz(const QByteArray& lip, const QByteArray& audio)
{
    QByteArray out("FUZE", 4);
    if (!lip.isEmpty())
    {
        out.append("LIPF", 4);
        out.append(static_cast<char>(lip.size() & 0xFF));
        out.append(static_cast<char>((lip.size() >> 8) & 0xFF));
        out.append(static_cast<char>((lip.size() >> 16) & 0xFF));
        out.append(static_cast<char>((lip.size() >> 24) & 0xFF));
        out.append(lip);
    }
    if (!audio.isEmpty())
    {
        out.append("XWAV", 4);
        out.append(static_cast<char>(audio.size() & 0xFF));
        out.append(static_cast<char>((audio.size() >> 8) & 0xFF));
        out.append(static_cast<char>((audio.size() >> 16) & 0xFF));
        out.append(static_cast<char>((audio.size() >> 24) & 0xFF));
        out.append(audio);
    }
    return out;
}

void TestFuzParser::testParseBasic()
{
    const QByteArray lip = "{\"cues\":[]}";
    const QByteArray audio = "\x52\x49\x46\x46 audio data";
    const QByteArray bytes = makeFuz(lip, audio);

    FuzParser out;
    QVERIFY(FuzParser::parse(bytes, out));
    QVERIFY(out.hasLip());
    QVERIFY(out.hasAudio());
    QCOMPARE(out.lipData, lip);
    QCOMPARE(out.audioData, audio);
    QCOMPARE(out.audioFourCC, QStringLiteral("XWAV"));
}

void TestFuzParser::testParseLipAndAudio()
{
    // Lip-only fuz
    FuzParser lipOnly;
    QVERIFY(FuzParser::parse(makeFuz("cue1", QByteArray()), lipOnly));
    QVERIFY(lipOnly.hasLip());
    QVERIFY(!lipOnly.hasAudio());

    // Audio-only fuz
    FuzParser audioOnly;
    QVERIFY(FuzParser::parse(makeFuz(QByteArray(), "RIFF...."), audioOnly));
    QVERIFY(!audioOnly.hasLip());
    QVERIFY(audioOnly.hasAudio());
}

void TestFuzParser::testParseRealForm()
{
    // Real .fuz layout: "FUZE" + version(4) + lipSize(4) + raw lip + RIFF audio.
    const QByteArray lip = "{\"cues\":[{\"t\":0,\"p\":\"a\"}]}";
    const QByteArray riffBody = QByteArray("XWMA") +
        "\x10\x00\x00\x00" +          // fmt chunk header
        "fmt\x20\x00\x00\x00" +       // fmt chunk name+size (fabricated)
        "\x00\x00\x00\x00";           // fmt payload
    const QByteArray audio = "RIFF" + QByteArray(4, '\0') + riffBody;

    QByteArray fuz("FUZE", 4);
    fuz.append("\x01\x00\x00\x00", 4);                    // version = 1
    fuz.append(static_cast<char>(lip.size() & 0xFF));
    fuz.append(static_cast<char>((lip.size() >> 8) & 0xFF));
    fuz.append(static_cast<char>((lip.size() >> 16) & 0xFF));
    fuz.append(static_cast<char>((lip.size() >> 24) & 0xFF));
    fuz.append(lip);
    fuz.append(audio);

    FuzParser out;
    QVERIFY(FuzParser::parse(fuz, out));
    QCOMPARE(out.lipData, lip);
    QCOMPARE(out.audioData, audio);
    QCOMPARE(out.audioFourCC, QStringLiteral("XWMA"));

    // Lip-only real form (empty audio region must still be valid).
    QByteArray lipOnlyFuz("FUZE", 4);
    lipOnlyFuz.append("\x01\x00\x00\x00", 4);
    lipOnlyFuz.append(static_cast<char>(lip.size() & 0xFF));
    lipOnlyFuz.append(static_cast<char>((lip.size() >> 8) & 0xFF));
    lipOnlyFuz.append(static_cast<char>((lip.size() >> 16) & 0xFF));
    lipOnlyFuz.append(static_cast<char>((lip.size() >> 24) & 0xFF));
    lipOnlyFuz.append(lip);
    lipOnlyFuz.append(QByteArray("RIFF") + QByteArray(8, '\0'));
    FuzParser lipOut;
    QVERIFY(FuzParser::parse(lipOnlyFuz, lipOut));
    QCOMPARE(lipOut.lipData, lip);
}

void TestFuzParser::testWrongMagic()
{
    FuzParser out;
    QVERIFY(!FuzParser::parse(QByteArray("NOTAFUZE"), out));
    QVERIFY(!FuzParser::parse(QByteArray(), out));
    QVERIFY(!FuzParser::parse(QByteArray("FUZ"), out)); // too short
}

void TestFuzParser::testLoadFile()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write(makeFuz("lipdata", "audiodata"));
    file.close();

    FuzParser out;
    QVERIFY(FuzParser::loadFile(file.fileName(), out));
    QCOMPARE(out.lipData, QByteArray("lipdata"));
    QCOMPARE(out.audioData, QByteArray("audiodata"));

    FuzParser missing;
    QVERIFY(!FuzParser::loadFile(QStringLiteral("Z:/missing.fuz"), missing));
}

QTEST_MAIN(TestFuzParser)
#include "test_fuzparser.moc"
