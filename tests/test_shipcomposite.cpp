#include <QtTest>
#include <QFileInfo>
#include <QTemporaryFile>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "gbfmrecord.hpp"
#include "constructibleobjectrecord.hpp"
#include "formlistrecord.hpp"
#include "common.hpp"
#include "logger.hpp"
#include "model/tools/shippartcodec.hpp"

// Validates the Starfield ship composite (REMAINING.md §3.8): GBFM BFCB
// component parsing, byte-exact round-trip, and the COBJ -> FLST -> GBFM
// resolver, all against the real Starfield.esm.
class TestShipComposite : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testGbfmComponentParsing();
    void testGbfmRoundTrip();
    void testApplyToGbfmNoOp();
    void testResolveRealChain();

private:
    QString esmPath() const;
};

// Scans `path` for records whose four-char name is `want`, invoking onRecord
// (which must consume the record) until it returns true. The file orders
// groups GBFM -> FLST -> COBJ, so resolving a chain needs one pass per type;
// this keeps each pass explicit rather than buffering the whole master.
template<typename PredFn>
static bool scanTyped(const QString& path, NAME want, PredFn predFn)
{
    ESMReader reader(path);
    reader.open();
    while (reader.isLeft())
    {
        NAME name = 0;
        try { name = reader.readName(); } catch (...) { return false; }
        if (name == 0) return false;
        if (name == (NAME)'GRUP') { reader.skipGrupHeader(); continue; }
        if (name != want) { reader.skipRecord(); continue; }
        if (predFn(reader))
            return true;
    }
    return false;
}

void TestShipComposite::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_shipcomposite_log.txt"));
}

QString TestShipComposite::esmPath() const
{
    return qEnvironmentVariable("OPENCK_DATA_DIR",
               QStringLiteral("C:/XboxGames/Starfield/Content/Data"))
        + "/Starfield.esm";
}

void TestShipComposite::testGbfmComponentParsing()
{
    const QString path = esmPath();
    if (!QFileInfo::exists(path)) QSKIP("Starfield.esm not found");

    ESMReader reader(path);
    reader.open();

    int parsed = 0;
    int withFullName = 0;
    int withKeywords = 0;
    int withLinks = 0;
    int failures = 0;
    while (parsed < 40 && reader.isLeft())
    {
        NAME name = 0;
        try { name = reader.readName(); } catch (...) { break; }
        if (name == 0) break;
        if (name == (NAME)'GRUP') { reader.skipGrupHeader(); continue; }
        if (name != (NAME)'GBFM') { reader.skipRecord(); continue; }

        GbfmRecord rec;
        rec.load(reader, true);

        const QVector<GbfmComponent> comps = rec.parseComponents();
        if (!comps.isEmpty())
        {
            // Everything inside a component must belong to a named component,
            // and the marker payload itself must not leak in as a subrecord.
            for (const GbfmComponent& c : comps)
            {
                if (c.typeName.isEmpty())
                    ++failures;
                for (const RawSubRecord& r : c.subrecords)
                    if (r.name == NAME('BFCB') || r.name == NAME('BFCE'))
                        ++failures;
            }
        }
        if (rec.fullNameStringId() != 0) ++withFullName;
        if (!rec.keywordFormIds().isEmpty()) ++withKeywords;
        if (!rec.linkedFormIds().isEmpty())
        {
            ++withLinks;
            // FLKW/FLFM come in pairs in the surveyed data.
            if (rec.linkedKeywordIds().size() != rec.linkedFormIds().size())
                ++failures;
        }

        // The component list must contain the union of typed component names.
        if (rec.fullNameStringId() != 0
            && !rec.componentTypeNames().contains(QStringLiteral("TESFullName_Component")))
            ++failures;

        ++parsed;
    }

    qDebug() << "GBFM parsed:" << parsed
             << "with FullName:" << withFullName
             << "with keywords:" << withKeywords
             << "with form links:" << withLinks
             << "failures:" << failures;
    QVERIFY(parsed > 0);
    QVERIFY(withFullName > 0);
    QCOMPARE(failures, 0);
}

void TestShipComposite::testGbfmRoundTrip()
{
    const QString path = esmPath();
    if (!QFileInfo::exists(path)) QSKIP("Starfield.esm not found");

    ESMReader reader(path);
    reader.open();

    int roundTripped = 0;
    int failures = 0;
    while (roundTripped < 15 && reader.isLeft())
    {
        NAME name = 0;
        try { name = reader.readName(); } catch (...) { break; }
        if (name == 0) break;
        if (name == (NAME)'GRUP') { reader.skipGrupHeader(); continue; }
        if (name != (NAME)'GBFM') { reader.skipRecord(); continue; }

        GbfmRecord rec;
        rec.load(reader, true);

        QTemporaryFile written;
        QVERIFY(written.open());
        {
            ESMWriter writer;
            writer.setAuthor("Test");
            writer.save(written);
            RecHeader rh;
            rh.id = rec.formId;
            writer.startRecord('GBFM', rh);
            rec.save(writer);
            writer.endRecord();
            writer.close();
        }
        written.close();

        ESMReader check(written.fileName());
        check.open();
        bool found = false;
        while (check.isLeft())
        {
            NAME cname = check.readName();
            if (cname == 0) break;
            if (cname == (NAME)'GRUP') { check.skipGrupHeader(); continue; }
            if (cname != (NAME)'GBFM') { check.skipRecord(); continue; }

            GbfmRecord back;
            back.load(check, true);
            found = true;
            if (rec.editorId != back.editorId || rec.formId != back.formId
                || rec.rawSubRecords.size() != back.rawSubRecords.size())
            {
                ++failures;
                qWarning() << "GBFM scalar mismatch on" << rec.editorId;
            }
            for (int i = 0; i < rec.rawSubRecords.size(); ++i)
            {
                if (rec.rawSubRecords[i].name != back.rawSubRecords[i].name
                    || rec.rawSubRecords[i].data != back.rawSubRecords[i].data)
                {
                    ++failures;
                    qWarning() << "GBFM raw mismatch on" << rec.editorId
                               << "at" << i;
                }
            }
            ++roundTripped;
            break;
        }
        if (!found)
            ++failures;
    }

    qDebug() << "GBFM round-tripped:" << roundTripped << "failures:" << failures;
    QVERIFY(roundTripped > 0);
    QCOMPARE(failures, 0);
}

void TestShipComposite::testApplyToGbfmNoOp()
{
    const QString path = esmPath();
    if (!QFileInfo::exists(path)) QSKIP("Starfield.esm not found");

    ESMReader reader(path);
    reader.open();

    int checked = 0;
    int failures = 0;
    while (checked < 15 && reader.isLeft())
    {
        NAME name = 0;
        try { name = reader.readName(); } catch (...) { break; }
        if (name == 0) break;
        if (name == (NAME)'GRUP') { reader.skipGrupHeader(); continue; }
        if (name != (NAME)'GBFM') { reader.skipRecord(); continue; }

        GbfmRecord rec;
        rec.load(reader, true);
        const QVector<RawSubRecord> before = rec.rawSubRecords;

        // Applying the record's own definition back must be a byte-exact no-op.
        const ShipPartDefinition def = ShipPartCodec::fromGbfm(rec);
        ShipPartCodec::applyToGbfm(def, rec);

        if (rec.rawSubRecords.size() != before.size())
        {
            ++failures;
            qWarning() << "applyToGbfm resized" << rec.editorId;
        }
        else
        {
            for (int i = 0; i < before.size(); ++i)
            {
                if (before[i].name != rec.rawSubRecords[i].name
                    || before[i].data != rec.rawSubRecords[i].data)
                {
                    ++failures;
                    qWarning() << "applyToGbfm changed bytes on" << rec.editorId
                               << "at" << i;
                }
            }
        }
        ++checked;
    }

    qDebug() << "applyToGbfm no-op checked:" << checked << "failures:" << failures;
    QVERIFY(checked > 0);
    QCOMPARE(failures, 0);
}

void TestShipComposite::testResolveRealChain()
{
    const QString path = esmPath();
    if (!QFileInfo::exists(path)) QSKIP("Starfield.esm not found");

    // The file orders groups GBFM -> FLST -> COBJ, so walk it once per type:
    // COBJ first to learn the chain, then FLST, then the GBFM variants.
    QString cobjEditorId;
    quint32 cobjFormId = 0;
    quint32 needFlst = 0;
    scanTyped(path, NAME('COBJ'), [&](ESMReader& reader) {
            CobjRecord cobj;
            cobj.load(reader, true);
            if (!cobj.editorId.startsWith(QStringLiteral("co_SMS_")))
                return false;
            const quint32 cnam = cobj.createdObjectId();
            if (cnam == 0)
                return false;
            cobjEditorId = cobj.editorId;
            cobjFormId = cobj.formId;
            needFlst = cnam;
            return true;
        });
    if (cobjEditorId.isEmpty())
        QSKIP("No co_SMS_ COBJ with a CNAM found");

    // Pass 2: the form list the recipe points at.
    FormListRecord flst;
    bool haveFlst = false;
    scanTyped(path, NAME('FLST'), [&](ESMReader& reader) {
            FormListRecord candidate;
            candidate.load(reader, true);
            if (candidate.formId != needFlst)
                return false;
            flst = candidate;
            haveFlst = true;
            return true;
        });
    QVERIFY(haveFlst);
    QVERIFY(!flst.formIds.isEmpty());

    // Pass 3: every GBFM variant the form list names.
    QVector<quint32> remaining = flst.formIds;
    IdCollection<GbfmRecord> gbfms;
    scanTyped(path, NAME('GBFM'), [&](ESMReader& reader) {
            GbfmRecord gbfm;
            gbfm.load(reader, true);
            if (!remaining.contains(gbfm.formId))
                return false;
            gbfms.load(gbfm, -1, true);
            remaining.removeAll(gbfm.formId);
            return remaining.isEmpty();
        });

    IdCollection<CobjRecord> cobjs;
    CobjRecord cobj;
    cobj.editorId = cobjEditorId;
    cobj.formId = cobjFormId;
    // Rebuild the CNAM from the form list id for the resolver.
    RawSubRecord cnam;
    cnam.name = NAME('CNAM');
    cnam.data.resize(4);
    cnam.data[0] = static_cast<char>(needFlst & 0xFF);
    cnam.data[1] = static_cast<char>((needFlst >> 8) & 0xFF);
    cnam.data[2] = static_cast<char>((needFlst >> 16) & 0xFF);
    cnam.data[3] = static_cast<char>((needFlst >> 24) & 0xFF);
    cobj.rawSubRecords.append(cnam);
    cobjs.load(cobj, -1, true);

    IdCollection<FormListRecord> flsts;
    flsts.load(flst, -1, true);

    qDebug() << "resolving COBJ:" << cobjEditorId
             << "FLST:" << flst.editorId
             << "variants:" << flst.formIds.size()
             << "GBFM collected:" << gbfms.size();

    ShipComposite composite;
    QVERIFY(ShipCompositeResolver::resolve(cobjs, flsts, gbfms,
                                           cobjEditorId, composite));
    QCOMPARE(composite.cobjEditorId, cobjEditorId);
    QCOMPARE(composite.cobjFormId, cobjFormId);
    QCOMPARE(composite.formListFormId, needFlst);
    QCOMPARE(composite.formListEditorId, flst.editorId);
    QCOMPARE(composite.variantFormIds, flst.formIds);
    QCOMPARE(composite.variantEditorIds.size(), flst.formIds.size());

    // At least one variant must resolve to a GBFM with real components.
    int resolved = 0;
    for (int i = 0; i < composite.variantEditorIds.size(); ++i)
    {
        if (composite.variantEditorIds[i].isEmpty())
            continue;
        ++resolved;
        const GbfmRecord* gbfm =
            ShipCompositeResolver::findGbfmByEditorId(gbfms,
                composite.variantEditorIds[i]);
        QVERIFY(gbfm != nullptr);
        const ShipPartDefinition def = ShipPartCodec::fromGbfm(*gbfm);
        QVERIFY(!def.componentTypes.isEmpty());
        QCOMPARE(def.editorId, composite.variantEditorIds[i]);
    }
    QVERIFY(resolved > 0);

    // Resolving an unknown COBJ must fail cleanly.
    ShipComposite missing;
    QVERIFY(!ShipCompositeResolver::resolve(cobjs, flsts, gbfms,
                                            QStringLiteral("co_DoesNotExist"),
                                            missing));
}

QTEST_MAIN(TestShipComposite)
#include "test_shipcomposite.moc"
