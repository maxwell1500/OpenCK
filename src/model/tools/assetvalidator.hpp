#pragma once

#include "../world/data.hpp"
#include "../../../libs/files/filepaths.hpp"

#include <QString>
#include <QVector>

class AssetValidator
{
public:
    struct ValidationIssue
    {
        enum Severity { Error, Warning, Info };
        Severity severity;
        QString category;
        QString message;
        QString recordId;
        QString filePath;
    };

    struct ValidationReport
    {
        QVector<ValidationIssue> issues;
        int errors() const;
        int warnings() const;
        int infos() const;
    };

    static ValidationReport validateAll(const Data& data, const QString& dataDir);
    static ValidationReport validateNif(const QString& nifPath);
    static ValidationReport validateTexture(const QString& texPath);
    static ValidationReport validateSound(const QString& soundPath);
    static ValidationReport validateMasters(const Data& data);
    static ValidationReport validateFormIds(const Data& data);
    static ValidationReport validateOrphanedRecords(const Data& data);
    static ValidationReport validateReferences(const Data& data);

    /// Structural relationships the per-field reference check cannot see: a
    /// placed reference whose parent cell or base object is missing, a cell
    /// whose owner does not exist, dialogue and quest links that point at
    /// nothing, and records whose component set is missing so their form dialog
    /// would render empty.
    static ValidationReport validateRelationships(const Data& data);

    /// Asset paths that escape the data directory or name an absolute location.
    /// Checked without touching the filesystem, since the path may legitimately
    /// live inside an archive.
    static ValidationReport validateAssetPaths(const Data& data);

    /// True when the report contains at least one issue of Error severity.
    /// This is the pre-save policy hook: a caller can block the save, warn, or
    /// ignore, by choosing what to do with the answer.
    static bool shouldBlockSave(const ValidationReport& report);
    static bool shouldBlockSave(const ValidationReport& report, int allowedErrors);

private:
    static ValidationReport mergeReports(const QVector<ValidationReport>& reports);
    static bool isPowerOf2(int value);
    static bool isSupportedDdsFormat(quint32 format);
    static bool isWavValidSampleRate(quint32 sampleRate);
    static bool isWavValidBitDepth(quint16 bitsPerSample);
    static bool isEscapingPath(const QString& path);
};
