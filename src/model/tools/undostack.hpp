#ifndef UNDOSTACK_H
#define UNDOSTACK_H

#include "command.hpp"

#include <QVector>
#include <QString>

class UndoStack
{
public:
    UndoStack(int maxDepth = 100);

    void push(Command* command);
    void undo();
    void redo();

    bool canUndo() const;
    bool canRedo() const;
    int undoCount() const;
    int redoCount() const;
    
    QString currentDescription() const;
    QString lastDescription() const;

    void setMaxDepth(int maxDepth);
    int getMaxDepth() const;

    void clear();
    ~UndoStack();

private:
    QVector<Command*> mHistory;
    int mPosition;
    int mMaxDepth;
};

#endif // UNDOSTACK_H
