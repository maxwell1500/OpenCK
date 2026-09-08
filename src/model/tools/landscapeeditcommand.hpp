#ifndef LANDSCAPEEDITCOMMAND_H
#define LANDSCAPEEDITCOMMAND_H

#include "command.hpp"
#include <QVector>
#include <QString>
#include <algorithm>

class LandscapeEditCommand : public Command
{
public:
    LandscapeEditCommand(QVector<float>* heightmap, int terrainSize,
                        int x, int y, int width, int height,
                        const QVector<float>& originalData,
                        const QVector<float>& newData)
        : mHeightmap(heightmap), mTerrainSize(terrainSize),
          mX(x), mY(y), mWidth(width), mHeight(height),
          mOriginalData(originalData), mNewData(newData)
    {
        mName = "Landscape edit";
    }

    void execute() override
    {
        if (!mHeightmap || mHeightmap->size() != mTerrainSize * mTerrainSize)
            return;
        copyRegion(mNewData, *mHeightmap);
    }

    void undo() override
    {
        if (!mHeightmap || mHeightmap->size() != mTerrainSize * mTerrainSize)
            return;
        copyRegion(mOriginalData, *mHeightmap);
    }

    QString name() const override
    {
        return mName;
    }

private:
    void copyRegion(const QVector<float>& src, QVector<float>& dst) const
    {
        for (int row = 0; row < mHeight; ++row)
        {
            int srcIdx = row * mWidth;
            int dstIdx = (mY + row) * mTerrainSize + mX;
            std::copy_n(src.constData() + srcIdx, mWidth, dst.data() + dstIdx);
        }
    }

    QVector<float>* mHeightmap;
    int mTerrainSize;
    int mX, mY, mWidth, mHeight;
    QVector<float> mOriginalData;
    QVector<float> mNewData;
    QString mName;
};

#endif // LANDSCAPEEDITCOMMAND_H
