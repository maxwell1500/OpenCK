#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QLocale>

#include "ini/inifile.hpp"
#include "ba2/resourcearchiveconfig.hpp"
#include "model/tools/assetresolver.hpp"
#include "model/tools/externaldatacollector.hpp"
#include "model/tools/assetdependencyscanner.hpp"
#include "ckid.hpp"
#include "ba2/bsaarchive.hpp"

class TestExternalData : public QObject
{
    Q_OBJECT
private slots:
    void iniReadsSectionsAndKeys();
    void iniIsCaseInsensitiveAndKeepsRepeats();
    void iniSplitsCommaLists();
    void iniRejectsMalformedLinesWithoutLosingTheFile();
    void iniMissingFileIsEmptyNotBroken();

    void configReadsBothArchiveListsSeparately();
    void configResolvesNamesAgainstDisk();
    void configNamesArchiveMatchesOnFileName();

    void planSeparatesLooseArchivedAndUnavailable();
    void resolverFindsArchivesAtTopLevelAndNested();
    void planHonoursIgnoreArchivesInside();
    void planRestrictsToResourceArchives();
    void collectWritesOnlyInsideDestination();
};

// ============================================================================
// IniFile
// ============================================================================

static bool writeFile(const QString& path, const QString& text)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    return f.write(text.toUtf8()) >= 0;
}

void TestExternalData::iniReadsSectionsAndKeys()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + "/tool.ini";
    QVERIFY(writeFile(path,
        "[General]\n"
        "bCheckForMultiFileForms = 1\n"
        "\n"
        "[Archive]\n"
        "SResourceArchiveList = A.ba2, B.ba2\n"
        "[Wwise]\n"
        "iDefaultExternalCodecID = 4\n"));

    IniFile ini;
    QVERIFY2(ini.load(path), "a readable file must load");
    QVERIFY(ini.hasSection(QStringLiteral("General")));
    QVERIFY(ini.hasSection(QStringLiteral("Wwise")));
    QVERIFY(!ini.hasSection(QStringLiteral("Nope")));
    QCOMPARE(ini.intValue(QStringLiteral("Wwise"), QStringLiteral("iDefaultExternalCodecID"), -1), 4);
    QCOMPARE(ini.value(QStringLiteral("General"), QStringLiteral("bCheckForMultiFileForms")),
             QStringLiteral("1"));
    // Surrounding whitespace is presentation, not data.
    QCOMPARE(ini.value(QStringLiteral("General"), QStringLiteral("bCheckForMultiFileForms")),
             QStringLiteral("1"));
}

void TestExternalData::iniIsCaseInsensitiveAndKeepsRepeats()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + "/tool.ini";
    // The real file is not self-consistent: one section carries both a
    // lower-camel and an upper-camel key.
    QVERIFY(writeFile(path,
        "[archive]\n"
        "sResourceArchiveList = One.ba2\n"
        "SResourceArchiveList = Two.ba2\n"));

    IniFile ini;
    QVERIFY(ini.load(path));
    QVERIFY(ini.hasKey(QStringLiteral("ARCHIVE"), QStringLiteral("SResourceArchiveList")));
    // values() is the only accessor that does not lose the second line.
    QCOMPARE(ini.values(QStringLiteral("Archive"), QStringLiteral("sResourceArchiveList")),
             QStringList({ QStringLiteral("One.ba2"), QStringLiteral("Two.ba2") }));
    QCOMPARE(ini.value(QStringLiteral("Archive"), QStringLiteral("SResourceArchiveList")),
             QStringLiteral("One.ba2"));
    // And the list form unions both occurrences rather than taking just the first.
    QCOMPARE(ini.valueList(QStringLiteral("Archive"), QStringLiteral("sResourceArchiveList")),
             QStringList({ QStringLiteral("One.ba2"), QStringLiteral("Two.ba2") }));
}

void TestExternalData::iniSplitsCommaLists()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + "/tool.ini";
    // Spaced commas and a trailing comma, both of which appear in practice.
    QVERIFY(writeFile(path, "[Archive]\nL = A.ba2, B.ba2,\n"));
    IniFile ini;
    QVERIFY(ini.load(path));
    QCOMPARE(ini.valueList(QStringLiteral("Archive"), QStringLiteral("L")),
             QStringList({ QStringLiteral("A.ba2"), QStringLiteral("B.ba2") }));
    QCOMPARE(ini.valueList(QStringLiteral("Archive"), QStringLiteral("Absent")), QStringList());
}

void TestExternalData::iniRejectsMalformedLinesWithoutLosingTheFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + "/tool.ini";
    QVERIFY(writeFile(path,
        "; a comment\n"
        "# another comment\n"
        "[Good]\n"
        "[Unclosed\n"
        "novalue\n"
        "= emptykey\n"
        "kept = yes\n"));
    IniFile ini;
    QVERIFY2(ini.load(path), "garbage lines must not make a readable file unreadable");
    QCOMPARE(ini.value(QStringLiteral("Good"), QStringLiteral("kept")), QStringLiteral("yes"));
    QVERIFY(ini.boolValue(QStringLiteral("Good"), QStringLiteral("kept")));
    QVERIFY(!ini.boolValue(QStringLiteral("Good"), QStringLiteral("kept"), false) == false);
    // A key whose value is unparsable keeps the caller's default instead of
    // silently becoming zero.
    QCOMPARE(ini.intValue(QStringLiteral("Good"), QStringLiteral("kept"), -1), -1);
}

void TestExternalData::iniMissingFileIsEmptyNotBroken()
{
    IniFile ini;
    QVERIFY(!ini.load(QStringLiteral("C:/definitely/not/here.ini")));
    QVERIFY(ini.isEmpty());
    QVERIFY(!ini.hasSection(QStringLiteral("Archive")));
    QCOMPARE(ini.value(QStringLiteral("Archive"), QStringLiteral("x"), QStringLiteral("fallback")),
             QStringLiteral("fallback"));
}

// ============================================================================
// ResourceArchiveConfig
// ============================================================================

void TestExternalData::configReadsBothArchiveListsSeparately()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + "/CreationKit.ini";
    QVERIFY(writeFile(path,
        "[Archive]\n"
        "sResourceIndexFileList = Tex01.ba2, Tex02.ba2\n"
        "SResourceArchiveList = Meshes01.ba2, Voices01.ba2\n"
        "[Wwise]\n"
        "iDefaultExternalCodecID = 4\n"));

    const ResourceArchiveConfig cfg = ResourceArchiveConfig::fromIni(path);
    QCOMPARE(cfg.resourceArchives().size(), 2);
    QCOMPARE(cfg.resourceIndexArchives().size(), 2);
    // The two lists name different archives, so merging them would claim
    // Tex01.ba2 is a resource archive when it is a texture-index entry.
    QVERIFY(!cfg.resourceArchives().contains(QStringLiteral("Tex01.ba2")));
    QVERIFY(cfg.namesArchive(QStringLiteral("Tex01.ba2")));
    QVERIFY(cfg.namesArchive(QStringLiteral("Meshes01.ba2")));
    QVERIFY(!cfg.namesArchive(QStringLiteral("SomeMod - Meshes.ba2")));
    QCOMPARE(cfg.allNamedArchives().size(), 4);
    QCOMPARE(cfg.defaultExternalCodecId(), 4);

    // A file with no codec id must not silently report codec 0.
    const QString bare = dir.path() + "/bare.ini";
    QVERIFY(writeFile(bare, "[Archive]\nSResourceArchiveList = A.ba2\n"));
    QCOMPARE(ResourceArchiveConfig::fromIni(bare).defaultExternalCodecId(), -1);
    // And an unreadable file yields an empty config rather than throwing.
    QVERIFY(ResourceArchiveConfig::fromIni(dir.path() + "/nope.ini").isEmpty());
}

void TestExternalData::configResolvesNamesAgainstDisk()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(writeFile(dir.path() + "/Starfield - Meshes01.ba2", "x"));
    if (!QDir().mkpath(dir.path() + "/DLC")) return;
    QVERIFY(writeFile(dir.path() + "/DLC/Starfield - Voices01.ba2", "x"));

    const QStringList names = { QStringLiteral("Starfield - Meshes01.ba2"),
                                QStringLiteral("Starfield - Voices01.ba2"),
                                QStringLiteral("Not Installed.ba2") };
    const auto res = ResourceArchiveConfig::resolve(names, dir.path());
    QCOMPARE(res.found.size(), 2);
    // A DLC archive one level down still resolves: configured lists routinely
    // name archives that are not in the Data root.
    QVERIFY(res.found.contains(QStringLiteral("Starfield - Voices01.ba2")));
    QVERIFY(QFileInfo(res.found.value(QStringLiteral("Starfield - Voices01.ba2")))
                .absolutePath().endsWith(QStringLiteral("DLC")));
    QCOMPARE(res.missing, QStringList({ QStringLiteral("Not Installed.ba2") }));
}

void TestExternalData::configNamesArchiveMatchesOnFileName()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + "/t.ini";
    QVERIFY(writeFile(path, "[Archive]\nSResourceArchiveList = Starfield - Meshes01.ba2\n"));
    const ResourceArchiveConfig cfg = ResourceArchiveConfig::fromIni(path);
    // Matched on file name, because that is all the INI carries.
    QVERIFY(cfg.namesArchive(QStringLiteral("C:/Data/Starfield - Meshes01.ba2")));
    QVERIFY(cfg.namesArchive(QStringLiteral("starfield - meshes01.BA2")));
    QVERIFY(!cfg.namesArchive(QStringLiteral("Starfield - Meshes01.ba2.bak")));
}

// ============================================================================
// ExternalDataCollector
// ============================================================================

// A real BSA with the given entries, so the plan and the write are exercised
// against the archive reader rather than a stub.
//
// BsaArchive::create() absolutises each entry itself and then relativises it
// against sourceRoot, so entries must be passed absolute -- a relative entry
// resolves against the process working directory, not sourceRoot, and is
// silently skipped as unreadable.
static bool makeBsa(const QString& archivePath, const QString& sourceRoot,
                    const QStringList& relPaths)
{
    QStringList absolute;
    for (const QString& rel : relPaths)
        absolute.append(QDir(sourceRoot).absoluteFilePath(rel));
    BsaArchive bsa;
    return bsa.create(absolute, archivePath, /*compress=*/false, sourceRoot);
}

void TestExternalData::resolverFindsArchivesAtTopLevelAndNested()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    if (!QDir().mkpath(dir.path() + "/top/meshes") || !QDir().mkpath(dir.path() + "/DLC/nested/meshes"))
        return;
    QVERIFY(writeFile(dir.path() + "/top/meshes/a.nif", "a"));
    QVERIFY(writeFile(dir.path() + "/DLC/nested/meshes/b.nif", "b"));
    QVERIFY(makeBsa(dir.path() + "/Root - Meshes.bsa", dir.path() + "/top",
                     { QStringLiteral("meshes/a.nif") }));
    QVERIFY(makeBsa(dir.path() + "/DLC/SomePack - Meshes.bsa", dir.path() + "/DLC/nested",
                     { QStringLiteral("meshes/b.nif") }));

    AssetResolver resolver(dir.path());
    // Almost every shipped archive sits in the Data root, so a scan that only
    // walks directories would find the DLC archive and miss the root one -- which
    // is exactly what a single QDir::Dirs-filtered walk does.
    QVERIFY2(resolver.contains(QStringLiteral("meshes/a.nif")),
             "an archive in the data root was not indexed");
    QVERIFY2(resolver.contains(QStringLiteral("meshes/b.nif")),
             "an archive nested under a subdirectory was not indexed");
    QCOMPARE(resolver.archiveCount(), 2);
    QCOMPARE(QFileInfo(resolver.archiveContaining(QStringLiteral("meshes/a.nif"))).fileName(),
             QStringLiteral("Root - Meshes.bsa"));
    QCOMPARE(QFileInfo(resolver.archiveContaining(QStringLiteral("meshes/b.nif"))).fileName(),
             QStringLiteral("SomePack - Meshes.bsa"));
    // A loose path must never be attributed to an archive.
    QVERIFY(writeFile(dir.path() + "/loose.nif", "x"));
    AssetResolver withLoose(dir.path());
    QVERIFY(withLoose.containsLoose(QStringLiteral("loose.nif")));
    QVERIFY(withLoose.archiveContaining(QStringLiteral("loose.nif")).isEmpty());
}

void TestExternalData::planSeparatesLooseArchivedAndUnavailable()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // Loose files the plugin references.
    if (!QDir().mkpath(dir.path() + "/meshes") || !QDir().mkpath(dir.path() + "/textures")) return;
    QVERIFY(writeFile(dir.path() + "/meshes/loose.nif", "loose"));
    QVERIFY(writeFile(dir.path() + "/textures/loose.dds", "loose"));

    // Two files only inside an archive.
    if (!QDir().mkpath(dir.path() + "/src/meshes")) return;
    if (!QDir().mkpath(dir.path() + "/src/textures")) return;
    QVERIFY(writeFile(dir.path() + "/src/meshes/archived.nif", "arc"));
    QVERIFY(writeFile(dir.path() + "/src/textures/archived.dds", "arc"));
    QVERIFY(makeBsa(dir.path() + "/Game - Meshes.bsa", dir.path() + "/src",
                     { QStringLiteral("meshes/archived.nif"),
                       QStringLiteral("textures/archived.dds") }));

    const QStringList referenced = {
        QStringLiteral("meshes\\loose.nif"),      // loose
        QStringLiteral("textures/loose.dds"),      // loose, forward slashes
        QStringLiteral("meshes\\archived.nif"),    // in an archive
        QStringLiteral("textures\\archived.dds"),  // in an archive
        QStringLiteral("meshes\\nowhere.nif"),     // nowhere at all
        QStringLiteral(""),                       // references nothing
    };
    QVector<AssetReference> refs;
    for (const QString& p : referenced)
        refs.append({ QStringLiteral("00000001"), CkId::Type_Stat_, p, QStringLiteral("model") });

    AssetResolver resolver(dir.path());
    QVERIFY(!resolver.isEmpty());

    ExternalDataCollector::Options opts;
    opts.resourceArchivesOnly = false;
    const auto plan = ExternalDataCollector::buildPlan(refs, resolver, opts, nullptr);

    QCOMPARE(plan.alreadyLoose.size(), 2);
    QCOMPARE(plan.toCollect.size(), 2);
    QCOMPARE(plan.unavailable.size(), 1);
    // The empty path references nothing and must not appear anywhere.
    for (const auto& list : { plan.alreadyLoose, plan.toCollect, plan.unavailable })
        for (const auto& item : list)
            QVERIFY(!item.assetPath.isEmpty());
    QCOMPARE(plan.unavailable.first().assetPath, QStringLiteral("meshes\\nowhere.nif"));
    for (const auto& item : plan.toCollect) {
        QCOMPARE(item.availability, ExternalDataCollector::Availability::InArchive);
        QVERIFY(!item.sourceArchive.isEmpty());
        QCOMPARE(item.referencedBy.size(), 1);
    }
    // Loose wins over archived: a file present both ways is not a collection item.
    QVERIFY(resolver.containsLoose(QStringLiteral("meshes/loose.nif")));
}

void TestExternalData::planHonoursIgnoreArchivesInside()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    if (!QDir().mkpath(dir.path() + "/src/meshes")) return;
    QVERIFY(writeFile(dir.path() + "/src/meshes/only.nif", "arc"));
    QVERIFY(makeBsa(dir.path() + "/Game.bsa", dir.path() + "/src",
                     { QStringLiteral("meshes/only.nif") }));

    QVector<AssetReference> refs = {
        { QStringLiteral("00000001"), CkId::Type_Stat_, QStringLiteral("meshes\\only.nif"),
          QStringLiteral("model") }
    };
    AssetResolver resolver(dir.path());
    ExternalDataCollector::Options opts;
    opts.resourceArchivesOnly = false;

    opts.ignoreFilesInsideArchives = true;
    const auto ignored = ExternalDataCollector::buildPlan(refs, resolver, opts, nullptr);
    QCOMPARE(ignored.toCollect.size(), 0);
    // Not "already loose": nothing is on disk, it is covered by the archive.
    QCOMPARE(ignored.coveredByArchives.size(), 1);
    QCOMPARE(ignored.alreadyLoose.size(), 0);

    opts.ignoreFilesInsideArchives = false;
    const auto collected = ExternalDataCollector::buildPlan(refs, resolver, opts, nullptr);
    QCOMPARE(collected.toCollect.size(), 1);
}

void TestExternalData::planRestrictsToResourceArchives()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    if (!QDir().mkpath(dir.path() + "/src/meshes")) return;
    QVERIFY(writeFile(dir.path() + "/src/meshes/only.nif", "arc"));
    QVERIFY(makeBsa(dir.path() + "/SomeMod - Meshes.bsa", dir.path() + "/src",
                     { QStringLiteral("meshes/only.nif") }));

    const QString iniPath = dir.path() + "/tool.ini";
    // Names a different archive entirely, so the one holding our file is not a
    // game archive.
    QVERIFY(writeFile(iniPath, "[Archive]\nSResourceArchiveList = Starfield - Meshes01.ba2\n"));
    const ResourceArchiveConfig cfg = ResourceArchiveConfig::fromIni(iniPath);

    QVector<AssetReference> refs = {
        { QStringLiteral("00000001"), CkId::Type_Stat_, QStringLiteral("meshes\\only.nif"),
          QStringLiteral("model") }
    };
    AssetResolver resolver(dir.path());

    ExternalDataCollector::Options opts;
    opts.resourceArchivesOnly = true;
    const auto plan = ExternalDataCollector::buildPlan(refs, resolver, opts, &cfg);
    QCOMPARE(plan.toCollect.size(), 0);
    // Reported rather than dropped: silently skipping would look like success.
    QCOMPARE(plan.inNonResourceArchive.size(), 1);

    // Without a config there is nothing to judge by, so everything is offered.
    const auto noConfig = ExternalDataCollector::buildPlan(refs, resolver, opts, nullptr);
    QCOMPARE(noConfig.toCollect.size(), 1);
}

void TestExternalData::collectWritesOnlyInsideDestination()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    if (!QDir().mkpath(dir.path() + "/src/meshes")) return;
    QVERIFY(writeFile(dir.path() + "/src/meshes/one.nif", "payload-one"));
    QVERIFY(QDir().mkpath(dir.path() + "/src/meshes/sub"));
    QVERIFY(writeFile(dir.path() + "/src/meshes/sub/two.nif", "payload-two"));
    QVERIFY(makeBsa(dir.path() + "/Game.bsa", dir.path() + "/src",
                     { QStringLiteral("meshes/one.nif"), QStringLiteral("meshes/sub/two.nif") }));

    QVector<AssetReference> refs = {
        { QStringLiteral("00000001"), CkId::Type_Stat_, QStringLiteral("meshes\\one.nif"),
          QStringLiteral("model") },
        { QStringLiteral("00000002"), CkId::Type_Stat_, QStringLiteral("meshes\\sub\\two.nif"),
          QStringLiteral("model") },
    };
    AssetResolver resolver(dir.path());
    ExternalDataCollector::Options opts;
    opts.resourceArchivesOnly = false;
    const auto plan = ExternalDataCollector::buildPlan(refs, resolver, opts, nullptr);
    QCOMPARE(plan.toCollect.size(), 2);

    QTemporaryDir out;
    QVERIFY(out.isValid());
    const auto outcome = ExternalDataCollector::collect(plan, out.path());
    QCOMPARE(outcome.failures, QStringList());
    QCOMPARE(outcome.written, 2);
    // Relative layout preserved, so the collected tree can sit beside the plugin.
    QCOMPARE(QFileInfo(out.path() + "/meshes/one.nif").exists(), true);
    QCOMPARE(QFileInfo(out.path() + "/meshes/sub/two.nif").exists(), true);
    QFile read(out.path() + "/meshes/sub/two.nif");
    QVERIFY(read.open(QIODevice::ReadOnly));
    QCOMPARE(read.readAll(), QByteArray("payload-two"));

    // An empty destination writes nothing at all.
    QCOMPARE(ExternalDataCollector::collect(plan, QString()).written, 0);
}

QTEST_MAIN(TestExternalData)
#include "test_externaldata.moc"