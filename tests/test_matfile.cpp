#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QMap>
#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonDocument>

#include "../../libs/files/nif/matfile.hpp"
#include "../../libs/files/data/zipreader.hpp"

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
    void testContentResourcesZip();
    void testRealBaseGameMaterials();
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

void TestMatFile::testContentResourcesZip()
{
    // The CK's Tools\ContentResources.zip holds the base game's authored
    // materials as the same JSON loose files use. Skip unless it is installed.
    const QStringList roots = {
        QStringLiteral("C:/Program Files (x86)/Steam/steamapps/common/Starfield/Tools/ContentResources.zip"),
        QStringLiteral("C:/Program Files/Steam/steamapps/common/Starfield/Tools/ContentResources.zip"),
        QStringLiteral("C:/XboxGames/Starfield/Content/Tools/ContentResources.zip"),
    };
    QString zip;
    for (const QString& r : roots)
        if (QFileInfo::exists(r)) { zip = r; break; }
    if (zip.isEmpty())
        QSKIP("Creation Kit ContentResources.zip not installed");

    ZipReader zr(zip);
    QVERIFY(zr.isValid());
    QVERIFY(zr.hasEntry(QStringLiteral("Materials/Common/Metal/MetalAluminum01.mat")));

    QByteArray raw = zr.readEntry(QStringLiteral("Materials/Common/Metal/MetalAluminum01.mat"));
    QVERIFY(!raw.isEmpty());

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    QCOMPARE(err.error, QJsonParseError::NoError);
    QVERIFY(doc.isObject());

    // The full path must work through MatFile, writing the file into cache.
    MatFile::setContentResources(zip);
    QTemporaryDir cache;
    QVERIFY(cache.isValid());
    const QString mat = MatFile::findLoose(QStringLiteral("Materials\\Common\\Metal\\MetalAluminum01.mat"),
                                           cache.path());
    QVERIFY(!mat.isEmpty());
    const QMap<int, QString> set = MatFile::textureSlots(mat);
    QCOMPARE(set.value(0), QStringLiteral("Data\\Textures\\Common\\Metal\\metalAlu_color.DDS"));
    // Metal galvanised normal map and dull-stain roughness are also authored.
    QVERIFY(set.contains(1));
    QVERIFY(set.contains(3));
}

void TestMatFile::testRealBaseGameMaterials()
{
    const QString zip = QStringLiteral("C:/Program Files (x86)/Steam/steamapps/common/Starfield/Tools/ContentResources.zip");
    if (!QFileInfo::exists(zip))
        QSKIP("CK ContentResources.zip not installed");
    MatFile::setContentResources(zip);

    // Materials referenced by the real base-game asset digitaltable01a.nif.
    const QStringList mats = {
        QStringLiteral("Materials\\Common\\Decal\\DecalLabels01.mat"),
        QStringLiteral("Materials\\Common\\Decal\\DecalLabelsLetters01.mat"),
        QStringLiteral("Materials\\Common\\Decal\\DecalLetterLineOverlay01.mat"),
        QStringLiteral("Materials\\Common\\Decal\\DecalMetalBevels01n.mat"),
        QStringLiteral("Materials\\Common\\Decal\\DecalMetalBevels04n.mat"),
        QStringLiteral("Materials\\Common\\Decal\\DecalMetalBoltsTrimsDetails01.mat"),
        QStringLiteral("Materials\\Common\\Decal\\DecalMetalBoltsTrimsDetails01n.mat"),
        QStringLiteral("Materials\\Common\\Glass\\GlassCleanOpaqueDark01.mat"),
        QStringLiteral("Materials\\Common\\Metal\\MetalAluminum01.mat"),
        QStringLiteral("Materials\\Common\\Metal\\MetalAluminumBrushed01.mat"),
        QStringLiteral("Materials\\Common\\Trim\\TrimMetal03.mat"),
        QStringLiteral("Materials\\Common\\Trim\\TrimTech01.mat"),
        QStringLiteral("Materials\\Common\\Trim\\TrimTech04.mat"),
        QStringLiteral("Materials\\SetDressing\\Computer_Cabinets\\Computer_Cabinets_Computer.mat"),
        QStringLiteral("Materials\\SetDressing\\Computer_Cabinets\\Computer_Cabinets_Panels.mat"),
        QStringLiteral("Materials\\SetDressing\\DigitalTables\\PaintedMetalPanel01_Grey01.mat"),
        QStringLiteral("Materials\\SetDressing\\DigitalTables\\PaintedMetalPanel01_Grey02.mat"),
        QStringLiteral("Materials\\SetDressing\\DigitalTables\\PaintedMetalPanel01_Grey03.mat"),
        QStringLiteral("Materials\\SetDressing\\DigitalTables\\PaintedMetalPanel02_Dark01.mat"),
        QStringLiteral("Materials\\SetDressing\\DigitalTables\\paintedmetalpanel01_orange01.mat"),
        QStringLiteral("Materials\\SetDressing\\DigitalTables\\paintedmetalpanel01_white01.mat"),
        QStringLiteral("Materials\\SetDressing\\DisplayScreens\\DisplayScreenLargeInverted2.mat"),
        QStringLiteral("Materials\\StarStations\\Emissives\\GlowDecal_Blue01.mat"),
        QStringLiteral("Materials\\StarStations\\Emissives\\GlowDecal_Green01.mat"),
        QStringLiteral("Materials\\StarStations\\Emissives\\GlowDecal_Orange01.mat"),
        QStringLiteral("Materials\\StarStations\\Emissives\\GlowDecal_White01.mat"),
        QStringLiteral("Materials\\StarStations\\Emissives\\GlowDecal_White02.mat"),
    };

    QTemporaryDir cache;
    QVERIFY(cache.isValid());
    int resolved = 0;
    for (const QString& m : mats) {
        const QString tex = MatFile::colorTexture(m, cache.path());
        qDebug() << "REAL" << m.split(QLatin1Char('\\')).last() << "->" << tex;
        if (!tex.isEmpty())
            ++resolved;
    }
    qDebug() << "REAL resolved" << resolved << "of" << mats.size();
    QVERIFY2(resolved > 0, "base-game materials must resolve through CK ContentResources.zip");

    // This one is a variation on DecalMetalBevels01.mat: its texture set is
    // named after the parent file, not after the family, so the join has to
    // follow the parent stem. The set carries normal and opacity but no
    // colour, which is why colourTexture() above is empty for it.
    const QString bevels = MatFile::findLoose(QStringLiteral("Materials\\Common\\Decal\\DecalMetalBevels01n.mat"),
                                             cache.path());
    QVERIFY(!bevels.isEmpty());
    const QMap<int, QString> bevelsSet = MatFile::textureSlots(bevels);
    QVERIFY(bevelsSet.contains(1));
    QVERIFY(bevelsSet.contains(2));
    QVERIFY(!bevelsSet.contains(0));
}

QTEST_MAIN(TestMatFile)
#include "test_matfile.moc"
