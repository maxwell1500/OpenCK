#ifndef IRECORDCOLLECTION_HPP
#define IRECORDCOLLECTION_HPP

#include <QByteArray>
#include <QSet>
#include <QString>
#include <QVector>
#include <cstdint>

class ESMWriter;

// One opaque subrecord (on-disk 4-byte name + payload) that a record kept
// unparsed. Exposed type-erased so tools like FormIdCompactor can inspect
// raw payloads of any record type without a typed cast.
struct RawSubPayload
{
    quint32 name = 0;
    QByteArray data;
};

class IRecordCollection
{
public:
    virtual ~IRecordCollection() = default;

    virtual int count() const = 0;
    virtual int searchId(const QString& id) const = 0;
    virtual QString getEditorId(int index) const = 0;
    virtual quint32 getFormId(int index) const = 0;
    virtual void setFormId(int index, quint32 formId) = 0;
    virtual bool containsFormId(quint32 formId) const = 0;
    virtual bool isRecordModified(int index) const = 0;
    virtual int countModifiedRecords() const = 0;
    virtual bool isRecordSaveable(int index) const = 0;
    virtual QVector<QString> getAllIds(bool includeDeleted = true) const = 0;
    virtual void saveModifiedRecords(ESMWriter& writer, uint32_t recordType) const = 0;
    virtual bool saveRecordAt(ESMWriter& writer, uint32_t recordType, int index) const = 0;
    virtual void saveModifiedRecordsExcept(ESMWriter& writer, uint32_t recordType, const QSet<quint64>& skipKeys) const = 0;

    // Opaque subrecords of the record at `index` (copies). Empty when the
    // record type carries no raw subrecords or the index is out of range.
    virtual QVector<RawSubPayload> rawSubRecordsAt(int index) const { return {}; }
};

#endif // IRECORDCOLLECTION_HPP
