#ifndef TREELODGENERATOR_HPP
#define TREELODGENERATOR_HPP

#include <QString>
#include <QImage>
#include <QVector>
#include <QRectF>

#include "nifparser.hpp"

class Data;

class TreeLodGenerator
{
public:
    struct TreeLodResult
    {
        bool success = false;
        QString error;
        int treesProcessed = 0;
        QImage atlasImage;
        QVector<QRectF> uvRects;
        QVector<QString> treeIds;
    };

    static QImage generateBillboard(const QString& treeNifPath, int resolution = 256);

    static TreeLodResult generateTreeLodAtlas(const Data& data,
                                               const QString& dataDir,
                                               const QString& outputDir,
                                               int atlasResolution = 2048);

    static bool createBillboardNif(const QString& treeNifPath,
                                    const QString& billboardTexturePath,
                                    const QString& outputPath);

private:
    static bool loadTreeModel(const QString& nifPath,
                               QVector<Nif::TriShape>& shapes,
                               Nif::Vector3& boundsMin,
                               Nif::Vector3& boundsMax);

    static QImage renderTreeToImage(const QVector<Nif::TriShape>& shapes,
                                     const Nif::Vector3& boundsMin,
                                     const Nif::Vector3& boundsMax,
                                     int resolution);
};

#endif // TREELODGENERATOR_HPP
