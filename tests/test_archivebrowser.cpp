#include <QTest>
#include <QTemporaryFile>
#include <QFile>
#include <QFileInfo>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>

#include "view/window/archivebrowserdialog.hpp"
#include "view/window/voicepreview.hpp"
#include "logger.hpp"

// Validates the ArchiveBrowserDialog wiring against the user's Skyrim SE
// install: opening a real BSA, filtering to .fuz entries, search, and the
// PCM->WAV writer used by voice preview. Requires the game; not registered
// with CTest by default.
class TestArchiveBrowser : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testQuickOpenList();
    void testOpenBsaAndList();
    void testVoiceFilter();
    void testSearch();
    void testWritePcmWav();
};

void TestArchiveBrowser::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral(
        "C:/Users/max/AppData/Local/Temp/opencode/test_archivebrowser_log.txt"));
}

namespace {
const QString s_dataDir = QStringLiteral(
    "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data");
const QString s_voiceArchive = QStringLiteral(
    "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data/Skyrim - Voices_en0.bsa");

bool openVoicesArchive(ArchiveBrowserDialog& dlg)
{
    auto* combo = dlg.findChild<QComboBox*>("quickOpen");
    if (!combo) return false;
    const int idx = combo->findData(s_voiceArchive);
    if (idx < 0) return false;
    combo->setCurrentIndex(idx);
    return true;
}
} // namespace

void TestArchiveBrowser::testQuickOpenList()
{
    if (!QFileInfo::exists(s_dataDir)) QSKIP("Skyrim SE data dir not found");
    ArchiveBrowserDialog dlg(s_dataDir);
    auto* combo = dlg.findChild<QComboBox*>("quickOpen");
    QVERIFY(combo);
    QVERIFY(combo->count() > 0);
}

void TestArchiveBrowser::testOpenBsaAndList()
{
    if (!QFileInfo::exists(s_voiceArchive)) QSKIP("Skyrim SE Voices archive not found");
    ArchiveBrowserDialog dlg(s_dataDir);
    QVERIFY(openVoicesArchive(dlg));

    auto* list = dlg.findChild<QListWidget*>("entryList");
    QVERIFY(list);
    QVERIFY(list->count() > 1000);
    QVERIFY(dlg.windowTitle().contains("Voices_en0", Qt::CaseInsensitive));
}

void TestArchiveBrowser::testVoiceFilter()
{
    if (!QFileInfo::exists(s_voiceArchive)) QSKIP("Skyrim SE Voices archive not found");
    ArchiveBrowserDialog dlg(s_dataDir);
    QVERIFY(openVoicesArchive(dlg));

    auto* filter = dlg.findChild<QComboBox*>("filterCombo");
    auto* list = dlg.findChild<QListWidget*>("entryList");
    QVERIFY(filter);
    QVERIFY(list);

    filter->setCurrentIndex(4); // Voice (.fuz)
    QVERIFY(list->count() > 1000);
    qDebug() << "voice-filtered entries:" << list->count();
    for (int i = 0; i < list->count(); ++i)
    {
        const QString text = list->item(i)->text();
        QVERIFY2(text.endsWith(QStringLiteral(".fuz"), Qt::CaseInsensitive),
                 qPrintable(text));
    }
}

void TestArchiveBrowser::testSearch()
{
    if (!QFileInfo::exists(s_voiceArchive)) QSKIP("Skyrim SE Voices archive not found");
    ArchiveBrowserDialog dlg(s_dataDir);
    QVERIFY(openVoicesArchive(dlg));

    auto* search = dlg.findChild<QLineEdit*>("searchEdit");
    auto* list = dlg.findChild<QListWidget*>("entryList");
    QVERIFY(search);
    QVERIFY(list);

    search->setText("femalekhajiit");
    QVERIFY(list->count() > 0);
    for (int i = 0; i < list->count(); ++i)
    {
        const QString text = list->item(i)->text();
        QVERIFY2(text.contains(QStringLiteral("femalekhajiit"), Qt::CaseInsensitive),
                 qPrintable(text));
    }
}

void TestArchiveBrowser::testWritePcmWav()
{
    QByteArray pcm;
    for (int i = 0; i < 1000; ++i)
    {
        const qint16 sample = static_cast<qint16>((i * 37) % 2000 - 1000);
        pcm.append(static_cast<char>(sample & 0xFF));
        pcm.append(static_cast<char>((sample >> 8) & 0xFF));
    }

    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    const QString out = tmp.fileName();
    tmp.close();

    QVERIFY(VoicePreview::writePcmWav(pcm, 44100, 1, out));

    QFile f(out);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QByteArray all = f.readAll();
    f.close();

    QCOMPARE(all.size(), pcm.size() + 44);
    QVERIFY(all.startsWith("RIFF"));
    QCOMPARE(all.mid(8, 4), QByteArray("WAVE"));
    QCOMPARE(all.mid(12, 4), QByteArray("fmt "));
    // audioFormat(20) == 1 (PCM), channels(22) == 1, sampleRate(24) == 44100
    const quint16 audioFormat = static_cast<quint16>(all.at(20))
        | (static_cast<quint16>(all.at(21)) << 8);
    const quint16 channels = static_cast<quint16>(all.at(22))
        | (static_cast<quint16>(all.at(23)) << 8);
    const quint32 sampleRate = static_cast<quint32>(static_cast<quint8>(all.at(24)))
        | (static_cast<quint32>(static_cast<quint8>(all.at(25))) << 8)
        | (static_cast<quint32>(static_cast<quint8>(all.at(26))) << 16)
        | (static_cast<quint32>(static_cast<quint8>(all.at(27))) << 24);
    QCOMPARE(audioFormat, quint16(1));
    QCOMPARE(channels, quint16(1));
    QCOMPARE(sampleRate, quint32(44100));
    QCOMPARE(all.mid(36, 4), QByteArray("data"));
}

QTEST_MAIN(TestArchiveBrowser)
#include "test_archivebrowser.moc"
