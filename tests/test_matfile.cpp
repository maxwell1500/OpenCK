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
    void testCompiledMaterialDatabase();
    void testLayeredInheritance();
    void testOpacityCutout();
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

    // The paint family has no colour map on its base layer: the colour is the
    // layer's tint. The lowercase NIF spelling must still find the zip entry,
    // whose name is mixed case.
    QCOMPARE(MatFile::colorTexture(QStringLiteral("Materials\\SetDressing\\DigitalTables\\paintedmetalpanel01_orange01.mat"), cache.path()), QString());
    const QColor orange = MatFile::baseTint(QStringLiteral("Materials\\SetDressing\\DigitalTables\\paintedmetalpanel01_orange01.mat"), cache.path());
    QVERIFY(orange.isValid());
    QVERIFY(orange.red() > orange.green() && orange.green() > orange.blue());

    // white01 has a Color with w=0 and no albedo on its base. A later detail
    // layer does carry metalAlu_color, which must not stand in for the paint.
    const QString white = QStringLiteral("Materials\\SetDressing\\DigitalTables\\paintedmetalpanel01_white01.mat");
    QCOMPARE(MatFile::colorTexture(white, cache.path()), QString());
    QVERIFY(MatFile::baseTint(white, cache.path()).isValid());

    // Albedo that only the Parent chain carries: the decal's own texture set
    // is empty and inherits colour, normal and opacity from another file.
    QVERIFY(MatFile::colorTexture(QStringLiteral("Materials\\Common\\Decal\\DecalMetalBevels04n.mat"), cache.path())
                .endsWith(QStringLiteral("DecalMetalBevels04_color.DDS"), Qt::CaseInsensitive));

    // Alpha-only decals have no colour and no tint: their whole appearance is
    // the opacity mask, which must come back so the viewport cuts a hole
    // instead of drawing an opaque block over the surface.
    const QString overlay = QStringLiteral("Materials\\Common\\Decal\\DecalMetalBevels01n.mat");
    QCOMPARE(MatFile::colorTexture(overlay, cache.path()), QString());
    QVERIFY(!MatFile::baseTint(overlay, cache.path()).isValid());
    QVERIFY(MatFile::opacityTexture(overlay, cache.path())
                .endsWith(QStringLiteral("decalmetalbevels01_opacity.DDS"), Qt::CaseInsensitive));
}

void TestMatFile::testCompiledMaterialDatabase()
{
    // Starfield - Materials.ba2 extracts materialsbeta.cdb; probe it if
    // present on disk (extracted by AssetResolver or manually).
    const QStringList cands = {
        QStringLiteral("C:/Temp/materialsbeta.cdb"),
        QStringLiteral("C:/XboxGames/Starfield/Content/Data/.openck_cache/materialsbeta.cdb"),
    };
    QString cdb;
    for (const QString& c : cands)
        if (QFileInfo::exists(c)) { cdb = c; break; }
    if (cdb.isEmpty())
        QSKIP("materialsbeta.cdb not present on disk");

    MatFile::setMaterialDatabase(cdb);

    QTemporaryDir cache;
    QVERIFY(cache.isValid());

    // Computer_Cabinets_Computer is authored in the CDB.
    const QString tex = MatFile::colorTexture(
        QStringLiteral("Materials\\SetDressing\\Computer_Cabinets\\Computer_Cabinets_Computer.mat"),
        cache.path());
    QCOMPARE(tex, QStringLiteral("Textures\\SetDressing\\computer_cabinets\\computer_cabinets_computer_color.DDS"));

    // GlowDecal_White01 carries colour in slot 7 in the authored zip, but its
    // CDB texture set directly associates with the colour map.
    const QString glow = MatFile::colorTexture(
        QStringLiteral("Materials\\StarStations\\Emissives\\GlowDecal_White01.mat"),
        cache.path());
    QCOMPARE(glow, QStringLiteral("Textures\\architecture\\generic\\common\\DecalLightBlur01_color.dds"));
}

void TestMatFile::testLayeredInheritance()
{
    // variant.mat imports base.mat. Its texture set has no files and names the
    // base's texture set as Parent, so the albedo can only come from following
    // the Parent id across files. The tint is variant's own.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QDir().mkpath(dir.path() + "/Materials");
    auto write = [&](const QString& name, const char* json) {
        QFile f(dir.path() + "/Materials/" + name);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(json);
    };
    write("base.mat", R"MAT({"Objects":[
      {"Parent":"Data\\Materials\\shader.mat","Components":[
        {"Index":0,"Type":"BSComponentDB::CTName","Data":{"Name":"base"}},
        {"Index":0,"Type":"BSMaterial::LayerID","Data":{"ID":"res:00000001:00000001:00000001"}}]},
      {"ID":"res:00000001:00000001:00000001","Parent":"res:0A","Components":[
        {"Index":0,"Type":"BSMaterial::MaterialID","Data":{"ID":"res:00000002:00000001:00000001"}}]},
      {"ID":"res:00000002:00000001:00000001","Parent":"res:0B","Components":[
        {"Index":0,"Type":"BSMaterial::TextureSetID","Data":{"ID":"res:00000003:00000001:00000001"}}]},
      {"ID":"res:00000003:00000001:00000001","Parent":"res:0C","Components":[
        {"Index":0,"Type":"BSMaterial::MRTextureFile","Data":{"FileName":"Data\\Textures\\base_color.dds"}}]}
    ]})MAT");
    write("variant.mat", R"MAT({"Import":["Data\\Materials\\base.mat"],"Objects":[
      {"Parent":"Data\\Materials\\base.mat","Components":[
        {"Index":0,"Type":"BSComponentDB::CTName","Data":{"Name":"variant"}},
        {"Index":0,"Type":"BSMaterial::LayerID","Data":{"ID":"res:00000011:00000001:00000001"}}]},
      {"ID":"res:00000011:00000001:00000001","Parent":"res:00000001:00000001:00000001","Components":[
        {"Index":0,"Type":"BSMaterial::MaterialID","Data":{"ID":"res:00000012:00000001:00000001"}}]},
      {"ID":"res:00000012:00000001:00000001","Parent":"res:00000002:00000001:00000001","Components":[
        {"Index":0,"Type":"BSMaterial::TextureSetID","Data":{"ID":"res:00000013:00000001:00000001"}},
        {"Index":0,"Type":"BSMaterial::Color","Data":{"Value":{"Type":"XMFLOAT4","Data":{"x":"0.5","y":"0.25","z":"0.75","w":"1"}}}}]},
      {"ID":"res:00000013:00000001:00000001","Parent":"res:00000003:00000001:00000001","Components":[
        {"Index":0,"Type":"BSComponentDB::CTName","Data":{"Name":"variant_TextureSet1"}}]}
    ]})MAT");

    QCOMPARE(MatFile::colorTexture(QStringLiteral("Materials\\variant.mat"), dir.path()),
             QStringLiteral("Textures\\base_color.dds"));
    const QColor tint = MatFile::baseTint(QStringLiteral("Materials\\variant.mat"), dir.path());
    QVERIFY(tint.isValid());
    QCOMPARE(tint, QColor::fromRgbF(0.5, 0.25, 0.75));
    // The base itself is untinted.
    QVERIFY(!MatFile::baseTint(QStringLiteral("Materials\\base.mat"), dir.path()).isValid());
}

void TestMatFile::testOpacityCutout()
{
    // An alpha-only decal carries an opacity mask and no colour at all. It has to
    // be drawable as a cutout, so the mask must resolve even though there is no
    // albedo to read.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QDir().mkpath(dir.path() + "/Materials");
    const QString path = dir.path() + "/Materials/overlay.mat";
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(R"MAT({"Objects":[
      {"Parent":"materials\\layered\\root\\layeredmaterials.mat","Components":[
        {"Index":0,"Type":"BSComponentDB::CTName","Data":{"Name":"Overlay"}},
        {"Index":0,"Type":"BSMaterial::LayerID","Data":{"ID":"res:00000010:00000001:00000001"}}]},
      {"ID":"res:00000010:00000001:00000001","Components":[
        {"Index":0,"Type":"BSComponentDB::CTName","Data":{"Name":"Overlay_Layer1"}},
        {"Index":0,"Type":"BSMaterial::MaterialID","Data":{"ID":"res:00000020:00000001:00000001"}},
        {"Index":0,"Type":"BSMaterial::UVStreamID","Data":{"ID":"res:00000030:00000001:00000001"}}]},
      {"ID":"res:00000020:00000001:00000001","Components":[
        {"Index":0,"Type":"BSComponentDB::CTName","Data":{"Name":"Overlay_Material1"}},
        {"Index":0,"Type":"BSMaterial::TextureSetID","Data":{"ID":"res:00000040:00000001:00000001"}}]},
      {"ID":"res:00000040:00000001:00000001","Components":[
        {"Index":0,"Type":"BSComponentDB::CTName","Data":{"Name":"Overlay_TextureSet1"}},
        {"Index":1,"Type":"BSMaterial::MRTextureFile","Data":{"FileName":"Data\\Textures\\overlay_normal.dds"}},
        {"Index":2,"Type":"BSMaterial::MRTextureFile","Data":{"FileName":"Data\\Textures\\overlay_opacity.dds"}}]},
      {"ID":"res:00000030:00000001:00000001","Components":[
        {"Index":0,"Type":"BSComponentDB::CTName","Data":{"Name":"Overlay_UVStream1"}}]}
    ]})MAT");
    f.close();

    // No colour of its own: normal and opacity only.
    QCOMPARE(MatFile::colorTexture(QStringLiteral("Materials\\overlay.mat"), dir.path()), QString());
    QVERIFY(!MatFile::baseTint(QStringLiteral("Materials\\overlay.mat"), dir.path()).isValid());
    QCOMPARE(MatFile::opacityTexture(QStringLiteral("Materials\\overlay.mat"), dir.path()),
             QStringLiteral("Textures\\overlay_opacity.dds"));
}

QTEST_MAIN(TestMatFile)
#include "test_matfile.moc"
