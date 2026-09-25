#ifndef SETREFRPARENTCELLCOMMAND_H
#define SETREFRPARENTCELLCOMMAND_H

#include "command.hpp"

class Data;

class SetRefrParentCellCommand : public Command
{
public:
    SetRefrParentCellCommand(Data* data, quint32 formId, quint32 oldCellId, quint32 newCellId)
        : mData(data), mFormId(formId), mOldCellId(oldCellId), mNewCellId(newCellId)
    {
    }

    void execute() override
    {
        if (mData) mData->setRefrParentCell(mFormId, mNewCellId);
    }

    void undo() override
    {
        if (mData) mData->setRefrParentCell(mFormId, mOldCellId);
    }

    QString name() const override { return QStringLiteral("Change cell reference parent"); }

private:
    Data* mData;
    quint32 mFormId;
    quint32 mOldCellId;
    quint32 mNewCellId;
};

#endif // SETREFRPARENTCELLCOMMAND_H
