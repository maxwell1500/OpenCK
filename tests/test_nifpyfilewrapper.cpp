#include <QtTest>
#include <QTemporaryDir>

#include "../../src/model/tools/nifpynifly/nifpyfilewrapper.hpp"

class TestNifPyFileWrapper : public QObject
{
    Q_OBJECT

private slots:
    void testInitialize();
    void testIsInitialized();
    void testGetPyniflyVersion();
    void testSetDefaultGame();
    void testLoadNifNonExistent();
    void testExtractShapesNonExistent();
    void testExtractTexturesNonExistent();
    void testValidateNifNonExistent();
};

void TestNifPyFileWrapper::testInitialize()
{
    // Without Python/PyNifly, initialization should fail gracefully
    bool result = NifPyFileWrapper::initialize(QString());
    QVERIFY(!result || NifPyFileWrapper::isInitialized());
    
    if (NifPyFileWrapper::isInitialized()) {
        NifPyFileWrapper::shutdown();
        QVERIFY(!NifPyFileWrapper::isInitialized());
    }
}

void TestNifPyFileWrapper::testIsInitialized()
{
    // Should start uninitialized
    QVERIFY(!NifPyFileWrapper::isInitialized());
    
    // Try to initialize with empty path (should fail)
    NifPyFileWrapper::initialize(QString());
    
    // Shutdown if initialized
    if (NifPyFileWrapper::isInitialized()) {
        NifPyFileWrapper::shutdown();
    }
    QVERIFY(!NifPyFileWrapper::isInitialized());
}

void TestNifPyFileWrapper::testGetPyniflyVersion()
{
    QString version = NifPyFileWrapper::getPyniflyVersion();
    // Version should be empty if not initialized, or a valid version string
    QVERIFY(!version.contains("ERROR"));
}

void TestNifPyFileWrapper::testSetDefaultGame()
{
    NifPyFileWrapper::setDefaultGame("SKYRIM");
    NifPyFileWrapper::setDefaultGame("FO4");
    NifPyFileWrapper::setDefaultGame("SKYRIMSE");
    // No exceptions should occur
}

void TestNifPyFileWrapper::testLoadNifNonExistent()
{
    NifPyFileWrapper::NifFileInfo fileInfo;
    bool result = NifPyFileWrapper::loadNif(QString("/nonexistent/path/file.nif"), fileInfo);
    QVERIFY(!result);
}

void TestNifPyFileWrapper::testExtractShapesNonExistent()
{
    QVector<NifPyFileWrapper::ShapeData> shapes;
    bool result = NifPyFileWrapper::extractShapes(QString("/nonexistent/path/file.nif"), shapes);
    QVERIFY(!result);
}

void TestNifPyFileWrapper::testExtractTexturesNonExistent()
{
    QMap<QString, QString> textures;
    bool result = NifPyFileWrapper::extractTextures(QString("/nonexistent/path/file.nif"), textures);
    QVERIFY(!result);
}

void TestNifPyFileWrapper::testValidateNifNonExistent()
{
    QVector<NifPyFileWrapper::ValidationError> errors;
    bool result = NifPyFileWrapper::validateNif(QString("/nonexistent/path/file.nif"), errors);
    QVERIFY(!result);
}

#include "test_nifpyfilewrapper.moc"
QTEST_MAIN(TestNifPyFileWrapper)
