#ifndef DELETERECORDCOMMAND_H
#define DELETERECORDCOMMAND_H

#include "command.hpp"
#include "../world/basecollection.hpp"
#include "../world/idtable.hpp"
#include "../world/record.hpp"

#include <memory>

class DeleteRecordCommand : public Command
{
public:
    DeleteRecordCommand(IdTable* model, BaseCollection* collection, int index, const QString& description = QString());

    void execute() override;
    void undo() override;
    QString name() const override;

private:
    IdTable* mModel;
    BaseCollection* mCollection;
    int mIndex;
    std::unique_ptr<BaseRecord> mDeletedRecord;
    QString mDescription;
};

#endif // DELETERECORDCOMMAND_H
