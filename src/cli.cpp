#include "cli.hpp"

#include "logger.hpp"
#include "filepaths.hpp"

#include "model/world/data.hpp"
#include "model/doc/messages.hpp"
#include "libs/files/data/dataexporter.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QString>
#include <QTextStream>

namespace OpenCK::Cli {

namespace {

bool loadData(Data& data, const QString& plugin, Messages& messages)
{
    if (data.preload(plugin, false) < 0)
    {
        LOG_ERROR(QString("CLI: preload failed for %1").arg(plugin));
        QTextStream(stdout) << "preload failed for " << plugin << "\n";
        return false;
    }
    // continueLoading returns true when all record groups are loaded.
    int guard = 0;
    while (!data.continueLoading(messages))
    {
        if (++guard > 10000)
        {
            LOG_ERROR("CLI: continueLoading did not terminate");
            QTextStream(stdout) << "continueLoading did not terminate\n";
            return false;
        }
    }
    return true;
}

// Resolves a possibly-absolute plugin path: the parent directory becomes the
// Data directory (preload joins dataDir + filename), so absolute and
// relative paths both work.
QString resolvePlugin(const QString& path, FilePaths& paths)
{
    const QFileInfo info(path);
    const QString dir = info.absolutePath();
    if (QDir(dir).exists())
    {
        paths.dataDir.setPath(dir);
        return info.fileName();
    }
    return path;
}
int cmdInfo(const QString& plugin)
{
    FilePaths paths("OpenCK");
    const QString resolved = resolvePlugin(plugin, paths);
    Data data({ resolved }, paths);
    Messages messages(Message::Default);
    if (!loadData(data, resolved, messages))
    {
        return 1;
    }

    int total = 0;
    const QVector<Data::TypedCollection> collections = data.allCollectionsWithTypes();
    QTextStream out(stdout);
    for (const auto& entry : collections)
    {
        const IRecordCollection* collection = entry.collection;
        if (!collection) continue;
        const int count = collection->count();
        if (count == 0) continue;
        out << CkId(entry.type).getTypeName() << ": " << count << "\n";
        total += count;
    }
    out << "TOTAL: " << total << "\n";
    out.flush();
    LOG_INFO(QString("CLI info: %1 record(s) in %2").arg(total).arg(plugin));
    return 0;
}

int cmdExport(const QStringList& args)
{
    if (args.isEmpty())
    {
        QTextStream(stdout) << "export: missing plugin path\n";
        return 1;
    }
    const QString plugin = args[0];

    QString format = "json";
    QString output;
    QStringList types;
    for (int i = 1; i < args.size(); ++i)
    {
        if (args[i] == "--format" && i + 1 < args.size()) format = args[++i];
        else if (args[i] == "--out" && i + 1 < args.size()) output = args[++i];
        else if (args[i] == "--types" && i + 1 < args.size()) types = args[++i].split(',');
    }

    FilePaths paths("OpenCK");
    const QString resolved = resolvePlugin(plugin, paths);
    Data data({ resolved }, paths);
    Messages messages(Message::Default);
    if (!loadData(data, resolved, messages))
    {
        return 1;
    }

    if (output.isEmpty())
    {
        QFileInfo info(plugin);
        output = info.completeBaseName() + "." + format;
    }

    // Default to all known record types when --types is omitted.
    if (types.isEmpty())
    {
        types = { "NPC_", "WEAP_", "ARMOR_", "SPEL_", "MGEF", "QUST_",
                  "DIAL_", "INFO_", "PACK_", "ALCH_", "INGR_", "CONT_",
                  "ENCH_", "BOOK_", "MISC_", "ACTI_", "STAT_", "RACE_",
                  "CLASS_", "FACT_", "PERK_", "CEL_", "WRLD_", "LOCT_", "REFR_" };
    }

    DataExporter::ExportFilter filter;
    filter.onlyModified = false; // headless export includes base records
    DataExporter::ExportResult result;

    if (format.compare("csv", Qt::CaseInsensitive) == 0)
        result = DataExporter::exportToCSV(data, types, output, filter);
    else if (format.compare("xml", Qt::CaseInsensitive) == 0)
        result = DataExporter::exportToXML(data, types, output, filter);
    else
        result = DataExporter::exportToJSON(data, types, output, filter);

    QTextStream out(stdout);
    if (result.recordsExported > 0)
    {
        out << "Exported " << result.recordsExported << " record(s) to "
            << result.outputPath << "\n";
        out.flush();
        LOG_INFO(QString("CLI export: %1 record(s) -> %2")
            .arg(result.recordsExported).arg(result.outputPath));
        return 0;
    }
    out << "Export produced no records (error: " << result.error << ")\n";
    out.flush();
    return 1;
}

int cmdSelfTest()
{
    const QString dir = QCoreApplication::applicationDirPath();
    const QFileInfoList tests = QDir(dir).entryInfoList(
        QStringList() << QStringLiteral("test_*.exe"), QDir::Files, QDir::Name);

    QTextStream out(stdout);
    int passed = 0;
    int failed = 0;
    for (const QFileInfo& info : tests)
    {
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start(info.absoluteFilePath());
        // Game-data tests (e.g. test_archivebrowser opening a real BSA) can
        // legitimately take minutes in a Debug build; keep a generous ceiling
        // that still terminates a wedged test.
        if (!process.waitForFinished(600000))
        {
            process.kill();
            process.waitForFinished();
            ++failed;
            out << "FAIL " << info.fileName() << " (did not finish)\n";
        }
        else if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0)
        {
            ++passed;
            out << "PASS " << info.fileName() << "\n";
        }
        else
        {
            ++failed;
            out << "FAIL " << info.fileName() << " (exit code "
                << process.exitCode() << ")\n";
        }
        out.flush();
    }

    out << QStringLiteral("SELFTEST: %1 passed, %2 failed, %3 total\n")
        .arg(passed).arg(failed).arg(tests.size());
    out.flush();
    LOG_INFO(QString("CLI selftest: %1 passed, %2 failed").arg(passed).arg(failed));
    return failed == 0 && !tests.isEmpty() ? 0 : 1;
}

} // namespace

QString usage()
{
    return QStringLiteral(
        "OpenCK command-line interface\n"
        "  openck --cli export <plugin> [--format json|csv|xml] [--out path] [--types T1,T2]\n"
        "  openck --cli info <plugin>\n"
        "  openck --cli selftest\n"
        "  openck --cli help\n");
}

int run(int argc, char* argv[])
{
    QStringList args;
    for (int i = 0; i < argc; ++i)
    {
        args << QString::fromLocal8Bit(argv[i]);
    }

    // Args are shifted so args[0] is the first argument after "openck".
    // find "--cli" and take everything after it.
    const int cliIdx = args.indexOf(QStringLiteral("--cli"));
    if (cliIdx < 0)
    {
        QTextStream(stdout) << usage();
        return 1;
    }

    const QStringList cliArgs = args.mid(cliIdx + 1);
    if (cliArgs.isEmpty() || cliArgs[0] == "help")
    {
        QTextStream(stdout) << usage();
        return 0;
    }

    if (cliArgs[0] == "info")
    {
        if (cliArgs.size() < 2)
        {
            QTextStream(stdout) << "info: missing plugin path\n";
            return 1;
        }
        return cmdInfo(cliArgs[1]);
    }

    if (cliArgs[0] == "export")
    {
        return cmdExport(cliArgs.mid(1));
    }

    if (cliArgs[0] == "selftest")
    {
        return cmdSelfTest();
    }

    QTextStream(stdout) << "Unknown command: " << cliArgs[0] << "\n" << usage();
    return 1;
}

} // namespace OpenCK::Cli
