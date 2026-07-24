#ifndef BASECOLLECTION_H
#define BASECOLLECTION_H

#include "irecordcollection.hpp"
#include "basecolumn.hpp"
#include "ckid.hpp"
#include "columns.hpp"

#include <QVariant>
#include <QVector>
#include <memory>

class BaseRecord;
class UndoStack;

class BaseCollection : public IRecordCollection
{
public:
    BaseCollection();
    virtual ~BaseCollection();

    virtual int size() const = 0;

    virtual QString getId(int index) const = 0;
    virtual int getIndex(const QString& id) const = 0;
    virtual int getColumns() const = 0;
    virtual const BaseColumn& getColumn(int column) const = 0;
    virtual void removeRows(int index, int count) = 0;
    virtual bool reorderRows(int baseIndex, const QVector<int>& newOrder) = 0;

    virtual QVariant getData(int index, int column) const = 0;
    virtual void setData(int index, int column, const QVariant& data) = 0;

    virtual int searchId(const QString& id) const = 0;
    virtual void replace(int index, const BaseRecord& record) = 0;
    virtual void appendRecord(const BaseRecord& record, CkId::Type type = CkId::Type_None) = 0;
    virtual void insertRecord(const BaseRecord& record, int index, CkId::Type type = CkId::Type_None) = 0;
    virtual void appendBlankRecord(const QString& id, CkId::Type type = CkId::Type::Type_None) = 0;
    virtual void cloneRecord(const QString& src, const QString& dest, const CkId::Type type) = 0;
    virtual bool touchRecord(const QString& id) = 0;
    virtual int getAppendIndex(const QString& id, CkId::Type type = CkId::Type_None) const = 0;
    virtual QVector<QString> getIds(bool listDeleted = true) const = 0;

    virtual const BaseRecord& getRecord(int index) const = 0;
    virtual const BaseRecord& getRecord(const QString& id) const = 0;

    int searchColumnIndex(ColumnId id) const;
    int findColumnIndex(ColumnId id) const;

    virtual std::unique_ptr<BaseRecord> cloneRecordAt(int index) const = 0;

    // IRecordCollection overrides
    int count() const override { return size(); }
    QVector<QString> getAllIds(bool includeDeleted) const override { return getIds(includeDeleted); }

    // Default implementations for IRecordCollection (overridden in Collection<T>)
    QString getEditorId(int index) const override { return getId(index); }
    quint32 getFormId(int) const override { return 0; }
    bool containsFormId(quint32) const override { return false; }
    bool isRecordModified(int) const override { return false; }
    void saveModifiedRecords(ESMWriter&, uint32_t) const override {}

    // Undo-aware operations (implemented in Collection<T>)
    virtual bool removeRecordWithUndo(const QString& id, UndoStack* undoStack) = 0;
    virtual bool cloneRecordWithUndo(const QString& src, const QString& dest, UndoStack* undoStack) = 0;
    virtual void batchCloneWithUndo(const QVector<QString>& srcIds, const QVector<QString>& destIds, UndoStack* undoStack) = 0;
    virtual void batchSetEditorIdWithUndo(const QVector<QString>& srcIds, const QString& newEditorId, UndoStack* undoStack) = 0;

protected:

private:
    BaseCollection(const BaseCollection&);
    BaseCollection& operator=(const BaseCollection&);
};

#endif // BASECOLLECTION_H
