#include "pluginmerger.hpp"

#include "../../../libs/files/esm/esmwriter.hpp"
#include "../../../libs/files/log/logger.hpp"

#include <QSaveFile>

namespace openck {

MergeWriteReport PluginMerger::mergeToFile(const QString& basePath,
                                           const QString& minePath,
                                           const QString& theirsPath,
                                           const QString& outputPath)
{
    MergeWriteReport report;
    report.outputPath = outputPath;

    const RecordMergePlan plan = RecordMerger::merge(basePath, minePath, theirsPath);

    QSaveFile out(outputPath);
    if (!out.open(QIODevice::WriteOnly))
    {
        report.error = QStringLiteral("Cannot write %1").arg(outputPath);
        return report;
    }

    ESMWriter writer;
    writer.setAuthor(QStringLiteral("OpenCK merge"));
    writer.setDescription(QStringLiteral("Three-way merge output"));
    // Binds the writer to the file and emits the TES4 record; every record
    // below streams through that same device.
    writer.save(out);

    for (const MergeAction& action : plan.actions)
    {
        if (action.kind == MergeAction::Skip)
        {
            ++report.skipped;
            continue;
        }
        if (action.kind == MergeAction::Conflict)
        {
            ++report.conflictsRetained;
            report.conflictReasons.append(action.reason);
        }

        const RecordSnapshot& r = action.record;
        RecHeader header;
        header.id = r.formId;
        header.flags.val = r.flags;
        header.vcDay = r.vcDay;
        header.vcMonth = r.vcMonth;
        header.vcLastUser = r.vcLastUser;
        header.vcCurrUser = r.vcCurrUser;
        header.version = r.version;
        header.unknown = r.unknown;

        writer.startRecord(r.type, header);
        for (const SubSnapshot& s : r.subs)
        {
            writer.startSubRecord(s.name);
            writer.writeRawData(s.payload.constData(), s.payload.size());
            writer.endSubRecord();
        }
        writer.endRecord();
        ++report.recordsWritten;
    }

    if (!out.commit())
    {
        report.error = QStringLiteral("Commit failed for %1").arg(outputPath);
        return report;
    }

    LOG_INFO(QString("PluginMerger: wrote %1 record(s) (%2 conflict(s) kept, %3 skipped)")
                 .arg(report.recordsWritten)
                 .arg(report.conflictsRetained)
                 .arg(report.skipped));
    return report;
}

} // namespace openck
