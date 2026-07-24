#ifndef COMMAND_H
#define COMMAND_H

#include <QString>

class Command
{
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual QString name() const = 0;
};

#endif // COMMAND_H
