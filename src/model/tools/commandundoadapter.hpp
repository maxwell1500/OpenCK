#ifndef COMMANDUNDOADAPTER_HPP
#define COMMANDUNDOADAPTER_HPP

#include <QUndoCommand>
#include "command.hpp"

class CommandUndoAdapter : public QUndoCommand
{
public:
    explicit CommandUndoAdapter(Command* cmd)
        : QUndoCommand(cmd->name()), m_cmd(cmd) {}
    ~CommandUndoAdapter() override { delete m_cmd; }

    void redo() override { m_cmd->execute(); }
    void undo() override { m_cmd->undo(); }

private:
    Command* m_cmd;
};

#endif // COMMANDUNDOADAPTER_HPP
