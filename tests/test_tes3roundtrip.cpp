#include <QtTest>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "../../src/model/world/data.hpp"
#include "../../src/model/world/ckid.hpp"
#include "../../src/model/doc/messages.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/tes4.hpp"
#include "../../libs/files/esm/gameformat.hpp"
#include "../../libs/files/esm/Tes3record.hpp"
#include "../../libs/components/tier1_components.hpp"
#include "../../libs/components/tesfullname.hpp"
#include "../../libs/components/tes3_components.hpp"
#include "../../src/model/tools/editrecordcommand.hpp"
#include "../../src/model/tools/undostack.hpp"
#include "../../libs/files/filepaths.hpp"
#include "../../libs/files/log/logger.hpp"

using namespace GameFormat;

class TestTes3RoundTrip : public QObject
{
    Q_OBJECT

private:
    QString fixturePath() const
    {
        return qEnvironmentVariable(
            "OPENCK_TEST_MORROWIND_ESM",
            QStringLiteral("C:/XboxGames/The Elder Scrolls III- Morrowind (PC)/Content/"
                           "Morrowind GOTY English/Data Files/Morrowind.esm"));
    }

    static QByteArray slurp(const QString& path)
    {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return QByteArray();
        return f.readAll();
    }

private slots:
    void initTestCase();
    void testLoadAndSaveByteIdentical();
    void testPerTypeCounts();
    void testSyntheticComponentWriteBack();
    void testSyntheticTypedDataEdit();

    static bool saveDataTo(Data& data, const QString& path);
    static bool loadDataFrom(Data& data, const QString& fileName);
};

void TestTes3RoundTrip::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Error);
    OpenCK::Logging::Logger::instance().init(QStringLiteral(
        "C:/Users/max/AppData/Local/Temp/opencode/test_tes3roundtrip_log.txt"));
}

void TestTes3RoundTrip::testLoadAndSaveByteIdentical()
{
    const QString path = fixturePath();
    if (!QFile::exists(path))
        QSKIP("Morrowind.esm fixture not available (set OPENCK_TEST_MORROWIND_ESM)");

    const QByteArray original = slurp(path);
    QVERIFY(!original.isEmpty());

    FilePaths paths;
    paths.dataDir.setPath(QFileInfo(path).absolutePath());
    const QString fileName = QFileInfo(path).fileName();
    Data data(QStringList{ fileName }, paths);
    QVERIFY2(data.preload(fileName, false) > 0, "preload failed");
    QCOMPARE(data.currentGame(), Game::Morrowind);

    Messages messages(Message::Info);
    int guard = 0;
    while (!data.continueLoading(messages))
    {
        if (++guard > 5000000)
        {
            QFAIL("continueLoading did not converge");
        }
    }

    QCOMPARE(data.pluginOrder().size(), 48295);

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString savedPath = tmp.path() + "/roundtrip.esm";

    {
        QFile out(savedPath);
        QVERIFY(out.open(QIODevice::WriteOnly));

        ESMWriter writer;
        const Header header = data.getReaderHeader();
        writer.setTes3(true);
        writer.setFileFlags(header.flags.val);
        writer.setVersion(header.version);
        writer.setAuthor(header.author);
        writer.setDescription(header.description);
        writer.setNumRecords(header.numRecords);
        writer.setTes3FileType(header.tes3FileType);
        writer.setFormatVersion(header.formatVersion);
        writer.setTes3Gmdt(header.tes3Gmdt);
        writer.setTes3Scrd(header.tes3Scrd);
        writer.setTes3Scrs(header.tes3Scrs);
        writer.save(out);
        data.saveTes3Records(writer);
        writer.close();
    }

    const QByteArray saved = slurp(savedPath);
    QVERIFY(!saved.isEmpty());

    if (saved != original)
    {
        qint64 diffAt = -1;
        const qint64 n = qMin(saved.size(), original.size());
        for (qint64 i = 0; i < n; ++i)
        {
            if (saved.at(i) != original.at(i))
            {
                diffAt = i;
                break;
            }
        }
        if (diffAt < 0)
            diffAt = n;
        QFAIL(qPrintable(QString("round-trip differs at offset %1 (saved %2 bytes, original %3 bytes)")
            .arg(diffAt).arg(saved.size()).arg(original.size())));
    }
    QCOMPARE(saved, original);
}

void TestTes3RoundTrip::testPerTypeCounts()
{
    const QString path = fixturePath();
    if (!QFile::exists(path))
        QSKIP("Morrowind.esm fixture not available (set OPENCK_TEST_MORROWIND_ESM)");

    FilePaths paths;
    paths.dataDir.setPath(QFileInfo(path).absolutePath());
    const QString fileName = QFileInfo(path).fileName();
    Data data(QStringList{ fileName }, paths);
    QVERIFY(data.preload(fileName, false) > 0);

    Messages messages(Message::Info);
    int guard = 0;
    while (!data.continueLoading(messages))
    {
        if (++guard > 5000000)
            QFAIL("continueLoading did not converge");
    }

    struct Expect { NAME code; CkId::Type type; int count; };
    const Expect expect[] = {
        { NAME('GMST'), CkId::Type_Gmst, 1449 },
        { NAME('NPC_'), CkId::Type_Npc_, 2675 },
        { NAME('STAT'), CkId::Type_Stat_, 2788 },
        { NAME('DIAL'), CkId::Type_Dial_, 2358 },
        { NAME('CELL'), CkId::Type_Cel_, 2538 },
        { NAME('LAND'), CkId::Type_Land_, 1390 },
        { NAME('PGRD'), CkId::Type_Pgrd_, 1194 },
        { NAME('BODY'), CkId::Type_Body_, 1125 },
        { NAME('SPEL'), CkId::Type_Spel_, 990 },
        { NAME('SNDG'), CkId::Type_Sndg_, 168 },
        { NAME('SKIL'), CkId::Type_Skil_, 27 },
        { NAME('LEVC'), CkId::Type_Levc_, 116 },
        { NAME('LEVI'), CkId::Type_Levi_, 227 },
        { NAME('LOCK'), CkId::Type_Lock_, 6 },
        { NAME('PROB'), CkId::Type_Prob_, 6 },
        { NAME('REPA'), CkId::Type_Repa_, 6 },
    };

    int total = 0;
    for (const auto& tc : data.allCollectionsWithTypes())
    {
        auto* coll = dynamic_cast<IdCollection<Tes3Record>*>(tc.collection);
        QVERIFY(coll != nullptr);
        total += coll->size();
    }
    QCOMPARE(total, 48295);

    for (const Expect& e : expect)
    {
        auto* coll = dynamic_cast<IdCollection<Tes3Record>*>(
            data.getCollectionByType(e.type));
        QVERIFY2(coll != nullptr, qPrintable(QString("no collection for %1").arg(e.count)));
        QCOMPARE(coll->size(), e.count);
        QVERIFY(coll->size() > 0 && coll->getFormId(0) != 0);
    }
}

namespace
{
QString tes3FullName(const Tes3Record& rec)
{
    const auto* full = static_cast<const tescomponents::TESFullName_Component*>(
        rec.components.findByName(QStringLiteral("TESFullName")));
    return full ? full->fullName : QString();
}

QByteArray tes3DataBytes(const Tes3Record& rec)
{
    const auto* data = static_cast<const tescomponents::Tes3Data_Component*>(
        rec.components.findByName(QStringLiteral("Tes3Data")));
    return data ? data->data : QByteArray();
}

QString tes3ModelPath(const Tes3Record& rec)
{
    const auto* model = static_cast<const tescomponents::TESModel_Component*>(
        rec.components.findByName(QStringLiteral("TESModel")));
    return model ? model->modelPath : QString();
}

IdCollection<Tes3Record>* clotCollection(Data& data)
{
    return dynamic_cast<IdCollection<Tes3Record>*>(
        data.getCollectionByType(CkId::Type_Clot_));
}
}

bool TestTes3RoundTrip::saveDataTo(Data& data, const QString& path)
{
    QFile out(path);
    if (!out.open(QIODevice::WriteOnly))
        return false;
    ESMWriter writer;
    const Header header = data.getReaderHeader();
    writer.setTes3(true);
    writer.setFileFlags(header.flags.val);
    writer.setVersion(header.version);
    writer.setAuthor(header.author);
    writer.setDescription(header.description);
    writer.setNumRecords(header.numRecords);
    writer.setTes3FileType(header.tes3FileType);
    writer.setFormatVersion(header.formatVersion);
    writer.setTes3Gmdt(header.tes3Gmdt);
    writer.setTes3Scrd(header.tes3Scrd);
    writer.setTes3Scrs(header.tes3Scrs);
    writer.save(out);
    data.saveTes3Records(writer);
    writer.close();
    out.close();
    return true;
}

bool TestTes3RoundTrip::loadDataFrom(Data& data, const QString& fileName)
{
    if (data.preload(fileName, false) <= 0)
        return false;
    Messages messages(Message::Info);
    int guard = 0;
    while (!data.continueLoading(messages))
    {
        if (++guard > 100000)
            return false;
    }
    return true;
}

// TES3 Phase 2c: component edits (form-dialog path) survive save/reload at
// their original subrecord positions, stay undoable, and untouched saves
// remain byte-identical. Game detection keys on the basename, so every
// reload stage lives in its own directory as Morrowind.esp.
void TestTes3RoundTrip::testSyntheticComponentWriteBack()
{
    QTemporaryDir srcDir;
    QVERIFY(srcDir.isValid());
    const QString fileName = QStringLiteral("Morrowind.esp");
    const QString srcPath = srcDir.filePath(fileName);
    const QByteArray dataBytes("\x01\x02\x03\x04\x05\x06", 6);

    {
        QFile out(srcPath);
        QVERIFY(out.open(QIODevice::WriteOnly));
        ESMWriter writer;
        writer.setTes3(true);
        writer.setAuthor("Synthetic TES3 Test");
        writer.setDescription("component write-back fixture");
        writer.save(out);

        RecHeader h;
        writer.startRecord(NAME('CLOT'), h);
        writer.writeSubZString(NAME('NAME'), QStringLiteral("synth_shirt"));
        writer.writeSubZString(NAME('FULL'), QStringLiteral("Shirt"));
        writer.writeSubZString(NAME('MODL'), QStringLiteral("m\\shirt.nif"));
        writer.startSubRecord(NAME('DATA'));
        writer.writeRawData(dataBytes.constData(), dataBytes.size());
        writer.endSubRecord();
        writer.endRecord();

        writer.startRecord(NAME('CLOT'), h);
        writer.writeSubZString(NAME('NAME'), QStringLiteral("synth_pants"));
        writer.writeSubZString(NAME('FULL'), QStringLiteral("Pants"));
        writer.endRecord();

        writer.close();
        out.close();
    }
    const QByteArray original = slurp(srcPath);
    QVERIFY(!original.isEmpty());

    FilePaths paths;
    paths.dataDir.setPath(srcDir.path());
    Data data(QStringList{ fileName }, paths);
    QVERIFY(loadDataFrom(data, fileName));
    QCOMPARE(data.currentGame(), Game::Morrowind);

    IdCollection<Tes3Record>* coll = clotCollection(data);
    QVERIFY(coll != nullptr);
    QCOMPARE(coll->size(), 2);
    QCOMPARE(tes3FullName(coll->getRecord(0).get()), QStringLiteral("Shirt"));
    QCOMPARE(tes3DataBytes(coll->getRecord(0).get()), dataBytes);

    QTemporaryDir untouchedDir;
    QVERIFY(untouchedDir.isValid());
    const QString untouchedPath = untouchedDir.filePath("untouched.esp");
    QVERIFY(saveDataTo(data, untouchedPath));
    QCOMPARE(slurp(untouchedPath), original);

    Tes3Record originalRec = coll->getRecord(0).get();
    Tes3Record modifiedRec = originalRec;
    auto* full = static_cast<tescomponents::TESFullName_Component*>(
        modifiedRec.components.findByName(QStringLiteral("TESFullName")));
    QVERIFY(full != nullptr);
    full->fullName = QStringLiteral("Edited Shirt");
    auto* tdata = static_cast<tescomponents::Tes3Data_Component*>(
        modifiedRec.components.findByName(QStringLiteral("Tes3Data")));
    QVERIFY(tdata != nullptr);
    tdata->data = QByteArray("\x0a\x0b\x0c", 3);

    EditRecordCommand<Tes3Record> probe(coll, 0, originalRec, modifiedRec, "Edit CLOT");
    QVERIFY(probe.hasChanged());
    UndoStack* stack = data.getUndoStack();
    QVERIFY(stack != nullptr);
    stack->push(new EditRecordCommand<Tes3Record>(coll, 0, originalRec, modifiedRec, "Edit CLOT"));
    QCOMPARE(tes3FullName(coll->getRecord(0).get()), QStringLiteral("Edited Shirt"));

    QTemporaryDir editedDir;
    QVERIFY(editedDir.isValid());
    const QString editedPath = editedDir.filePath(fileName);
    QVERIFY(saveDataTo(data, editedPath));

    FilePaths editedPaths;
    editedPaths.dataDir.setPath(editedDir.path());
    Data reloaded(QStringList{ fileName }, editedPaths);
    QVERIFY(loadDataFrom(reloaded, fileName));
    IdCollection<Tes3Record>* reloadedColl = clotCollection(reloaded);
    QVERIFY(reloadedColl != nullptr);
    QCOMPARE(reloadedColl->size(), 2);
    const Tes3Record& editedRec = reloadedColl->getRecord(0).get();
    QCOMPARE(tes3FullName(editedRec), QStringLiteral("Edited Shirt"));
    QCOMPARE(tes3DataBytes(editedRec), QByteArray("\x0a\x0b\x0c", 3));
    QCOMPARE(tes3ModelPath(editedRec), QStringLiteral("m\\shirt.nif"));
    QCOMPARE(editedRec.loadOrder,
             QVector<NAME>({ NAME('NAME'), NAME('FULL'), NAME('MODL'), NAME('DATA') }));

    stack->undo();
    QCOMPARE(tes3FullName(coll->getRecord(0).get()), QStringLiteral("Shirt"));
    QCOMPARE(tes3DataBytes(coll->getRecord(0).get()), dataBytes);
    QTemporaryDir undoneDir;
    QVERIFY(undoneDir.isValid());
    const QString undonePath = undoneDir.filePath(fileName);
    QVERIFY(saveDataTo(data, undonePath));
    QCOMPARE(slurp(undonePath), slurp(untouchedPath));

    stack->redo();
    QCOMPARE(tes3FullName(coll->getRecord(0).get()), QStringLiteral("Edited Shirt"));

    // A component value for a subrecord the source lacked is appended.
    Tes3Record pantsOriginal = coll->getRecord(1).get();
    Tes3Record pantsModified = pantsOriginal;
    auto* pantsModel = static_cast<tescomponents::TESModel_Component*>(
        pantsModified.components.findByName(QStringLiteral("TESModel")));
    QVERIFY(pantsModel != nullptr);
    pantsModel->modelPath = QStringLiteral("m\\pants.nif");
    EditRecordCommand<Tes3Record> pantsProbe(
        coll, 1, pantsOriginal, pantsModified, "Add pants MODL");
    QVERIFY(pantsProbe.hasChanged());
    stack->push(new EditRecordCommand<Tes3Record>(
        coll, 1, pantsOriginal, pantsModified, "Add pants MODL"));

    QTemporaryDir appendedDir;
    QVERIFY(appendedDir.isValid());
    const QString appendedPath = appendedDir.filePath(fileName);
    QVERIFY(saveDataTo(data, appendedPath));

    FilePaths appendedPaths;
    appendedPaths.dataDir.setPath(appendedDir.path());
    Data appended(QStringList{ fileName }, appendedPaths);
    QVERIFY(loadDataFrom(appended, fileName));
    IdCollection<Tes3Record>* appendedColl = clotCollection(appended);
    QVERIFY(appendedColl != nullptr);
    const Tes3Record& pantsRec = appendedColl->getRecord(1).get();
    QCOMPARE(tes3ModelPath(pantsRec), QStringLiteral("m\\pants.nif"));
    QCOMPARE(pantsRec.loadOrder,
             QVector<NAME>({ NAME('NAME'), NAME('FULL'), NAME('MODL') }));
    QCOMPARE(tes3FullName(appendedColl->getRecord(0).get()), QStringLiteral("Edited Shirt"));

    qDebug() << "synthetic TES3 component write-back OK";
}

// Typed DATA layouts (tes3datalayout.*) decode on load, edit through the
// UndoStack, and survive save/reload; unknown layouts stay hex-only.
void TestTes3RoundTrip::testSyntheticTypedDataEdit()
{
    QTemporaryDir srcDir;
    QVERIFY(srcDir.isValid());
    const QString fileName = QStringLiteral("Morrowind.esp");
    const QString srcPath = srcDir.filePath(fileName);
    const QByteArray infoData("\x04\x00\x00\x00\x32\x00\x00\x00\x01\x00\x00\x00", 12);
    const QByteArray cellData("\x02\x00\x00\x00\x07\x00\x00\x00\x03\x00\x00\x00", 12);
    const QByteArray clotData("\xaa\xbb\xcc\xdd\xee\xff", 6);

    {
        QFile out(srcPath);
        QVERIFY(out.open(QIODevice::WriteOnly));
        ESMWriter writer;
        writer.setTes3(true);
        writer.setAuthor("Synthetic TES3 Test");
        writer.setDescription("typed DATA fixture");
        writer.save(out);

        RecHeader h;
        writer.startRecord(NAME('DIAL'), h);
        writer.writeSubZString(NAME('NAME'), QStringLiteral("synth_topic"));
        writer.startSubRecord(NAME('DATA'));
        const char dialType = '\x04';
        writer.writeRawData(&dialType, 1);
        writer.endSubRecord();
        writer.endRecord();

        writer.startRecord(NAME('INFO'), h);
        writer.writeSubZString(NAME('NAME'), QStringLiteral("synth_info"));
        writer.startSubRecord(NAME('DATA'));
        writer.writeRawData(infoData.constData(), infoData.size());
        writer.endSubRecord();
        writer.endRecord();

        writer.startRecord(NAME('CELL'), h);
        writer.writeSubZString(NAME('NAME'), QStringLiteral("synth_cell"));
        writer.startSubRecord(NAME('DATA'));
        writer.writeRawData(cellData.constData(), cellData.size());
        writer.endSubRecord();
        writer.endRecord();

        writer.startRecord(NAME('CLOT'), h);
        writer.writeSubZString(NAME('NAME'), QStringLiteral("synth_clot"));
        writer.startSubRecord(NAME('DATA'));
        writer.writeRawData(clotData.constData(), clotData.size());
        writer.endSubRecord();
        writer.endRecord();

        writer.close();
        out.close();
    }
    const QByteArray original = slurp(srcPath);
    QVERIFY(!original.isEmpty());

    const auto tes3DataOf = [](Tes3Record& rec) -> tescomponents::Tes3Data_Component* {
        return static_cast<tescomponents::Tes3Data_Component*>(
            rec.components.findByName(QStringLiteral("Tes3Data")));
    };

    FilePaths paths;
    paths.dataDir.setPath(srcDir.path());
    Data data(QStringList{ fileName }, paths);
    QVERIFY(loadDataFrom(data, fileName));

    auto* dialColl = dynamic_cast<IdCollection<Tes3Record>*>(
        data.getCollectionByType(CkId::Type_Dial_));
    auto* infoColl = dynamic_cast<IdCollection<Tes3Record>*>(
        data.getCollectionByType(CkId::Type_Info_));
    auto* cellColl = dynamic_cast<IdCollection<Tes3Record>*>(
        data.getCollectionByType(CkId::Type_Cel_));
    auto* clotColl = dynamic_cast<IdCollection<Tes3Record>*>(
        data.getCollectionByType(CkId::Type_Clot_));
    QVERIFY(dialColl && infoColl && cellColl && clotColl);
    QCOMPARE(dialColl->size(), 1);
    QCOMPARE(infoColl->size(), 1);
    QCOMPARE(cellColl->size(), 1);
    QCOMPARE(clotColl->size(), 1);

    tescomponents::Tes3Data_Component* dialData = tes3DataOf(dialColl->getRecord(0).get());
    tescomponents::Tes3Data_Component* infoDataComp = tes3DataOf(infoColl->getRecord(0).get());
    tescomponents::Tes3Data_Component* cellDataComp = tes3DataOf(cellColl->getRecord(0).get());
    tescomponents::Tes3Data_Component* clotDataComp = tes3DataOf(clotColl->getRecord(0).get());
    QVERIFY(dialData && infoDataComp && cellDataComp && clotDataComp);

    QVERIFY(dialData->typedValid);
    QCOMPARE(dialData->typedFields.size(), 1);
    QCOMPARE(dialData->typedFields[0].name, QStringLiteral("dialogType"));
    QCOMPARE(dialData->typedFields[0].value.toUInt(), 4u);

    QVERIFY(infoDataComp->typedValid);
    QCOMPARE(infoDataComp->typedFields.size(), 6);
    QCOMPARE(infoDataComp->typedFields[1].name, QStringLiteral("disposition"));
    QCOMPARE(infoDataComp->typedFields[1].value.toUInt(), 50u);

    QVERIFY(cellDataComp->typedValid);
    QCOMPARE(cellDataComp->typedFields[1].name, QStringLiteral("gridX"));
    QCOMPARE(cellDataComp->typedFields[1].value.toInt(), 7);

    QVERIFY(!clotDataComp->typedValid);
    QCOMPARE(clotDataComp->toHex(), QStringLiteral("AABBCCDDEEFF"));

    // Typed properties: 6 fields + hex view; edits go through setValue.
    {
        auto props = infoDataComp->createEditorProperties();
        QCOMPARE(int(props.size()), 7);
        QCOMPARE(props[1]->name(), QStringLiteral("disposition"));
        props[1]->setValue(QVariant(75u));
        QCOMPARE(infoDataComp->typedFields[1].value.toUInt(), 75u);
        // Back to the loaded value; the undoable edit below re-applies it.
        props[1]->setValue(QVariant(50u));
        // Unsigned widths clamp instead of overflowing the payload.
        props[1]->setValue(QVariant(999u));
        QCOMPARE(infoDataComp->typedFields[1].value.toUInt(), 255u);
        props[1]->setValue(QVariant(50u));
    }

    QTemporaryDir untouchedDir;
    QVERIFY(untouchedDir.isValid());
    QVERIFY(saveDataTo(data, untouchedDir.filePath("untouched.esp")));
    QCOMPARE(slurp(untouchedDir.filePath("untouched.esp")), original);

    // Undoable typed edits on INFO disposition + CELL gridX.
    UndoStack* stack = data.getUndoStack();
    QVERIFY(stack != nullptr);
    {
        Tes3Record infoOriginal = infoColl->getRecord(0).get();
        Tes3Record infoModified = infoOriginal;
        QVERIFY(tes3DataOf(infoModified)->setTypedValue(1, QVariant(75u)));
        EditRecordCommand<Tes3Record> probe(infoColl, 0, infoOriginal, infoModified);
        QVERIFY(probe.hasChanged());
        stack->push(new EditRecordCommand<Tes3Record>(infoColl, 0, infoOriginal, infoModified,
                                                      "Edit INFO disposition"));
    }
    {
        Tes3Record cellOriginal = cellColl->getRecord(0).get();
        Tes3Record cellModified = cellOriginal;
        QVERIFY(tes3DataOf(cellModified)->setTypedValue(1, QVariant(8)));
        stack->push(new EditRecordCommand<Tes3Record>(cellColl, 0, cellOriginal, cellModified,
                                                      "Edit CELL gridX"));
    }

    QTemporaryDir editedDir;
    QVERIFY(editedDir.isValid());
    const QString editedPath = editedDir.filePath(fileName);
    QVERIFY(saveDataTo(data, editedPath));

    FilePaths editedPaths;
    editedPaths.dataDir.setPath(editedDir.path());
    Data reloaded(QStringList{ fileName }, editedPaths);
    QVERIFY(loadDataFrom(reloaded, fileName));
    auto* reloadedInfo = dynamic_cast<IdCollection<Tes3Record>*>(
        reloaded.getCollectionByType(CkId::Type_Info_));
    auto* reloadedCell = dynamic_cast<IdCollection<Tes3Record>*>(
        reloaded.getCollectionByType(CkId::Type_Cel_));
    QVERIFY(reloadedInfo && reloadedCell);
    QCOMPARE(tes3DataOf(reloadedInfo->getRecord(0).get())->typedFields[1].value.toUInt(), 75u);
    QCOMPARE(tes3DataOf(reloadedInfo->getRecord(0).get())->typedFields[3].value.toUInt(), 1u);
    QCOMPARE(tes3DataOf(reloadedCell->getRecord(0).get())->typedFields[1].value.toInt(), 8);
    QCOMPARE(reloadedInfo->getRecord(0).get().loadOrder,
             QVector<NAME>({ NAME('NAME'), NAME('DATA') }));

    stack->undo();
    stack->undo();
    QCOMPARE(tes3DataOf(infoColl->getRecord(0).get())->typedFields[1].value.toUInt(), 50u);
    QCOMPARE(tes3DataOf(cellColl->getRecord(0).get())->typedFields[1].value.toInt(), 7);

    qDebug() << "synthetic TES3 typed DATA edit OK";
}

QTEST_MAIN(TestTes3RoundTrip)
#include "test_tes3roundtrip.moc"
