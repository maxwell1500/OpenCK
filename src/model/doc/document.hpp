#ifndef DOCUMENT_H
#define DOCUMENT_H

#include "../tools/reports.hpp"
#include "../world/data.hpp"
#include "../../../libs/files/esm/esmreader.hpp"
#include "../../../libs/files/esm/esmwriter.hpp"
#include "../../../libs/files/filepaths.hpp"

#include <QObject>
#include <memory>

class ESMReader;
class ESMWriter;

class Document : public QObject
{
    Q_OBJECT

public:
    Document(const QStringList& contentFiles, const QString& savePath, bool isNew);
    ~Document();

    void save(const QString& savePath);

    bool isNewFile() const;
    bool isBase() const;
    const QString getSavePath() const;
    QStringList getContentFiles() const;

    // TES4 file flags (FileFlag::Master, FileFlag::LightMaster, ...). These
    // control whether the saved plugin is an ESM, an ESL (light master), or a
    // plain ESP. Preserved from the loaded header; settable for new files.
    quint32 fileFlags() const { return mFileFlags; }
    void setFileFlags(quint32 flags) { mFileFlags = flags; }
    bool isLightMaster() const { return (mFileFlags & 0x200) != 0; }
    void setLightMaster(bool on) { on ? (mFileFlags |= 0x200) : (mFileFlags &= ~0x200u); }

    std::shared_ptr<ReportModel> getReport();

    const Data& getData() const;
    Data& getData();

private:
    void createNew();
    void writeCellChildrenGroups(ESMWriter& writer, quint32 cellId);

    FilePaths paths;
    QStringList contentFiles;
    QString savePath;
    bool newFile;
    bool base;
    quint32 mFileFlags = 0;

    std::shared_ptr<ReportModel> reports;

    std::unique_ptr<Data> data;
};

#endif // DOCUMENT_H
