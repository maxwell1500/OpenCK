#ifndef BTDFILE_H
#define BTDFILE_H

#include <QString>
#include <QVector>
#include <QJsonObject>

// BtdFile models the BTD land-texture file format (as reverse-engineered by
// the Morrowind project's generate_btd*.py). A BTD file stores the per-quad
// texture assignments for one landscape cell: a grid of texture indices plus
// per-texture material names. OpenCK uses this model to generate BTD files
// from the landscape editor's texture-layer table and to read them back.
struct BtdFile
{
    QString fileName;
    int gridSize = 0;                // quads per side (usually 32)
    int textureCount = 0;            // number of distinct textures
    QStringList textureNames;        // e.g. "Grass01.dds" (index -> name)
    QVector<quint16> quadIndices;    // gridSize*gridSize texture indices

    // Serializes to JSON (for the editor + tests; the legacy binary layout
    // differs between Morrowind/Skyrim generations and is kept as a follow-up
    // until real samples are available).
    QJsonObject toJson() const;

    // Loads from JSON.
    static BtdFile fromJson(const QJsonObject& obj);

    // Builds a BTD from a per-quad index grid. 'gridSize' is the quads-per-
    // side; 'textureNames' index into quadIndices. Returns true on success.
    static bool build(int gridSize, const QStringList& textureNames,
                      const QVector<quint16>& quadIndices, BtdFile& out);

    // Returns the texture name for a quad, or the index as a fallback.
    QString textureForQuad(int x, int y) const;
};

#endif // BTDFILE_H
