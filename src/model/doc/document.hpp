#ifndef DOCUMENT_H
#define DOCUMENT_H

#include "../tools/reports.hpp"
#include "../world/data.hpp"
#include "../../../libs/files/esm/esmreader.hpp"
#include "../../../libs/files/esm/esmwriter.hpp"
#include "../../../libs/files/filepaths.hpp"

#include <QHash>
#include <QObject>
#include <QSet>
#include <memory>

class ESMReader;
class ESMWriter;
class IRecordCollection;

struct NewPluginOptions
{
    GameFormat::Game game = GameFormat::Game::Unknown;
    QVector<MasterData> masters;
    QString author;
    quint32 nextObjectId = 0x800;
    bool lightMaster = false;
};

// Lookup tables built once per save so the ordered replay, the cell-children
// passthrough and the grouped fallback all agree on which record lives where.
struct SaveIndex
{
    QHash<uint32_t, const IRecordCollection*> collByTag;
    QHash<uint32_t, QHash<quint32, int>> indexByTag;
    QHash<quint64, int> opaqueByKey;
};

class Document : public QObject
{
    Q_OBJECT

public:
    Document(const QStringList& contentFiles, const QString& savePath, bool isNew,
             const NewPluginOptions& options = NewPluginOptions());
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
    void writeCellChildrenGroups(ESMWriter& writer, quint32 cellId,
        const SaveIndex& index, QSet<quint64>& written);
    void writeOpaqueRecord(ESMWriter& writer, int opaqueIndex);

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
