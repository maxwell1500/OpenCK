#ifndef BATCHTOOLS_HPP
#define BATCHTOOLS_HPP

#include "../world/ckid.hpp"

#include <QString>
#include <QStringList>

class Data;

class BatchTools
{
public:
    struct RenameResult
    {
        int recordsRenamed = 0;
        QStringList warnings;
    };

    struct FormIdResult
    {
        int formIdsReassigned = 0;
        QStringList warnings;
    };

    static RenameResult batchRename(Data& data, CkId::Type type,
                                     const QString& findPattern, const QString& replaceWith,
                                     bool useRegex = false);

    static FormIdResult batchReassignFormIds(Data& data, quint32 startFormId = 0x800);
};

#endif // BATCHTOOLS_HPP
