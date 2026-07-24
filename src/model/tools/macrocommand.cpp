#include "macrocommand.hpp"

MacroCommand::MacroCommand(const QString& description)
    : mName(description)
{
}

MacroCommand::~MacroCommand()
{
    for (auto cmd : mCommands)
    {
        delete cmd;
    }
}

void MacroCommand::addCommand(Command* command)
{
    mCommands.append(command);
}

void MacroCommand::execute()
{
    for (auto cmd : mCommands)
    {
        cmd->execute();
    }
}

void MacroCommand::undo()
{
    for (int i = mCommands.size() - 1; i >= 0; i--)
    {
        mCommands[i]->undo();
    }
}

QString MacroCommand::name() const
{
    return mName;
}
