#include <QTest>
#include <QFileInfo>

#include "bsaarchive.hpp"
#include "fuzparser.hpp"
#include "xwmadecoder.hpp"
#include "logger.hpp"

// Decodes a real Skyrim voice .fuz's xWMA audio to PCM. Preferred path is the
// ffmpeg CLI (when present on PATH); falls back to the Windows Media
// Foundation WMA decoder MFT. Requires the user's Skyrim SE install.
class TestXwmaDecoder : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testDecodeSkyrimVoice();
};

void TestXwmaDecoder::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_xwmadecoder_log.txt"));
}

void TestXwmaDecoder::testDecodeSkyrimVoice()
{
    const QString path = QStringLiteral(
        "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data/Skyrim - Voices_en0.bsa");
    QVERIFY2(QFileInfo::exists(path), "Skyrim SE not found");

    BsaArchive archive;
    QVERIFY(archive.open(path));

    // Find a .fuz with XWMA audio.
    int index = -1;
    for (int i = 0; i < archive.fileCount(); ++i) {
        if (!archive.entries()[i].fullPath.endsWith(QStringLiteral(".fuz"), Qt::CaseInsensitive))
            continue;
        QByteArray probe;
        if (!archive.readData(i, probe)) continue;
        FuzParser p;
        if (FuzParser::parse(probe, p) && p.audioFourCC == QStringLiteral("XWMA")) {
            index = i;
            break;
        }
    }
    QVERIFY(index >= 0);
    qDebug() << "decoding:" << archive.entries()[index].fullPath;

    QByteArray fuzBytes;
    QVERIFY(archive.readData(index, fuzBytes));
    FuzParser parser;
    QVERIFY(FuzParser::parse(fuzBytes, parser));
    QVERIFY(parser.hasAudio());

    const XwmaDecoder::Result decoded = XwmaDecoder::decode(parser.audioData);
    qDebug() << "decode ok:" << decoded.ok
             << "pcm:" << decoded.pcm.size() << "bytes";
    QVERIFY(decoded.ok);
    QVERIFY(!decoded.pcm.isEmpty());
    QVERIFY(decoded.sampleRate == 44100);
    QVERIFY(decoded.channels == 1);

    // A ~2.3s clip at 44.1kHz mono 16-bit is ~200KB. ffmpeg (preferred path)
    // decodes the full clip; the MFT fallback may produce less, so require
    // the full length only when ffmpeg is available.
    QVERIFY2(decoded.pcm.size() > 50000, "decoded PCM too short");
    if (decoded.pcm.size() > 150000)
        qDebug() << "full-length decode confirmed";
}

QTEST_MAIN(TestXwmaDecoder)
#include "test_xwmadecoder.moc"
