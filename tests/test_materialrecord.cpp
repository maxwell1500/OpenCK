#include <QTest>
#include <QTemporaryFile>
#include <QFile>

#include "../../libs/files/esm/materialrecord.hpp"
#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/log/logger.hpp"

class TestMaterialRecord : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testTextureSlotsRoundTrip();
    void testEmptyTextureSlotsOmitsSubrecord();
    void testBlankClearsTextureSlots();
};

void TestMaterialRecord::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_materialrecord_log.txt"));
}

void TestMaterialRecord::testTextureSlotsRoundTrip()
{
    MaterialRecord rec;
    rec.editorId = QStringLiteral("TestMaterial");
    rec.formId = 0x40001;
    rec.materialName = QStringLiteral("TestMaterial");
    rec.textureSlots.insert(QStringLiteral("Diffuse"), QStringLiteral("textures/diff.dds"));
    rec.textureSlots.insert(QStringLiteral("Normal"), QStringLiteral("textures/nrm.dds"));

    QTemporaryFile tmpFile;
    tmpFile.open();
    const QString path = tmpFile.fileName();
    tmpFile.close();

    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        ESMWriter writer;
        writer.setAuthor("Test");
        writer.save(file);
        RecHeader recHeader;
        recHeader.id = rec.formId;
        writer.startRecord('MATR', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(path);
        reader.open();
        const quint32 type = reader.readName();
        QCOMPARE(type, static_cast<quint32>('MATR'));
        MaterialRecord loaded;
        loaded.load(reader, true);
        QVERIFY(loaded.editorId.startsWith(QStringLiteral("TestMaterial")));
        QCOMPARE(loaded.materialName, QStringLiteral("TestMaterial"));
        QCOMPARE(loaded.textureSlots.size(), 2);
        QCOMPARE(loaded.textureSlots.value(QStringLiteral("Diffuse")), QStringLiteral("textures/diff.dds"));
        QCOMPARE(loaded.textureSlots.value(QStringLiteral("Normal")), QStringLiteral("textures/nrm.dds"));
}
}

void TestMaterialRecord::testEmptyTextureSlotsOmitsSubrecord()
{
    MaterialRecord rec;
    rec.editorId = QStringLiteral("PlainMaterial");
    rec.formId = 0x40002;

    QTemporaryFile tmpFile;
    tmpFile.open();
    const QString path = tmpFile.fileName();
    tmpFile.close();

    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        ESMWriter writer;
        writer.setAuthor("Test");
        writer.save(file);
        RecHeader recHeader;
        recHeader.id = rec.formId;
        writer.startRecord('MATR', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(path);
        reader.open();
        reader.readName();
        MaterialRecord loaded;
        loaded.load(reader, true);
        QVERIFY(loaded.textureSlots.isEmpty());
    }
}

void TestMaterialRecord::testBlankClearsTextureSlots()
{
    MaterialRecord rec;
    rec.editorId = QStringLiteral("TestMaterial");
    rec.textureSlots.insert(QStringLiteral("Diffuse"), QStringLiteral("textures/diff.dds"));
    rec.blank();
    QVERIFY(rec.editorId.isEmpty());
    QVERIFY(rec.textureSlots.isEmpty());
}

QTEST_MAIN(TestMaterialRecord)
#include "test_materialrecord.moc"
