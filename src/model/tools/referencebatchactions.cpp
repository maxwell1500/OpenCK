#include "referencebatchactions.hpp"

void ReferenceBatchActions::moveByOffset(QVector<CellRefEntry>& refs, float dx, float dy, float dz)
{
    for (CellRefEntry& ref : refs)
    {
        ref.posX += dx;
        ref.posY += dy;
        ref.posZ += dz;
    }
}

void ReferenceBatchActions::snapToGrid(QVector<CellRefEntry>& refs, float gridSize)
{
    if (gridSize <= 0.0f) return;
    for (CellRefEntry& ref : refs)
    {
        ref.posX = static_cast<float>(qRound(ref.posX / gridSize)) * gridSize;
        ref.posY = static_cast<float>(qRound(ref.posY / gridSize)) * gridSize;
        ref.posZ = static_cast<float>(qRound(ref.posZ / gridSize)) * gridSize;
    }
}

void ReferenceBatchActions::setScale(QVector<CellRefEntry>& refs, float scale)
{
    for (CellRefEntry& ref : refs)
    {
        ref.scale = scale;
    }
}

void ReferenceBatchActions::setFlag(QVector<CellRefEntry>& refs, Flag flag, bool on)
{
    const bool disabled = (flag == Flag::Disabled);
    const bool hidden = (flag == Flag::Hidden);
    for (CellRefEntry& ref : refs)
    {
        if (disabled) ref.setDisabled(on);
        if (hidden) ref.setHidden(on);
    }
}

void ReferenceBatchActions::resetRotation(QVector<CellRefEntry>& refs)
{
    for (CellRefEntry& ref : refs)
    {
        ref.rotX = 0.0f;
        ref.rotY = 0.0f;
        ref.rotZ = 0.0f;
    }
}
