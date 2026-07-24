#include <QtTest/QtTest>
#include <QTemporaryDir>
#include "model/tools/lodgenerator.hpp"
#include "model/tools/textureatlasgenerator.hpp"

class TestLodGenerator : public QObject {
    Q_OBJECT

private slots:
    void testDecimateSimpleMesh();
    void testDecimatePreservesShape();
    void testDecimateEdgeCases();
    void testRecalculateNormals();
    void testSimplifyMeshBasic();
    void testAtlasGeneration();
    void testAtlasPackFailsGracefully();

private:
    Nif::TriShape createTestTriangle();
    Nif::TriShape createTestQuad();
};

Nif::TriShape TestLodGenerator::createTestTriangle() {
    Nif::TriShape shape;
    shape.vertices = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };
    shape.normals = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}
    };
    shape.uvs = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f}
    };
    shape.indices = {0, 1, 2};
    return shape;
}

Nif::TriShape TestLodGenerator::createTestQuad() {
    Nif::TriShape shape;
    shape.vertices = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };
    shape.normals = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}
    };
    shape.uvs = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };
    shape.indices = {0, 1, 2, 0, 2, 3};
    return shape;
}

void TestLodGenerator::testDecimateSimpleMesh() {
    Nif::TriShape shape = createTestQuad();
    QCOMPARE(shape.vertices.size(), 4);
    QCOMPARE(shape.indices.size(), 6);

    // Decimate with 50% reduction target
    LodGenerator::decimateVertices(shape.vertices, shape.uvs, shape.normals,
                                   shape.indices, 0.5f, false, false);

    // Quad has 4 verts, target is max(4, 4*0.5)=4, so no reduction happens
    // (decimateVertices requires < 4 or < 6 to early-out, and won't reduce below target)
    QCOMPARE(shape.vertices.size(), 4u);
}

void TestLodGenerator::testDecimatePreservesShape() {
    // Triangle (3 verts, 3 indices) - below the minimum threshold (4 verts, 6 indices)
    Nif::TriShape shape = createTestTriangle();
    LodGenerator::decimateVertices(shape.vertices, shape.uvs, shape.normals,
                                   shape.indices, 0.5f, false, false);

    // Should remain unchanged: 3 verts < 4 triggers early return
    QCOMPARE(shape.vertices.size(), 3u);
    QCOMPARE(shape.indices.size(), 3u);
}

void TestLodGenerator::testDecimateEdgeCases() {
    // Empty mesh
    Nif::TriShape empty;
    LodGenerator::decimateVertices(empty.vertices, empty.uvs, empty.normals,
                                   empty.indices, 0.5f, false, false);
    QCOMPARE(empty.vertices.size(), 0u);

    // Mesh with 3 verts, 3 indices (below threshold)
    Nif::TriShape tri = createTestTriangle();
    LodGenerator::decimateVertices(tri.vertices, tri.uvs, tri.normals,
                                   tri.indices, 1.0f, false, false);
    QCOMPARE(tri.vertices.size(), 3u);

    // Mesh with exactly 4 verts, 6 indices (at threshold)
    Nif::TriShape quad = createTestQuad();
    LodGenerator::decimateVertices(quad.vertices, quad.uvs, quad.normals,
                                   quad.indices, 0.0f, false, false);
    // 0% reduction means targetVertexCount = vertices.size(), so no simplification
    QCOMPARE(quad.vertices.size(), 4u);
}

void TestLodGenerator::testRecalculateNormals() {
    Nif::TriShape shape = createTestQuad();
    // Verify initial normals are set
    QCOMPARE(shape.normals.size(), 4u);
    for (const auto& n : shape.normals) {
        QCOMPARE(n.z, 1.0f);
    }

    // Recalculate normals
    shape.recalculateNormals();

    // After recalculation, normals should still be valid and non-zero
    QCOMPARE(shape.normals.size(), shape.vertices.size());
    for (const auto& n : shape.normals) {
        float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
        QVERIFY(len > 0.0f);
    }
}

void TestLodGenerator::testSimplifyMeshBasic() {
    Nif::NifParser nif;
    Nif::Node* root = new Nif::Node();
    root->name = "TestRoot";
    nif.setRoot(root);

    // Add a large mesh (above the 4-vert / 6-index threshold)
    Nif::TriShape largeShape;
    largeShape.name = "LargeMesh";
    // Create a grid of quads (9 verts, 6 faces = 18 indices)
    for (int z = 0; z < 3; ++z) {
        for (int x = 0; x < 3; ++x) {
            largeShape.vertices.append({static_cast<float>(x), 0.0f, static_cast<float>(z)});
            largeShape.normals.append({0.0f, 1.0f, 0.0f});
            largeShape.uvs.append({static_cast<float>(x) / 2.0f, static_cast<float>(z) / 2.0f});
        }
    }
    for (int z = 0; z < 2; ++z) {
        for (int x = 0; x < 2; ++x) {
            unsigned int i = z * 3 + x;
            largeShape.indices.append(i);
            largeShape.indices.append(i + 1);
            largeShape.indices.append(i + 3);
            largeShape.indices.append(i + 1);
            largeShape.indices.append(i + 4);
            largeShape.indices.append(i + 3);
        }
    }
    root->shapes.append(largeShape);

    LodGenerator::LodOptions opts;
    opts.reductionPercent = 0.5f;
    opts.preserveUVs = true;
    opts.preserveNormals = true;
    opts.targetLodLevels = 1;

    auto result = LodGenerator::simplifyMesh(nif, opts);

    QVERIFY(result.success);
    QCOMPARE(result.error, QString());
    QVERIFY(result.originalVertices > 0);
    // Simplified should be <= original (may be same if target equals current)
    QVERIFY(result.simplifiedVertices <= result.originalVertices);
}

void TestLodGenerator::testAtlasGeneration() {
    QVector<QImage> images;
    images.append(QImage(64, 64, QImage::Format_RGBA8888));
    images.append(QImage(32, 32, QImage::Format_RGBA8888));
    images.append(QImage(128, 128, QImage::Format_RGBA8888));

    auto result = TextureAtlasGenerator::generateAtlasFromImages(images, 1024, 2);

    QVERIFY(result.success);
    QVERIFY(result.atlasWidth > 0);
    QVERIFY(result.atlasHeight > 0);
    QCOMPARE(result.entries.size(), 3);

    for (const auto& entry : result.entries) {
        QVERIFY(entry.uvRect.width() > 0);
        QVERIFY(entry.uvRect.height() > 0);
    }
}

void TestLodGenerator::testAtlasPackFailsGracefully() {
    QVector<QImage> images;
    images.append(QImage(256, 256, QImage::Format_RGBA8888));
    images.append(QImage(256, 256, QImage::Format_RGBA8888));

    // Max size smaller than individual images - images are skipped
    auto result = TextureAtlasGenerator::generateAtlasFromImages(images, 128, 2);

    // Should handle gracefully (images too large, so atlas is empty)
    QVERIFY(!result.success);
    QVERIFY(result.entries.isEmpty());
}

QTEST_MAIN(TestLodGenerator)
#include "test_lodgenerator.moc"
