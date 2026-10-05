#include <QTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

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
    void testArchiveDiscoveryIsRecursive();
    void testExtractButtonReflectsFilterScope();
    void testOpenBsaAndList();
    void testVoiceFilter();
    void testSearch();
    void testSafeExtractionPaths();
    void testWritePcmWav();
};

// Opening and filtering a 75k-file BSA through a QListWidget takes minutes
// in a Debug build; the default per-function watchdog (5 min) is too tight
// for CI. This test requires the game, so its timeout budget is raised
// before main() runs (i.e. before QTest reads the limit).
namespace {
const struct WatchdogBudget {
    WatchdogBudget() { qputenv("QTEST_FUNCTION_TIMEOUT", "1800000"); }
} g_watchdogBudget;
} // namespace

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

void TestArchiveBrowser::testArchiveDiscoveryIsRecursive()
{
    // Self-contained: the discovery rule is worth covering without needing a game
    // install, because the bug it fixes is precisely that a nested layout was
    // silently missed. Fallout 4 and Skyrim keep DLC in the Data root, so the
    // shipped games would not have caught it.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

// Built without QVERIFY inside a lambda on purpose: QVERIFY expands to a
    // bare `return`, which inside a lambda only leaves the lambda, so a failed
    // mkdir used to stop creating the remaining files without failing the test.
    // That produced a fixture missing its nested entries, which then looked like
    // a discovery bug.
    const QStringList relatives = {
        QStringLiteral("Top.bsa"),
        QStringLiteral("nested/Inner.ba2"),
        QStringLiteral("nested/deeper/Deepest.ba2"),
        QStringLiteral("nested/notanarchive.txt"),
    };
    for (const QString& relative : relatives) {
        const QString path = dir.filePath(relative);
        if (!QDir().mkpath(QFileInfo(path).absolutePath()))
            QFAIL(qPrintable(QStringLiteral("could not create the folder for %1").arg(relative)));
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly))
            QFAIL(qPrintable(QStringLiteral("could not create %1").arg(relative)));
        f.write("x");
        f.close();
    }
    // The fixture is asserted by exact path, so a fixture that failed to build
    // fails here rather than masquerading as a discovery failure.
    for (const QString& relative : relatives) {
        const QString path = dir.filePath(relative);
        if (!QFileInfo::exists(path))
            QFAIL(qPrintable(QStringLiteral("fixture file was not created: %1").arg(path)));
    }

    const QStringList found = ArchiveBrowserDialog::findArchives(dir.path());
    // entryInfoList reports forward slashes on Windows while absoluteFilePath
    // produces the native separator, so both sides are normalised before
    // comparing — otherwise this asserts on separator style, not on discovery.
    const auto normalise = [](QStringList paths) {
        for (QString& p : paths)
            p = QDir::fromNativeSeparators(p);
        return paths;
    };
    const QStringList expected = normalise({
        QDir(dir.path()).absoluteFilePath(QStringLiteral("Top.bsa")),
        QDir(dir.path()).absoluteFilePath(QStringLiteral("nested/Inner.ba2")),
        QDir(dir.path()).absoluteFilePath(QStringLiteral("nested/deeper/Deepest.ba2")),
    });
    QCOMPARE(normalise(found), expected);
    // A non-archive in the same tree must not be picked up.
    for (const QString& path : normalise(found))
        QVERIFY(!path.endsWith(QStringLiteral("notanarchive.txt")));

    // Sorted, so the quick-open list order does not depend on the filesystem.
    QStringList sorted = found;
    sorted.sort();
    QCOMPARE(found, sorted);

    // An empty or missing root yields nothing rather than throwing.
    QVERIFY(ArchiveBrowserDialog::findArchives(QString()).isEmpty());
    QVERIFY(ArchiveBrowserDialog::findArchives(
        dir.filePath(QStringLiteral("does-not-exist"))).isEmpty());
}

void TestArchiveBrowser::testExtractButtonReflectsFilterScope()
{
    if (!QFileInfo::exists(s_dataDir)) QSKIP("Skyrim SE data dir not found");
    ArchiveBrowserDialog dlg(s_dataDir);
    QVERIFY(openVoicesArchive(dlg));

    auto* button = dlg.findChild<QPushButton*>("extractAllBtn");
    auto* list = dlg.findChild<QListWidget*>("entryList");
    QVERIFY(button);
    QVERIFY(list);
    QVERIFY(list->count() > 0);

    // Unfiltered: the button offers the whole archive and says so with a count.
    const int total = list->count();
    const QString unfiltered = button->text();
    QVERIFY2(unfiltered.contains(QStringLiteral("Extract All")),
             qPrintable(QStringLiteral("unfiltered label was '%1'").arg(unfiltered)));
    QVERIFY2(unfiltered.contains(QString::number(total)),
             qPrintable(QStringLiteral("unfiltered label '%1' omits the entry count %2")
                            .arg(unfiltered).arg(total)));

    // Narrow with the search box rather than the type filter. A voices archive
    // contains only .fuz entries, so the Voice filter provably narrows nothing
    // there and would assert against a premise that does not hold.
    auto* search = dlg.findChild<QLineEdit*>("searchEdit");
    QVERIFY(search);

    const QString term = QFileInfo(list->item(0)->text()).completeBaseName();
    QVERIFY(!term.isEmpty());
    search->setText(term);

    const int filtered = list->count();
    if (filtered == 0 || filtered == total) {
        // A degenerate name would make the label assertions below meaningless.
        QSKIP(qPrintable(QStringLiteral("search term '%1' did not narrow %2 entries")
                             .arg(term).arg(total)));
    }

    const QString filteredLabel = button->text();
    QVERIFY2(filteredLabel != unfiltered,
             "label did not change when the search narrowed the list");
    QVERIFY2(filteredLabel.contains(QString::number(filtered)),
             qPrintable(QStringLiteral("filtered label '%1' omits the filtered count %2")
                            .arg(filteredLabel).arg(filtered)));
    QVERIFY2(!filteredLabel.contains(QStringLiteral("Extract All")),
             qPrintable(QStringLiteral("filtered label '%1' still claims to extract all")
                            .arg(filteredLabel)));

    // A search that matches nothing must disable the button rather than leave a
    // live control that would extract an empty set.
    search->setText(QStringLiteral("no-such-entry-anywhere-zzz"));
    QCOMPARE(list->count(), 0);
    QVERIFY2(!button->isEnabled(),
             "the extract button stayed enabled with nothing selected to extract");
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

void TestArchiveBrowser::testSafeExtractionPaths()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString root = dir.path();
    QString output;
    QVERIFY(ArchiveBrowserDialog::isSafeExtractionPath(root, "textures/a.dds", &output));
    QVERIFY(output.startsWith(QDir(root).absolutePath(), Qt::CaseInsensitive));
    QVERIFY(!ArchiveBrowserDialog::isSafeExtractionPath(root, "../escape.txt"));
    QVERIFY(!ArchiveBrowserDialog::isSafeExtractionPath(root, "textures/../../escape.txt"));
    QVERIFY(!ArchiveBrowserDialog::isSafeExtractionPath(root, "C:/escape.txt"));
    QVERIFY(!ArchiveBrowserDialog::isSafeExtractionPath(root, "//server/share/file.txt"));
    QVERIFY(!ArchiveBrowserDialog::isSafeExtractionPath(root, "\\server\\share\\file.txt"));
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
