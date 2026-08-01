#ifndef REFERENCEBATCHACTIONS_HPP
#define REFERENCEBATCHACTIONS_HPP

#include "../../../libs/files/esm/cellreferencedata.hpp"

#include <QVector>
#include <QPointF>

// Batch operations that can be applied to multiple cell references at once,
// mirroring the real Creation Kit's "reference batch action" window. Each
// operation is a pure function over the reference list so it can be unit
// tested and used from both the Cell View and the Object Window.
struct ReferenceBatchActions
{
    enum class Flag { Disabled, Hidden };

    // Moves every reference by the given world-space offset.
    static void moveByOffset(QVector<CellRefEntry>& refs, float dx, float dy, float dz);

    // Snaps the X/Y position of every reference to the given grid size.
    static void snapToGrid(QVector<CellRefEntry>& refs, float gridSize);

    // Sets the uniform scale of every reference.
    static void setScale(QVector<CellRefEntry>& refs, float scale);

    // Sets or clears a flag on every reference.
    static void setFlag(QVector<CellRefEntry>& refs, Flag flag, bool on);

    // Zeros the rotation of every reference.
    static void resetRotation(QVector<CellRefEntry>& refs);
};

#endif // REFERENCEBATCHACTIONS_HPP
