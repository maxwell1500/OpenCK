#ifndef SETCOLUMNCOMMAND_H
#define SETCOLUMNCOMMAND_H

#include "command.hpp"
#include "../world/basecollection.hpp"
#include "../world/idtable.hpp"

#include <QVariant>

class SetColumnCommand : public Command
{
public:
    SetColumnCommand(IdTable* model, BaseCollection* collection, int recordIndex, int column, const QVariant& newValue, const QString& description = QString());

    void execute() override;
    void undo() override;
    QString name() const override;

private:
    void notifyModel();

    IdTable* mModel;
    BaseCollection* mCollection;
    int mRecordIndex;
    int mColumn;
    QVariant mOldValue;
    QVariant mNewValue;
    QString mDescription;
};

#endif // SETCOLUMNCOMMAND_H
