#include "columns.hpp"
#include "ckid.hpp"
#include "basecolumn.hpp"

struct ColumnDesc
{
    int id;
    QString name;
};

const ColumnDesc columnNames[] =
{
    { ColumnId_Id, "ID" },
    { ColumnId_Value, "Value" },
    { ColumnId_ValueType, "Value Type" },
    { ColumnId_Modification, "Modified" },
    { ColumnId_RecordType, "Record Type" },

    { ColumnId_End, 0 }
};

QString getColumnName(ColumnId column)
{
    for (int i = 0; i < ColumnId_End; ++i)
    {
        if (column == columnNames[i].id)
        {
            return columnNames[i].name;
        }
    }

    return "";
}

namespace Columns
{
    static QString varTypeEnums[8] =
    {
        "None", "Short", "Integer", "Long", "Float", "String", "Bool", 0
    };

    static QString modificationEnums[5] =
    {
        "Base", "Modified", "Added", "Deleted", 0
    };

    bool hasNames(ColumnId column)
    {
        return getEnumNames(column) != nullptr || column == ColumnId::ColumnId_RecordType;
    }

    QString* getEnumNames(ColumnId column)
    {
        switch (column)
        {
            case ColumnId::ColumnId_Modification: return modificationEnums;
            case ColumnId::ColumnId_ValueType: return varTypeEnums;
        }
        return nullptr;
    }

    QVector<QPair<int, QString>> getEnums(ColumnId column)
    {
        QVector<QPair<int, QString>> enums;

        if (QString* nameTable = getEnumNames(column))
        {
            for (int i = 0; !nameTable[i].isNull(); i++)
            {
                enums.push_back(QPair<int, QString>(i, nameTable[i]));
            }
        }
        else if (column == ColumnId_RecordType)
        {
            enums.push_back(QPair<int, QString>(CkId::Type_None, ""));

            for (int i = CkId::Type_None + 1; i < CkId::NumTypes; i++)
            {
                enums.push_back(QPair<int, QString>(i, CkId(static_cast<CkId::Type>(i)).getTypeName()));
            }
        }

        return enums;
    }
}