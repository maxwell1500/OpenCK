#ifndef COLUMNIMP_H
#define COLUMNIMP_H

#include "basecolumn.hpp"
#include "columns.hpp"
#include "../../../libs/files/esm/variant.hpp"

#include <QVariant>

template<typename ESXRecord>
struct StringIdColumn : public Column<ESXRecord>
{
    StringIdColumn(bool hidden = false) :
        Column<ESXRecord>(ColumnId::ColumnId_Id, BaseColumn::Display_Id,
            hidden ? 0 : BaseColumn::Flag_Table | BaseColumn::Flag_Dialogue)
    {

    }

    virtual QVariant get(const Record<ESXRecord>& record) const
    {
        return record.get().editorId;
    }

    virtual bool isEditable() const
    {
        return false;
    }
};

template<typename ESXRecord>
struct RecordStateColumn : public Column<ESXRecord>
{
    RecordStateColumn() :
        Column<ESXRecord>(ColumnId::ColumnId_Modification, BaseColumn::Display_RecordState)
    {

    }

    virtual QVariant get(const Record<ESXRecord>& record) const
    {
        if (record.state == State::State_Erased)
        {
            return static_cast<int>(State::State_Deleted);
        }

        return static_cast<int>(record.state);
    }

    virtual void set(Record<ESXRecord>& record, const QVariant& data)
    {
        record.state = static_cast<State>(data.toInt());
    }

    virtual bool isEditable() const
    {
        return true;
    }

    virtual bool isUserEditable() const
    {
        return false;
    }
};

template<typename ESXRecord>
struct FixedRecordTypeColumn : public Column<ESXRecord>
{
    int type;

    FixedRecordTypeColumn(int type) :
        Column<ESXRecord>(ColumnId::ColumnId_RecordType, BaseColumn::Display_SignedInteger32, 0),
        type(type)
    {

    }

    virtual QVariant get(const Record<ESXRecord>& record) const
    {
        return type;
    }

    virtual bool isEditable() const
    {
        return false;
    }
};

template<typename ESXRecord>
struct VarTypeColumn : public Column<ESXRecord>
{
    VarTypeColumn(BaseColumn::Display display) :
        Column<ESXRecord>(ColumnId::ColumnId_ValueType, display, BaseColumn::Flag_Table | BaseColumn::Flag_Dialogue)
    {

    }

    virtual QVariant get(const Record<ESXRecord>& record) const
    {
        return static_cast<int>(record.get().value.getType());
    }

    virtual void set(Record<ESXRecord>& record, const QVariant& data)
    {
        ESXRecord newRecord = record.get();
        newRecord.value.setType(static_cast<VariantType>(data.toInt()));
        record.setModified(newRecord);
    }

    virtual bool isEditable() const
    {
        return true;
    }
};

template<typename ESXRecord>
struct StringColumn : public Column<ESXRecord>
{
    QString columnName;
    QString ESXRecord::*getter;

    StringColumn(const QString& name, QString ESXRecord::*g)
        : Column<ESXRecord>(ColumnId::ColumnId_Custom, BaseColumn::Display_String),
          columnName(name),
          getter(g)
    {

    }

    virtual QVariant get(const Record<ESXRecord>& record) const override
    {
        return record.get().*getter;
    }

    virtual bool isEditable() const
    {
        return true;
    }
};

template<typename ESXRecord>
struct IntColumn : public Column<ESXRecord>
{
    QString columnName;
    quint32 ESXRecord::*getter;

    IntColumn(const QString& name, quint32 ESXRecord::*g)
        : Column<ESXRecord>(ColumnId::ColumnId_Custom, BaseColumn::Display_SignedInteger32),
          columnName(name),
          getter(g)
    {

    }

    virtual QVariant get(const Record<ESXRecord>& record) const override
    {
        return static_cast<int>(record.get().*getter);
    }

    virtual void set(Record<ESXRecord>& record, const QVariant& data)
    {
        ESXRecord newRecord = record.get();
        newRecord.*getter = static_cast<quint32>(data.toInt());
        record.setModified(newRecord);
    }

    virtual bool isEditable() const
    {
        return true;
    }
};

template<typename ESXRecord>
struct FloatColumn : public Column<ESXRecord>
{
    QString columnName;
    float ESXRecord::*getter;

    FloatColumn(const QString& name, float ESXRecord::*g)
        : Column<ESXRecord>(ColumnId::ColumnId_Custom, BaseColumn::Display_Float),
          columnName(name),
          getter(g)
    {

    }

    virtual QVariant get(const Record<ESXRecord>& record) const override
    {
        return static_cast<double>(record.get().*getter);
    }

    virtual void set(Record<ESXRecord>& record, const QVariant& data)
    {
        ESXRecord newRecord = record.get();
        newRecord.*getter = static_cast<float>(data.toDouble());
        record.setModified(newRecord);
    }

    virtual bool isEditable() const
    {
        return true;
    }
};

template<typename ESXRecord>
struct VariantColumn : public Column<ESXRecord>
{
    QString columnName;
    Variant ESXRecord::*getter;

    VariantColumn(const QString& name, Variant ESXRecord::*g)
        : Column<ESXRecord>(ColumnId::ColumnId_Custom, BaseColumn::Display_Var),
          columnName(name),
          getter(g)
    {

    }

    virtual QVariant get(const Record<ESXRecord>& record) const override
    {
        return (record.get().*getter).getData();
    }

    virtual bool isEditable() const
    {
        return true;
    }
};

template<typename ESXRecord>
struct VarValueColumn : public Column<ESXRecord>
{
    VarValueColumn() :
        Column<ESXRecord>(ColumnId::ColumnId_Value, BaseColumn::Display_Var, BaseColumn::Flag_Table | BaseColumn::Flag_Dialogue)
    {

    }

    virtual QVariant get(const Record<ESXRecord>& record) const
    {
        switch (record.get().value.getType())
        {
            case VariantType::Var_String:
                return record.get().value.getString();
        
            case VariantType::Var_LString:
                return record.get().value.getLString().string;

            case VariantType::Var_Int:
            case VariantType::Var_Short:
            case VariantType::Var_Long:
                return record.get().value.getInt();

            case VariantType::Var_Float:
                return record.get().value.getFloat();

            default:
                return QVariant();
        }
    }

    virtual void set(Record<ESXRecord>& record, const Variant& data)
    {
        ESXRecord newRecord = record.get();

        switch (newRecord.value.getType())
        {
            case VariantType::Var_String:
                newRecord.value.setString(data.getString());
                break;

            case VariantType::Var_LString:
                newRecord.value.setLString(data.getLString());
                break;

            case VariantType::Var_Int:
            case VariantType::Var_Short:
            case VariantType::Var_Long:
                newRecord.value.setInt(data.getInt());
                break;

            case VariantType::Var_Float:
                newRecord.value.setFloat(data.getFloat());
                break;

            default:
                break;
        }

        record.setModified(newRecord);
    }

    virtual bool isEditable() const
    {
        return true;
    }
};

#endif // COLUMNIMP_H