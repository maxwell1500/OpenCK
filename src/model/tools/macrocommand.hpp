#ifndef MACROCOMMAND_H
#define MACROCOMMAND_H

#include "command.hpp"

#include <QVector>
#include <QString>

class MacroCommand : public Command
{
public:
    explicit MacroCommand(const QString& description = "Macro operation");
    ~MacroCommand() override;

    void addCommand(Command* command);

    void execute() override;
    void undo() override;
    QString name() const override;

private:
    QString mName;
    QVector<Command*> mCommands;
};

#endif // MACROCOMMAND_H
