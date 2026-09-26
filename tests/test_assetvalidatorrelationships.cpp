// Tests for the relationship and asset-path validation rules.
//
// validateReferences() checks per-field references one record type at a time.
// It cannot see a placed reference whose parent cell came from the load-time
// index rather than the record, a quest stage pointing at nothing, or an asset
// path that climbs out of the data directory. These tests build small synthetic
// plugin graphs and pin each rule and its source location.

#include <QtTest>

#include "../../src/model/tools/assetvalidator.hpp"
#include "../../src/model/world/data.hpp"
#include "../../src/model/world/collection.hpp"
#include "../../src/model/world/record.hpp"
#include "../../src/model/world/ckid.hpp"
#include "../../libs/files/filepaths.hpp"
#include "../../libs/files/esm/cellrecord.hpp"
#include "../../libs/files/esm/dialrecord.hpp"
#include "../../libs/files/esm/inforecord.hpp"
#include "../../libs/files/esm/npcrecord.hpp"
#include "../../libs/files/esm/questrecord.hpp"
#include "../../libs/files/esm/refrecord.hpp"
#include "../../libs/files/esm/sounrecord.hpp"
#include "../../libs/files/esm/statrecord.hpp"
#include "../../libs/components/tier1_components.hpp"

using ValidationIssue = AssetValidator::ValidationIssue;
using ValidationReport = AssetValidator::ValidationReport;

class TestAssetValidatorRelationships : public QObject
{
    Q_OBJECT

private:
    static bool hasIssue(const ValidationReport& report, const QString& category,
        const QString& needle)
    {
        for (const auto& issue : report.issues)
        {
            if (issue.category != category) continue;
            if (issue.message.contains(needle, Qt::CaseInsensitive)) return true;
        }
        return false;
    }

    static int countCategory(const ValidationReport& report, const QString& category)
    {
        int n = 0;
        for (const auto& issue : report.issues)
            if (issue.category == category) ++n;
        return n;
    }

    static NpcRecord makeActor(quint32 formId, const QString& editorId,
        const QString& modelPath)
    {
        NpcRecord rec;
        rec.formId = formId;
        rec.editorId = editorId;
        rec.initComponents();
        // initComponents() does not necessarily seed a model component, so add
        // one rather than assume it.
        auto* model = static_cast<tescomponents::TESModel_Component*>(
            rec.components.findByName(QStringLiteral("TESModel")));
        if (!model)
            model = rec.components.add<tescomponents::TESModel_Component>();
        model->modelPath = modelPath;
        return rec;
    }

    // IdCollection may reassign a FormID when a record is added, so the tests
    // read the real value back instead of assuming the one they set.
    template <typename Coll>
    static quint32 formIdAt(const Coll& collection, int index)
    {
        return collection.getRecord(index).get().formId;
    }

private slots:
    // ---- asset paths ---------------------------------------------------

    void aCleanPathIsAccepted()
    {
        const FilePaths paths;
        Data data(QStringList(), paths);
        data.getNpcCollection().add(makeActor(1, QStringLiteral("Guard"),
            QStringLiteral("meshes\\actors\\guard.nif")));
        const ValidationReport report = AssetValidator::validateAssetPaths(data);
        QCOMPARE(countCategory(report, QStringLiteral("AssetPath")), 0);
    }

    void anEmptyPathIsAccepted()
    {
        const FilePaths paths;
        Data data(QStringList(), paths);
        data.getNpcCollection().add(makeActor(1, QStringLiteral("Guard"), QString()));
        const ValidationReport report = AssetValidator::validateAssetPaths(data);
        QCOMPARE(countCategory(report, QStringLiteral("AssetPath")), 0);
    }

    void aParentHopIsRejected()
    {
        const FilePaths paths;
        Data data(QStringList(), paths);
        data.getNpcCollection().add(makeActor(1, QStringLiteral("Guard"),
            QStringLiteral("meshes\\..\\..\\secrets.nif")));
        const ValidationReport report = AssetValidator::validateAssetPaths(data);
        QVERIFY(hasIssue(report, QStringLiteral("AssetPath"), QStringLiteral("Guard")));
        QCOMPARE(report.issues.first().severity, ValidationIssue::Error);
        // The issue must point at the offending record and path.
        QCOMPARE(report.issues.first().recordId, QStringLiteral("Guard"));
        QVERIFY(report.issues.first().filePath.contains(QStringLiteral("secrets")));
    }

    void aForwardSlashParentHopIsRejected()
    {
        const FilePaths paths;
        Data data(QStringList(), paths);
        StatRecord stat;
        stat.editorId = QStringLiteral("Chair");
        stat.modelPath = QStringLiteral("meshes/furniture/../../outside.nif");
        data.getStatCollection().add(stat);
        const ValidationReport report = AssetValidator::validateAssetPaths(data);
        QVERIFY(hasIssue(report, QStringLiteral("AssetPath"), QStringLiteral("Chair")));
    }

    void anAbsolutePathIsRejected()
    {
        const FilePaths paths;
        Data data(QStringList(), paths);
        SounRecord sound;
        sound.editorId = QStringLiteral("DoorOpen");
        sound.soundFile = QStringLiteral("/sound/fx/door.wav");
        data.getSounCollection().add(sound);
        const ValidationReport report = AssetValidator::validateAssetPaths(data);
        QVERIFY(hasIssue(report, QStringLiteral("AssetPath"), QStringLiteral("DoorOpen")));
    }

    void aDriveQualifiedPathIsRejected()
    {
        const FilePaths paths;
        Data data(QStringList(), paths);
        SounRecord sound;
        sound.editorId = QStringLiteral("DoorShut");
        sound.soundFile = QStringLiteral("C:/games/sound/door.wav");
        data.getSounCollection().add(sound);
        const ValidationReport report = AssetValidator::validateAssetPaths(data);
        QVERIFY(hasIssue(report, QStringLiteral("AssetPath"), QStringLiteral("DoorShut")));
    }

    void actorPathsComeFromComponents()
    {
        const FilePaths paths;
        Data data(QStringList(), paths);
        data.getNpcCollection().add(makeActor(1, QStringLiteral("Thief"),
            QStringLiteral("..\\thief.nif")));
        const ValidationReport report = AssetValidator::validateAssetPaths(data);
        QVERIFY(hasIssue(report, QStringLiteral("AssetPath"), QStringLiteral("Thief")));
        QVERIFY(report.issues.first().message.contains(QStringLiteral("model")));
    }

    // ---- relationships -------------------------------------------------

    void aRefrWithNoParentCellWarns()
    {
        const FilePaths paths;
        Data data(QStringList(), paths);
        RefrRecord ref;
        ref.formId = 0x100;
        ref.editorId = QStringLiteral("Sword01");
        ref.baseId = 0;   // world object, no base record needed
        data.getRefrCollection().add(ref);
        // Deliberately not registered as a child of any cell.

        const ValidationReport report = AssetValidator::validateRelationships(data);
        QVERIFY(hasIssue(report, QStringLiteral("Relationship"),
            QStringLiteral("no parent cell")));
        QCOMPARE(report.issues.first().severity, ValidationIssue::Warning);
    }

    void aRefrWhoseParentCellIsMissingErrors()
    {
        const FilePaths paths;
        Data data(QStringList(), paths);
        RefrRecord ref;
        ref.formId = 0x100;
        ref.editorId = QStringLiteral("Sword01");
        data.getRefrCollection().add(ref);
        const quint32 refId = formIdAt(data.getRefrCollection(), 0);
        data.setRefrParentCell(refId, 0xDEAD);   // a cell that does not exist

        const ValidationReport report = AssetValidator::validateRelationships(data);
        QVERIFY(hasIssue(report, QStringLiteral("Relationship"),
            QStringLiteral("parent cell")));
    }

    void aRefrWithAValidParentCellIsClean()
    {
        const FilePaths paths;
        Data data(QStringList(), paths);
        CellRecord cell;
        cell.formId = 0x200;
        cell.editorId = QStringLiteral("Cell01");
        data.getCellCollection().add(cell);

        RefrRecord ref;
        ref.formId = 0x100;
        ref.editorId = QStringLiteral("Sword01");
        data.getRefrCollection().add(ref);

        const quint32 cellId = formIdAt(data.getCellCollection(), 0);
        const quint32 refId = formIdAt(data.getRefrCollection(), 0);
        QVERIFY(cellId != 0);
        QVERIFY(refId != 0);
        data.setRefrParentCell(refId, cellId);

        const ValidationReport report = AssetValidator::validateRelationships(data);
        QVERIFY2(!hasIssue(report, QStringLiteral("Relationship"),
                   QStringLiteral("Sword01")),
                 qPrintable(QString::number(report.issues.size())
                     + QStringLiteral(" issue(s): ")
                     + (report.issues.isEmpty() ? QString()
                        : report.issues.first().message)));
    }

    void aCellWithAMissingOwnerErrors()
    {
        const FilePaths paths;
        Data data(QStringList(), paths);
        CellRecord cell;
        cell.formId = 0x200;
        cell.editorId = QStringLiteral("Cell01");
        cell.owner = 0xBEEF;   // no such actor
        data.getCellCollection().add(cell);

        const ValidationReport report = AssetValidator::validateRelationships(data);
        QVERIFY(hasIssue(report, QStringLiteral("Relationship"),
            QStringLiteral("Cell01")));
    }

    void questAndDialogueLinksAreChecked()
    {
        const FilePaths paths;
        Data data(QStringList(), paths);
        QuestRecord quest;
        quest.formId = 0x300;
        quest.editorId = QStringLiteral("FindThing");
        quest.stageIds = { 0xDEAD1u };
        quest.objectiveIds = { 0xDEAD2u };
        data.getQuestCollection().add(quest);

        DialRecord topic;
        topic.formId = 0x400;
        topic.editorId = QStringLiteral("AskAboutThing");
        topic.responseIds = { 0xDEAD3u };
        data.getDialCollection().add(topic);

        InfoRecord response;
        response.formId = 0x500;
        response.editorId = QStringLiteral("Response01");
        response.targetId = 0xDEAD4u;
        data.getInfoCollection().add(response);

        const ValidationReport report = AssetValidator::validateRelationships(data);
        // One issue per dangling link: 2 quest + 1 topic + 1 response.
        QCOMPARE(countCategory(report, QStringLiteral("Relationship")), 4);
        QVERIFY(hasIssue(report, QStringLiteral("Relationship"), QStringLiteral("stage")));
        QVERIFY(hasIssue(report, QStringLiteral("Relationship"), QStringLiteral("objective")));
        QVERIFY(hasIssue(report, QStringLiteral("Relationship"), QStringLiteral("response")));
        QVERIFY(hasIssue(report, QStringLiteral("Relationship"), QStringLiteral("target topic")));
    }

    void aFullyWiredQuestIsClean()
    {
        const FilePaths paths;
        Data data(QStringList(), paths);
        QuestRecord quest;
        quest.formId = 0x300;
        quest.editorId = QStringLiteral("FindThing");
        quest.stageIds = { 0x600u };
        data.getQuestCollection().add(quest);

        DialRecord topic;
        topic.formId = 0x400;
        topic.editorId = QStringLiteral("AskAboutThing");
        topic.responseIds = { 0x500u };
        data.getDialCollection().add(topic);

        InfoRecord response;
        response.formId = 0x500;
        response.editorId = QStringLiteral("Response01");
        data.getInfoCollection().add(response);

        // Something for the quest stage to point at, so it is not dangling.
        NpcRecord actor;
        actor.formId = 0x600;
        actor.editorId = QStringLiteral("Victim");
        data.getNpcCollection().add(actor);

        const ValidationReport report = AssetValidator::validateRelationships(data);
        QCOMPARE(countCategory(report, QStringLiteral("Relationship")), 0);
    }

    // ---- save policy ---------------------------------------------------

    void savePolicyBlocksOnAnyError()
    {
        ValidationReport clean;
        QVERIFY(!AssetValidator::shouldBlockSave(clean));

        ValidationReport warning;
        warning.issues.append({ValidationIssue::Warning, "X", "w", "", ""});
        QVERIFY(!AssetValidator::shouldBlockSave(warning));

        ValidationReport bad;
        bad.issues.append({ValidationIssue::Warning, "X", "w", "", ""});
        bad.issues.append({ValidationIssue::Error, "X", "e", "", ""});
        QVERIFY(AssetValidator::shouldBlockSave(bad));
    }

    void savePolicyHonoursAnErrorAllowance()
    {
        ValidationReport one;
        one.issues.append({ValidationIssue::Error, "X", "e", "", ""});
        QVERIFY(AssetValidator::shouldBlockSave(one, 0));
        QVERIFY(!AssetValidator::shouldBlockSave(one, 1));
    }

    // Passing an invalid data directory used to send the asset scan from an
    // invalid root: five minutes of walking, then a stack overrun. It must now
    // bail out immediately with a warning instead.
    void validateAllWithoutADataDirectoryReturnsPromptly()
    {
        const FilePaths paths;
        Data data(QStringList(), paths);

        QElapsedTimer timer;
        timer.start();
        const ValidationReport report = AssetValidator::validateAll(data, QString());
        const qint64 elapsed = timer.elapsed();

        QVERIFY2(elapsed < 5000,
            qPrintable(QStringLiteral("took %1 ms").arg(elapsed)));
        QVERIFY(hasIssue(report, QStringLiteral("Validation"),
            QStringLiteral("No data directory")));
    }

    // A data directory that does not exist must be treated the same way.
    void validateAllWithAMissingDataDirectoryReturnsPromptly()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString missing = dir.path() + QStringLiteral("/does-not-exist");
        const FilePaths paths;
        Data data(QStringList(), paths);

        QElapsedTimer timer;
        timer.start();
        const ValidationReport report = AssetValidator::validateAll(data, missing);
        const qint64 elapsed = timer.elapsed();

        QVERIFY2(elapsed < 5000,
            qPrintable(QStringLiteral("took %1 ms").arg(elapsed)));
        QVERIFY(hasIssue(report, QStringLiteral("Validation"),
            QStringLiteral("No data directory")));
    }

    void validateAllRunsTheNewRules()
    {
        // validateAll() walks the data directory, so it needs a real one. An
        // empty path makes it scan from an invalid root, which takes minutes
        // and then overruns the stack.
        QTemporaryDir dataDir;
        QVERIFY(dataDir.isValid());
        const FilePaths paths;
        Data data(QStringList(), paths);
        CellRecord cell;
        cell.formId = 0x200;
        cell.editorId = QStringLiteral("Cell01");
        cell.owner = 0xBEEF;
        data.getCellCollection().add(cell);

        const ValidationReport report =
            AssetValidator::validateAll(data, dataDir.path());
        QVERIFY(hasIssue(report, QStringLiteral("Relationship"), QStringLiteral("Cell01")));
    }
};

QTEST_MAIN(TestAssetValidatorRelationships)
#include "test_assetvalidatorrelationships.moc"
