#include "brushtool.hpp"

BrushTool::BrushTool(QObject* parent) :
    QObject(parent),
    mActive(false)
{
}

void BrushTool::beginStroke()
{
    mActive = true;
}

void BrushTool::endStroke()
{
    if (mActive) {
        emit strokeFinished();
        mActive = false;
    }
}

void BrushTool::notifyStrokeApplied()
{
    emit strokeApplied();
}