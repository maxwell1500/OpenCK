#include "undostack.hpp"

UndoStack::UndoStack(int maxDepth)
    : mPosition(-1), mMaxDepth(maxDepth)
{
}

void UndoStack::push(Command* command)
{
    while (mHistory.size() > mPosition + 1)
    {
        Command* removed = mHistory.takeLast();
        delete removed;
    }

    if (mHistory.size() >= mMaxDepth && mPosition >= 0)
    {
        Command* oldest = mHistory.takeFirst();
        delete oldest;
        mPosition--;
    }

    mHistory.append(command);
    mPosition++;
}

void UndoStack::undo()
{
    if (mPosition < 0)
    {
        return;
    }

    mHistory[mPosition]->undo();
    mPosition--;
}

void UndoStack::redo()
{
    if (mPosition + 1 >= mHistory.size())
    {
        return;
    }

    mPosition++;
    mHistory[mPosition]->execute();
}

bool UndoStack::canUndo() const
{
    return mPosition >= 0;
}

bool UndoStack::canRedo() const
{
    return mPosition + 1 < mHistory.size();
}

int UndoStack::undoCount() const
{
    return mPosition + 1;
}

int UndoStack::redoCount() const
{
    return mHistory.size() - mPosition - 1;
}

QString UndoStack::currentDescription() const
{
    if (mPosition >= 0 && mPosition < mHistory.size())
    {
        return mHistory[mPosition]->name();
    }
    return QString();
}

QString UndoStack::lastDescription() const
{
    if (!mHistory.isEmpty())
    {
        return mHistory.last()->name();
    }
    return QString();
}

void UndoStack::setMaxDepth(int maxDepth)
{
    mMaxDepth = qMax(1, maxDepth);
    
    while (mHistory.size() > mMaxDepth)
    {
        Command* removed = mHistory.takeFirst();
        delete removed;
        if (mPosition > 0) mPosition--;
    }
    
    if (mPosition >= mMaxDepth)
    {
        mPosition = mMaxDepth - 1;
    }
}

int UndoStack::getMaxDepth() const
{
    return mMaxDepth;
}

void UndoStack::clear()
{
    for (auto cmd : mHistory)
    {
        delete cmd;
    }
    mHistory.clear();
    mPosition = -1;
}

UndoStack::~UndoStack()
{
    clear();
}
