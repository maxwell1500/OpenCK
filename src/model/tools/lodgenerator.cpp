#include "lodgenerator.hpp"
#include "logger.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFileInfoList>
#include <algorithm>
#include <cmath>
#include <set>
#include <map>
#include <queue>
#include <cstring>
#include <functional>

float LodGenerator::edgeLength(const Nif::Vector3& a, const Nif::Vector3& b)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float dz = b.z - a.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

float LodGenerator::dotProduct(const Nif::Vector3& a, const Nif::Vector3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float LodGenerator::vecLength(const Nif::Vector3& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

Nif::Vector3 LodGenerator::vecSub(const Nif::Vector3& a, const Nif::Vector3& b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Nif::Vector3 LodGenerator::vecAdd(const Nif::Vector3& a, const Nif::Vector3& b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

Nif::Vector3 LodGenerator::vecScale(const Nif::Vector3& v, float s)
{
    return {v.x * s, v.y * s, v.z * s};
}

Nif::Vector3 LodGenerator::vecNormalize(const Nif::Vector3& v)
{
    float len = vecLength(v);
    if (len < 1e-8f) return {0, 0, 1};
    return {v.x / len, v.y / len, v.z / len};
}

Nif::Vector3 LodGenerator::computeFaceNormal(const Nif::Vector3& a, const Nif::Vector3& b, const Nif::Vector3& c)
{
    Nif::Vector3 u = vecSub(b, a);
    Nif::Vector3 v = vecSub(c, a);
    return {
        u.y * v.z - u.z * v.y,
        u.z * v.x - u.x * v.z,
        u.x * v.y - u.y * v.x
    };
}

float LodGenerator::computeVertexImportance(int vertexIndex,
                                             const QVector<Nif::Vector3>& vertices,
                                             const QVector<Nif::Vector3>& normals,
                                             const QVector<unsigned int>& indices)
{
    float avgEdgeLength = 0.0f;
    int edgeCount = 0;
    float normalDeviation = 0.0f;
    int normalCount = 0;

    for (int i = 0; i < indices.size(); i += 3)
    {
        int i0 = static_cast<int>(indices[i]);
        int i1 = static_cast<int>(indices[i + 1]);
        int i2 = static_cast<int>(indices[i + 2]);

        if (i0 == vertexIndex || i1 == vertexIndex || i2 == vertexIndex)
        {
            int otherA = (i0 == vertexIndex) ? i1 : i0;
            int otherB = (i0 == vertexIndex || i1 == vertexIndex) ? i2 : i1;

            if (otherA < vertices.size() && otherB < vertices.size())
            {
                avgEdgeLength += edgeLength(vertices[vertexIndex], vertices[otherA]);
                avgEdgeLength += edgeLength(vertices[vertexIndex], vertices[otherB]);
                edgeCount += 2;
            }

            if (!normals.isEmpty() &&
                vertexIndex < normals.size() &&
                otherA < normals.size() &&
                otherB < normals.size())
            {
                Nif::Vector3 faceNormal = computeFaceNormal(
                    vertices[i0], vertices[i1], vertices[i2]);
                faceNormal = vecNormalize(faceNormal);

                float deviation = 1.0f - std::abs(dotProduct(normals[vertexIndex], faceNormal));
                normalDeviation += deviation;
                normalCount++;
            }
        }
    }

    if (edgeCount == 0) return 0.0f;

    avgEdgeLength /= static_cast<float>(edgeCount);
    float normalFactor = (normalCount > 0) ? (normalDeviation / static_cast<float>(normalCount)) : 0.0f;

    float importance = avgEdgeLength * (1.0f + normalFactor * 2.0f);
    return importance;
}

struct Quadric {
    float data[16];

    Quadric() { std::memset(data, 0, sizeof(data)); }

    void addFace(float a, float b, float c, float d) {
        float v[4] = {a, b, c, d};
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                data[i * 4 + j] += v[i] * v[j];
    }

    float error(float x, float y, float z) const {
        float v[4] = {x, y, z, 1.0f};
        float result = 0.0f;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                result += v[i] * data[i * 4 + j] * v[j];
        return result;
    }

    Quadric operator+(const Quadric& other) const {
        Quadric r;
        for (int i = 0; i < 16; i++)
            r.data[i] = data[i] + other.data[i];
        return r;
    }
};

struct EdgeCollapse {
    int u, v;
    float cost;
    Nif::Vector3 optimalPos;

    bool operator>(const EdgeCollapse& o) const { return cost > o.cost; }
};

static void computeOptimalPosition(const Quadric& q, const Nif::Vector3& posU, const Nif::Vector3& posV, Nif::Vector3& outPos)
{
    Quadric sum = q;
    float det = sum.data[0] * (sum.data[5] * sum.data[10] - sum.data[6] * sum.data[9])
              - sum.data[1] * (sum.data[4] * sum.data[10] - sum.data[6] * sum.data[8])
              + sum.data[2] * (sum.data[4] * sum.data[9] - sum.data[5] * sum.data[8]);

    if (std::abs(det) > 1e-6f) {
        float invDet = 1.0f / det;
        float a11 = (sum.data[5] * sum.data[10] - sum.data[6] * sum.data[9]) * invDet;
        float a12 = (sum.data[2] * sum.data[9] - sum.data[1] * sum.data[10]) * invDet;
        float a13 = (sum.data[1] * sum.data[6] - sum.data[2] * sum.data[5]) * invDet;
        float a21 = (sum.data[6] * sum.data[8] - sum.data[4] * sum.data[10]) * invDet;
        float a22 = (sum.data[0] * sum.data[10] - sum.data[2] * sum.data[8]) * invDet;
        float a23 = (sum.data[2] * sum.data[4] - sum.data[0] * sum.data[6]) * invDet;
        float a31 = (sum.data[4] * sum.data[9] - sum.data[5] * sum.data[8]) * invDet;
        float a32 = (sum.data[1] * sum.data[8] - sum.data[0] * sum.data[9]) * invDet;
        float a33 = (sum.data[0] * sum.data[5] - sum.data[1] * sum.data[4]) * invDet;

        float b0 = sum.data[3];
        float b1 = sum.data[7];
        float b2 = sum.data[11];

        outPos.x = -(a11 * b0 + a12 * b1 + a13 * b2);
        outPos.y = -(a21 * b0 + a22 * b1 + a23 * b2);
        outPos.z = -(a31 * b0 + a32 * b1 + a33 * b2);

        float maxDelta = 100.0f;
        float dx = outPos.x - (posU.x + posV.x) * 0.5f;
        float dy = outPos.y - (posU.y + posV.y) * 0.5f;
        float dz = outPos.z - (posU.z + posV.z) * 0.5f;
        if (dx * dx + dy * dy + dz * dz > maxDelta * maxDelta) {
            outPos.x = (posU.x + posV.x) * 0.5f;
            outPos.y = (posU.y + posV.y) * 0.5f;
            outPos.z = (posU.z + posV.z) * 0.5f;
        }
    } else {
        outPos.x = (posU.x + posV.x) * 0.5f;
        outPos.y = (posU.y + posV.y) * 0.5f;
        outPos.z = (posU.z + posV.z) * 0.5f;
    }
}

void LodGenerator::decimateVertices(QVector<Nif::Vector3>& vertices,
                                     QVector<Nif::Vector2>& uvs,
                                     QVector<Nif::Vector3>& normals,
                                     QVector<unsigned int>& indices,
                                     float reductionPercent,
                                     bool preserveUVs,
                                     bool preserveNormals)
{
    if (vertices.size() < 4 || indices.size() < 6)
        return;

    int targetVertexCount = std::max(4, static_cast<int>(vertices.size() * reductionPercent));
    if (static_cast<int>(vertices.size()) <= targetVertexCount)
        return;

    QVector<Quadric> vertexQuadrics(vertices.size());

    for (int i = 0; i < indices.size(); i += 3) {
        int i0 = static_cast<int>(indices[i]);
        int i1 = static_cast<int>(indices[i + 1]);
        int i2 = static_cast<int>(indices[i + 2]);

        const Nif::Vector3& v0 = vertices[i0];
        const Nif::Vector3& v1 = vertices[i1];
        const Nif::Vector3& v2 = vertices[i2];

        float ex1 = v1.x - v0.x, ey1 = v1.y - v0.y, ez1 = v1.z - v0.z;
        float ex2 = v2.x - v0.x, ey2 = v2.y - v0.y, ez2 = v2.z - v0.z;
        float nx = ey1 * ez2 - ez1 * ey2;
        float ny = ez1 * ex2 - ex1 * ez2;
        float nz = ex1 * ey2 - ey1 * ex2;
        float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (len < 1e-10f) continue;
        nx /= len; ny /= len; nz /= len;

        float d = -(nx * v0.x + ny * v0.y + nz * v0.z);

        Quadric q;
        q.addFace(nx, ny, nz, d);

        vertexQuadrics[i0] = vertexQuadrics[i0] + q;
        vertexQuadrics[i1] = vertexQuadrics[i1] + q;
        vertexQuadrics[i2] = vertexQuadrics[i2] + q;
    }

    QVector<bool> vertexRemoved(vertices.size(), false);

    std::set<std::pair<int, int>> edgeSet;
    for (int i = 0; i < indices.size(); i += 3) {
        int i0 = static_cast<int>(indices[i]);
        int i1 = static_cast<int>(indices[i + 1]);
        int i2 = static_cast<int>(indices[i + 2]);
        auto addEdge = [&](int a, int b) {
            if (a > b) std::swap(a, b);
            edgeSet.insert({a, b});
        };
        addEdge(i0, i1);
        addEdge(i1, i2);
        addEdge(i0, i2);
    }

    std::set<std::pair<int, int>> boundaryEdges;
    std::map<std::pair<int, int>, int> edgeTriCount;
    for (int i = 0; i < indices.size(); i += 3) {
        int i0 = static_cast<int>(indices[i]);
        int i1 = static_cast<int>(indices[i + 1]);
        int i2 = static_cast<int>(indices[i + 2]);
        auto edgeKey = [](int a, int b) -> std::pair<int, int> {
            if (a > b) std::swap(a, b);
            return {a, b};
        };
        edgeTriCount[edgeKey(i0, i1)]++;
        edgeTriCount[edgeKey(i1, i2)]++;
        edgeTriCount[edgeKey(i0, i2)]++;
    }
    for (auto& kv : edgeTriCount) {
        if (kv.second == 1)
            boundaryEdges.insert(kv.first);
    }

    float boundaryWeight = 100.0f;
    for (auto& be : boundaryEdges) {
        int a = be.first, b = be.second;
        Nif::Vector3 edge = vecSub(vertices[b], vertices[a]);
        float edgeLen = vecLength(edge);
        if (edgeLen < 1e-8f) continue;
        Nif::Vector3 edgeNorm = vecNormalize(edge);
        float ex = edgeNorm.x, ey = edgeNorm.y, ez = edgeNorm.z;
        float px = vertices[a].x, py = vertices[a].y, pz = vertices[a].z;
        Nif::Vector3 up = {0, 0, 1};
        float dotUp = edgeNorm.x * up.x + edgeNorm.y * up.y + edgeNorm.z * up.z;
        Nif::Vector3 perp = {up.x - dotUp * edgeNorm.x, up.y - dotUp * edgeNorm.y, up.z - dotUp * edgeNorm.z};
        float perpLen = std::sqrt(perp.x * perp.x + perp.y * perp.y + perp.z * perp.z);
        if (perpLen < 1e-6f) {
            up = {1, 0, 0};
            dotUp = edgeNorm.x * up.x + edgeNorm.y * up.y + edgeNorm.z * up.z;
            perp = {up.x - dotUp * edgeNorm.x, up.y - dotUp * edgeNorm.y, up.z - dotUp * edgeNorm.z};
            perpLen = std::sqrt(perp.x * perp.x + perp.y * perp.y + perp.z * perp.z);
        }
        Nif::Vector3 faceNorm = {perp.x / perpLen, perp.y / perpLen, perp.z / perpLen};
        float nx = faceNorm.x, ny = faceNorm.y, nz = faceNorm.z;
        float bd = -(nx * px + ny * py + nz * pz);
        Quadric bq;
        bq.addFace(nx * boundaryWeight, ny * boundaryWeight, nz * boundaryWeight, bd * boundaryWeight);
        vertexQuadrics[a] = vertexQuadrics[a] + bq;
        vertexQuadrics[b] = vertexQuadrics[b] + bq;
    }

    std::priority_queue<EdgeCollapse, std::vector<EdgeCollapse>, std::greater<EdgeCollapse>> collapseQueue;

    auto computeEdgeCost = [&](int u, int v, EdgeCollapse& ec) {
        if (vertexRemoved[u] || vertexRemoved[v]) { ec.cost = 1e30f; return; }
        Quadric qSum = vertexQuadrics[u] + vertexQuadrics[v];
        Nif::Vector3 optPos;
        computeOptimalPosition(qSum, vertices[u], vertices[v], optPos);
        float cost = qSum.error(optPos.x, optPos.y, optPos.z);
        ec.u = u;
        ec.v = v;
        ec.cost = cost;
        ec.optimalPos = optPos;
    };

    for (auto& edge : edgeSet) {
        EdgeCollapse ec;
        computeEdgeCost(edge.first, edge.second, ec);
        collapseQueue.push(ec);
    }

    std::map<std::pair<int, int>, bool> collapsedEdges;

    int currentVertexCount = static_cast<int>(vertices.size());
    while (currentVertexCount > targetVertexCount && !collapseQueue.empty()) {
        EdgeCollapse ec = collapseQueue.top();
        collapseQueue.pop();

        if (vertexRemoved[ec.u] || vertexRemoved[ec.v]) continue;
        if (ec.u == ec.v) continue;

        int u = ec.u, v = ec.v;
        if (u > v) std::swap(u, v);
        if (collapsedEdges.count({u, v})) continue;
        collapsedEdges[{u, v}] = true;

        Quadric qSum = vertexQuadrics[ec.u] + vertexQuadrics[ec.v];
        float recheckCost = qSum.error(ec.optimalPos.x, ec.optimalPos.y, ec.optimalPos.z);
        float bestRecheck = recheckCost;
        Nif::Vector3 bestPos = ec.optimalPos;
        Nif::Vector3 avgPos;
        avgPos.x = (vertices[ec.u].x + vertices[ec.v].x) * 0.5f;
        avgPos.y = (vertices[ec.u].y + vertices[ec.v].y) * 0.5f;
        avgPos.z = (vertices[ec.u].z + vertices[ec.v].z) * 0.5f;
        float avgCost = qSum.error(avgPos.x, avgPos.y, avgPos.z);
        if (avgCost < bestRecheck) {
            bestRecheck = avgCost;
            bestPos = avgPos;
        }
        if (bestRecheck > 1e20f) continue;

        int removeIdx = ec.u;
        int keepIdx = ec.v;

        Nif::Vector2 uvKeep = (keepIdx < uvs.size()) ? uvs[keepIdx] : Nif::Vector2{0, 0};
        Nif::Vector2 uvRemove = (removeIdx < uvs.size()) ? uvs[removeIdx] : Nif::Vector2{0, 0};
        Nif::Vector2 newUV;
        newUV.u = (uvKeep.u + uvRemove.u) * 0.5f;
        newUV.v = (uvKeep.v + uvRemove.v) * 0.5f;

        for (int i = 0; i < indices.size(); i++) {
            if (static_cast<int>(indices[i]) == removeIdx)
                indices[i] = static_cast<unsigned int>(keepIdx);
        }

        vertices[keepIdx] = bestPos;
        if (preserveUVs && keepIdx < uvs.size())
            uvs[keepIdx] = newUV;
        vertexQuadrics[keepIdx] = qSum;

        vertexRemoved[removeIdx] = true;
        currentVertexCount--;

        QVector<int> triToCheck;
        for (int i = 0; i < indices.size(); i += 3) {
            int i0 = static_cast<int>(indices[i]);
            int i1 = static_cast<int>(indices[i + 1]);
            int i2 = static_cast<int>(indices[i + 2]);
            if (i0 == removeIdx || i1 == removeIdx || i2 == removeIdx)
                triToCheck.append(i / 3);
        }

        for (int ti : triToCheck) {
            int i0 = static_cast<int>(indices[ti * 3]);
            int i1 = static_cast<int>(indices[ti * 3 + 1]);
            int i2 = static_cast<int>(indices[ti * 3 + 2]);
            if (i0 == i1 || i1 == i2 || i0 == i2) {
                indices[ti * 3] = 0;
                indices[ti * 3 + 1] = 0;
                indices[ti * 3 + 2] = 0;
            }
        }
    }

    QVector<int> newIndexMap(vertices.size(), -1);
    QVector<Nif::Vector3> newVertices;
    QVector<Nif::Vector2> newUvs;
    QVector<Nif::Vector3> newNormals;

    for (int i = 0; i < vertices.size(); i++) {
        if (!vertexRemoved[i]) {
            newIndexMap[i] = newVertices.size();
            newVertices.append(vertices[i]);
            if (preserveUVs && i < uvs.size())
                newUvs.append(uvs[i]);
            if (preserveNormals && i < normals.size())
                newNormals.append(normals[i]);
        }
    }

    QVector<unsigned int> newIndices;
    for (int i = 0; i < indices.size(); i += 3) {
        int i0 = static_cast<int>(indices[i]);
        int i1 = static_cast<int>(indices[i + 1]);
        int i2 = static_cast<int>(indices[i + 2]);

        if (i0 == 0 && i1 == 0 && i2 == 0)
            continue;

        if (i0 >= newIndexMap.size() || i1 >= newIndexMap.size() || i2 >= newIndexMap.size())
            continue;

        int ni0 = newIndexMap[i0];
        int ni1 = newIndexMap[i1];
        int ni2 = newIndexMap[i2];

        if (ni0 < 0 || ni1 < 0 || ni2 < 0)
            continue;
        if (ni0 == ni1 || ni1 == ni2 || ni0 == ni2)
            continue;

        newIndices.append(static_cast<unsigned int>(ni0));
        newIndices.append(static_cast<unsigned int>(ni1));
        newIndices.append(static_cast<unsigned int>(ni2));
    }

    vertices = newVertices;
    uvs = newUvs;
    normals = newNormals;
    indices = newIndices;
}

LodGenerator::LodResult LodGenerator::simplifyMesh(Nif::NifParser& nif, const LodOptions& options)
{
    LodResult result;
    Nif::Node* root = nif.getRoot();
    if (!root)
    {
        result.error = "No root node in NIF";
        return result;
    }

    int totalOriginal = 0;
    int totalSimplified = 0;
    int meshesProcessed = 0;

    QVector<Nif::TriShape*> allShapes;
    QVector<Nif::Node*> nodeList;
    std::function<void(Nif::Node*)> collectNodes = [&](Nif::Node* node) {
        if (!node) return;
        nodeList.append(node);
        for (auto& shape : node->shapes)
            allShapes.append(&shape);
        for (auto* child : node->children)
            collectNodes(child);
    };
    collectNodes(root);

    for (auto* shapePtr : allShapes)
    {
        Nif::TriShape& shape = *shapePtr;
        if (shape.vertices.size() < 4 || shape.indices.size() < 6)
            continue;

        int originalCount = shape.vertices.size();
        totalOriginal += originalCount;

        decimateVertices(shape.vertices, shape.uvs, shape.normals, shape.indices,
                         options.reductionPercent, options.preserveUVs, options.preserveNormals);

        totalSimplified += shape.vertices.size();
        meshesProcessed++;

        shape.recalculateNormals();
    }

    result.success = true;
    result.originalVertices = totalOriginal;
    result.simplifiedVertices = totalSimplified;
    result.lodLevelsGenerated = 1;

    LOG_INFO(QString("LOD simplification: %1 meshes, %2 -> %3 vertices")
             .arg(meshesProcessed).arg(totalOriginal).arg(totalSimplified));

    return result;
}

LodGenerator::LodResult LodGenerator::generateLodLevels(Nif::NifParser& nif, const QString& filePath, const LodOptions& options)
{
    LodResult result;
    Nif::Node* root = nif.getRoot();
    if (!root)
    {
        result.error = "No root node in NIF";
        return result;
    }

    int totalOriginal = 0;
    int totalSimplified = 0;
    int levelsGenerated = 0;

    QVector<Nif::Node*> nodeList;
    std::function<void(Nif::Node*)> collectNodes = [&](Nif::Node* node) {
        if (!node) return;
        nodeList.append(node);
        for (auto* child : node->children)
            collectNodes(child);
    };
    collectNodes(root);

    for (Nif::Node* node : nodeList)
    {
        if (node->shapes.isEmpty())
            continue;

        QVector<Nif::TriShape> originalShapes = node->shapes;

        for (const auto& s : originalShapes)
            totalOriginal += s.vertices.size();

        for (int lodLevel = 1; lodLevel < options.targetLodLevels; lodLevel++)
        {
            float levelReduction = std::pow(options.reductionPercent, static_cast<float>(lodLevel));

            Nif::NifParser lodNif;
            Nif::Node* lodRoot = new Nif::Node();
            lodRoot->name = root->name;
            lodRoot->position = root->position;
            lodRoot->rotation = root->rotation;
            lodNif.setRoot(lodRoot);

            for (const auto& origShape : originalShapes)
            {
                if (origShape.vertices.size() < 4 || origShape.indices.size() < 6)
                    continue;

                Nif::TriShape lodShape = origShape;
                lodShape.name = QString("%1_LOD%2").arg(origShape.name).arg(lodLevel);

                decimateVertices(lodShape.vertices, lodShape.uvs, lodShape.normals, lodShape.indices,
                                 levelReduction, options.preserveUVs, options.preserveNormals);

                lodShape.recalculateNormals();

                totalSimplified += lodShape.vertices.size();
                lodRoot->shapes.append(lodShape);
            }

            QFileInfo fi(filePath);
            QString baseName = fi.completeBaseName();
            QString dirPath = fi.path();
            QString lodFileName = QString("%1/%2_lod%3.nif").arg(dirPath).arg(baseName).arg(lodLevel);

            if (lodNif.save(lodFileName))
            {
                levelsGenerated++;
                LOG_INFO(QString("LOD generation: saved %1")
                         .arg(QFileInfo(lodFileName).fileName()));
            }
            else
            {
                LOG_WARNING(QString("LOD generation: failed to save %1")
                            .arg(lodFileName));
            }
        }
    }

    result.success = true;
    result.originalVertices = totalOriginal;
    result.simplifiedVertices = totalSimplified;
    result.lodLevelsGenerated = levelsGenerated;

    LOG_INFO(QString("LOD generation: %1 LOD files generated, total vertices: %2 -> %3")
             .arg(levelsGenerated).arg(totalOriginal).arg(totalSimplified));

    return result;
}

int LodGenerator::batchGenerateLod(const QString& dataDir, const QStringList& nifPaths, const LodOptions& options)
{
    QDir dir(dataDir);
    if (!dir.exists())
    {
        LOG_ERROR(QString("LOD batch: data directory does not exist: %1").arg(dataDir));
        return 0;
    }

    QStringList filesToProcess = nifPaths;

    if (filesToProcess.isEmpty())
    {
        QDirIterator it(dataDir, {"*.nif"}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext())
        {
            QString relativePath = dir.relativeFilePath(it.next());
            filesToProcess.append(relativePath);
        }
    }

    int processed = 0;

    for (const QString& relPath : filesToProcess)
    {
        QString fullPath = dir.filePath(relPath);
        QFileInfo fi(fullPath);
        if (!fi.exists() || fi.suffix().toLower() != "nif")
            continue;

        Nif::NifParser parser;
        if (!parser.load(fullPath))
        {
            LOG_WARNING(QString("LOD batch: failed to load %1").arg(relPath));
            continue;
        }

        LodResult result = generateLodLevels(parser, fullPath, options);
        if (result.success)
        {
            processed++;
            LOG_INFO(QString("LOD batch: processed %1 (%2 LOD files generated)")
                     .arg(relPath).arg(result.lodLevelsGenerated));
        }
        else
        {
            LOG_WARNING(QString("LOD batch: failed %1 - %2").arg(relPath).arg(result.error));
        }
    }

    LOG_INFO(QString("LOD batch complete: %1 files processed").arg(processed));
    return processed;
}
