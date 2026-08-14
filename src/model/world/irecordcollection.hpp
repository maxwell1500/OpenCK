#ifndef IRECORDCOLLECTION_HPP
#define IRECORDCOLLECTION_HPP

#include <QString>
#include <QVector>
#include <cstdint>

class ESMWriter;

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
    virtual QVector<QString> getAllIds(bool includeDeleted = true) const = 0;
    virtual void saveModifiedRecords(ESMWriter& writer, uint32_t recordType) const = 0;
};

#endif // IRECORDCOLLECTION_HPP
