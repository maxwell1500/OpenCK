#ifndef LANDSCAPEEDITCOMMAND_H
#define LANDSCAPEEDITCOMMAND_H

#include "command.hpp"
#include <QVector>
#include <QString>

class LandscapeEditCommand : public Command
{
public:
    LandscapeEditCommand(QVector<float>* heightmap, int terrainSize,
                        const QVector<float>& originalData,
                        const QVector<float>& newData)
        : mHeightmap(heightmap), mTerrainSize(terrainSize),
          mOriginalData(originalData), mNewData(newData)
    {
        mName = "Landscape edit";
    }

    void execute() override
    {
        if (!mHeightmap || mHeightmap->size() != mTerrainSize * mTerrainSize)
            return;
        
        *mHeightmap = mNewData;
    }

    void undo() override
    {
        if (!mHeightmap || mHeightmap->size() != mTerrainSize * mTerrainSize)
            return;
        
        *mHeightmap = mOriginalData;
    }

    QString name() const override
    {
        return mName;
    }

private:
    QVector<float>* mHeightmap;
    int mTerrainSize;
    QVector<float> mOriginalData;
    QVector<float> mNewData;
    QString mName;
};

#endif // LANDSCAPEEDITCOMMAND_H
