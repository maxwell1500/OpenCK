#ifndef ADDRECORDCOMMAND_H
#define ADDRECORDCOMMAND_H

#include "command.hpp"
#include "../world/basecollection.hpp"
#include "../world/idtable.hpp"
#include "../world/record.hpp"

#include <memory>

class AddRecordCommand : public Command
{
public:
    AddRecordCommand(IdTable* model, BaseCollection* collection, int index, const BaseRecord& record, const QString& description = QString());

    void execute() override;
    void undo() override;
    QString name() const override;

private:
    IdTable* mModel;
    BaseCollection* mCollection;
    int mIndex;
    std::unique_ptr<BaseRecord> mRecord;
    QString mDescription;
};

#endif // ADDRECORDCOMMAND_H
