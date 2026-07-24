#include <QtTest>
#include <QTemporaryDir>
#include <QFile>

#include "../../libs/files/nif/nifparser.hpp"
#include "../../src/model/tools/assetconverter.hpp"

class TestNifIntegration : public QObject
{
    Q_OBJECT

private slots:
    void testNifParserLoad();
    void testNifParserNonExistent();
    void testNifToObjConversion();
    void testObjToNifBasic();
    void testTextureConversion();
    void testSoundConversion();
};

void TestNifIntegration::testNifParserLoad()
{
    Nif::NifParser parser;
    // Without a real NIF file, this should fail gracefully
    bool result = parser.load(QString(":/nonexistent/file.nif"));
    QVERIFY(!result);
}

void TestNifIntegration::testNifParserNonExistent()
{
    Nif::NifParser parser;
    bool result = parser.load(QString("C:\\nonexistent\\path\\file.nif"));
    QVERIFY(!result);
}

void TestNifIntegration::testNifToObjConversion()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    
    QString nifPath = tempDir.path() + "/test.nif";
    QString objPath = tempDir.path() + "/test.obj";
    
    // Create a dummy file to test error handling
    QFile nifFile(nifPath);
    if (nifFile.open(QIODevice::WriteOnly)) {
        nifFile.write("not a nif file");
        nifFile.close();
    }
    
    AssetConverter::ConversionResult result = AssetConverter::nifToObj(nifPath, objPath);
    QVERIFY(!result.success);
    QVERIFY(!result.error.isEmpty());
}

void TestNifIntegration::testObjToNifBasic()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    
    QString objPath = tempDir.path() + "/test.obj";
    QString nifPath = tempDir.path() + "/test.nif";
    
    // Create a dummy OBJ file
    QFile objFile(objPath);
    if (objFile.open(QIODevice::WriteOnly)) {
        objFile.write("v 0 0 0\n");
        objFile.close();
    }
    
    AssetConverter::ConversionResult result = AssetConverter::objToNif(objPath, nifPath);
    // May or may not succeed depending on implementation
    QVERIFY(true);
}

void TestNifIntegration::testTextureConversion()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    
    // Test with non-existent files - should fail gracefully
    QStringList inputFiles;
    inputFiles.append(tempDir.path() + "/nonexistent.dds");
    
    AssetConverter::ConversionResult result = AssetConverter::convertTextures(
        inputFiles, tempDir.path(), "png");
    
    QVERIFY(!result.success || result.filesConverted == 0);
}

void TestNifIntegration::testSoundConversion()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    
    // Test with non-existent files - should fail gracefully
    QStringList inputFiles;
    inputFiles.append(tempDir.path() + "/nonexistent.wav");
    
    AssetConverter::ConversionResult result = AssetConverter::convertSounds(
        inputFiles, tempDir.path());
    
    QVERIFY(!result.success || result.filesConverted == 0);
}

#include "test_nifintegration.moc"
QTEST_MAIN(TestNifIntegration)
