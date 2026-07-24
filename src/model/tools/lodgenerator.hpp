#ifndef LODGENERATOR_HPP
#define LODGENERATOR_HPP

#include <QString>
#include <QStringList>
#include <QVector>

#include "nifparser.hpp"

class LodGenerator
{
public:
    struct LodOptions
    {
        float reductionPercent = 0.5f;
        bool preserveUVs = true;
        bool preserveNormals = true;
        int targetLodLevels = 3;
    };

    struct LodResult
    {
        bool success = false;
        QString error;
        int originalVertices = 0;
        int simplifiedVertices = 0;
        int lodLevelsGenerated = 0;
    };

    static LodResult simplifyMesh(Nif::NifParser& nif, const LodOptions& options);
    static LodResult generateLodLevels(Nif::NifParser& nif, const QString& filePath, const LodOptions& options);
    static int batchGenerateLod(const QString& dataDir, const QStringList& nifPaths, const LodOptions& options);

    static void decimateVertices(QVector<Nif::Vector3>& vertices,
                                 QVector<Nif::Vector2>& uvs,
                                 QVector<Nif::Vector3>& normals,
                                 QVector<unsigned int>& indices,
                                 float reductionPercent,
                                 bool preserveUVs,
                                 bool preserveNormals);

private:
    struct VertexInfo
    {
        int index;
        float importance;
    };

    static float computeVertexImportance(int vertexIndex,
                                         const QVector<Nif::Vector3>& vertices,
                                         const QVector<Nif::Vector3>& normals,
                                         const QVector<unsigned int>& indices);
    static float edgeLength(const Nif::Vector3& a, const Nif::Vector3& b);
    static float dotProduct(const Nif::Vector3& a, const Nif::Vector3& b);
    static float vecLength(const Nif::Vector3& v);
    static Nif::Vector3 vecSub(const Nif::Vector3& a, const Nif::Vector3& b);
    static Nif::Vector3 vecAdd(const Nif::Vector3& a, const Nif::Vector3& b);
    static Nif::Vector3 vecScale(const Nif::Vector3& v, float s);
    static Nif::Vector3 vecNormalize(const Nif::Vector3& v);
    static Nif::Vector3 computeFaceNormal(const Nif::Vector3& a, const Nif::Vector3& b, const Nif::Vector3& c);
};

#endif // LODGENERATOR_HPP
