#ifndef SETINFOPARENTDIALCOMMAND_H
#define SETINFOPARENTDIALCOMMAND_H

#include "command.hpp"

class Data;

class SetInfoParentDialCommand : public Command
{
public:
    SetInfoParentDialCommand(Data* data, quint32 infoId, quint32 oldDialId, quint32 newDialId)
        : mData(data), mInfoId(infoId), mOldDialId(oldDialId), mNewDialId(newDialId)
    {
    }

    void execute() override
    {
        if (mData) mData->setInfoParentDial(mInfoId, mNewDialId);
    }

    void undo() override
    {
        if (mData) mData->setInfoParentDial(mInfoId, mOldDialId);
    }

    QString name() const override { return QStringLiteral("Change dialogue parent"); }

private:
    Data* mData;
    quint32 mInfoId;
    quint32 mOldDialId;
    quint32 mNewDialId;
};

#endif // SETINFOPARENTDIALCOMMAND_H
