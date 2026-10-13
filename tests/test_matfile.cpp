#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QMap>
#include <QCoreApplication>
#include <QFileInfo>

#include "../../libs/files/nif/matfile.hpp"

// .mat files are JSON dumps of a material's component sub-graph. The join that
// matters is "<Family>_Material1" -> "<Family>_TextureSet1": the material object
// only names a TextureSet, and the texture set object is the one that lists the
// texture files. Shader-model entries also carry a FileName but are not
// textures, so the reader must not treat them as one.
class TestMatFile : public QObject
{
    Q_OBJECT

private slots:
    void testTextureSetJoin();
    void testShaderModelFileNameIgnored();
    void testInstallRootPrefixStripped();
    void testFallsBackToAnyTextureSet();
    void testNotReadable();
};

void TestMatFile::testTextureSetJoin()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + "/mymat.mat";
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(R"MAT({
      "Objects": [
        {"Parent": "materials\\layered\\root\\layeredmaterials.mat", "Components": [
          {"Index":0,"Type":"BSComponentDB::CTName","Data":{"Name":"TexFamily"}},
          {"Index":0,"Type":"BSMaterial::ShaderModelComponent","Data":{"FileName":"4LayerStandard"}}]},
        {"Parent": "materials\\layered\\root\\materials.mat", "Components": [
          {"Index":0,"Type":"BSComponentDB::CTName","Data":{"Name":"TexFamily_Material1"}},
          {"Index":0,"Type":"BSMaterial::TextureSetID","Data":{"ID":"res:6FBC4863:00060B66:A511721A"}}]},
        {"Parent": "materials\\layered\\root\\texturesets.mat", "Components": [
          {"Index":0,"Type":"BSComponentDB::CTName","Data":{"Name":"TexFamily_TextureSet1"}},
          {"Index":0,"Type":"BSMaterial::MRTextureFile","Data":{"FileName":"Data\\Textures\\thing_color.dds"}},
          {"Index":1,"Type":"BSMaterial::MRTextureFile","Data":{"FileName":"Data\\Textures\\thing_normal.dds"}}]},
        {"Parent": "materials\\layered\\root\\texturesets.mat", "Components": [
          {"Index":0,"Type":"BSComponentDB::CTName","Data":{"Name":"OtherFamily_TextureSet1"}},
          {"Index":0,"Type":"BSMaterial::MRTextureFile","Data":{"FileName":"Data\\Textures\\other_color.dds"}}]}
      ]})MAT");
    f.close();

    const QMap<int, QString> set = MatFile::textureSlots(path);
    QCOMPARE(set.size(), 2);
    QCOMPARE(set.value(0), QStringLiteral("Data\\Textures\\thing_color.dds"));
    QCOMPARE(set.value(1), QStringLiteral("Data\\Textures\\thing_normal.dds"));
    // Sibling texture sets of unrelated families must not leak in.
    QVERIFY(!set.values().contains(QStringLiteral("Data\\Textures\\other_color.dds")));
}

void TestMatFile::testShaderModelFileNameIgnored()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + "/shader.mat";
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(R"MAT({
      "Objects": [
        {"Parent": "materials\\layered\\root\\layeredmaterials.mat", "Components": [
          {"Index":0,"Type":"BSComponentDB::CTName","Data":{"Name":"Only"}},
          {"Index":0,"Type":"BSMaterial::ShaderModelComponent","Data":{"FileName":"1LayerStandard"}}]}
      ]})MAT");
    f.close();

    // "1LayerStandard" is a shader model, not a texture: no slots, no textures.
    QVERIFY(MatFile::textureSlots(path).isEmpty());
}

void TestMatFile::testInstallRootPrefixStripped()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString dataDir = dir.path();
    QDir().mkpath(dataDir + "/Textures/common");
    // A real texture the resolver would be expected to find.
    QVERIFY(QFile::copy(QCoreApplication::applicationFilePath(),
                        dataDir + "/Textures/common/stub.dds"));

    const QString matDir = dataDir + "/materials/thing";
    QDir().mkpath(matDir);
    const QString path = matDir + "/thing.mat";
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(R"MAT({
      "Objects": [
        {"Parent": "materials\\layered\\root\\texturesets.mat", "Components": [
          {"Index":0,"Type":"BSComponentDB::CTName","Data":{"Name":"thing_TextureSet1"}},
          {"Index":0,"Type":"BSMaterial::MRTextureFile","Data":{"FileName":"Data\\Textures\\common\\stub.dds"}}]}
      ]})MAT");
    f.close();

    const QString tex = MatFile::colorTexture("Materials\\thing\\thing.mat", dataDir);
    // .mat paths are relative to the install root, so the leading "Data" has to
    // go for the asset resolver to find the file under dataDir.
    QCOMPARE(tex, QStringLiteral("Textures\\common\\stub.dds"));
    QVERIFY2(QFileInfo::exists(dataDir + "/" + tex),
             "colour texture must resolve to a real file under the data dir");
}

void TestMatFile::testFallsBackToAnyTextureSet()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + "/flat.mat";
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    // No layered root: the file only carries a texture set.
    f.write(R"MAT({
      "Objects": [
        {"Parent": "materials\\layered\\root\\texturesets.mat", "Components": [
          {"Index":0,"Type":"BSComponentDB::CTName","Data":{"Name":"flat_TextureSet1"}},
          {"Index":0,"Type":"BSMaterial::MRTextureFile","Data":{"FileName":"Data\\Textures\\flat_color.dds"}}]}
      ]})MAT");
    f.close();

    QCOMPARE(MatFile::textureSlots(path).value(0),
             QStringLiteral("Data\\Textures\\flat_color.dds"));
}

void TestMatFile::testNotReadable()
{
    QVERIFY(MatFile::textureSlots(QString()).isEmpty());
    QVERIFY(MatFile::textureSlots(QStringLiteral("nope/missing.mat")).isEmpty());
    QVERIFY(MatFile::colorTexture("nope/missing.mat", "nope").isEmpty());
    // A base-game material is archive-only, so a bare data dir yields nothing.
    QVERIFY(MatFile::findLoose(QStringLiteral("Materials\\Common\\Metal\\MetalAluminum01.mat"),
                               QStringLiteral("nope")).isEmpty());
}

QTEST_MAIN(TestMatFile)
#include "test_matfile.moc"
