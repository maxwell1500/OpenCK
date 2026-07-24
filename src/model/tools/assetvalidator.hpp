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

private:
    static ValidationReport mergeReports(const QVector<ValidationReport>& reports);
    static bool isPowerOf2(int value);
    static bool isSupportedDdsFormat(quint32 format);
    static bool isWavValidSampleRate(quint32 sampleRate);
    static bool isWavValidBitDepth(quint16 bitsPerSample);
};
