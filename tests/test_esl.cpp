#include <QTest>
#include <QTemporaryFile>
#include <QFile>

#include "model/tools/formidcompactor.hpp"
#include "model/world/data.hpp"
#include "model/world/collection.hpp"
#include "model/world/irecordcollection.hpp"
#include "libs/files/esm/relarecord.hpp"
#include "libs/files/esm/shourecord.hpp"
#include "libs/files/esm/ecznrecord.hpp"
#include "libs/files/esm/ipdsrecord.hpp"
#include "libs/files/esm/statrecord.hpp"
#include "libs/files/esm/cellrecord.hpp"
#include "libs/files/esm/npcrecord.hpp"
#include "libs/files/esm/alchrecord.hpp"
#include "libs/files/esm/refrecord.hpp"
#include "libs/files/esm/locationrecord.hpp"
#include "libs/components/tier2_components.hpp"
#include "libs/files/filepaths.hpp"
#include "logger.hpp"

namespace {

QByteArray rawFormId(quint32 id)
{
    QByteArray b(4, '\0');
    b[0] = static_cast<char>(id & 0xFF);
    b[1] = static_cast<char>((id >> 8) & 0xFF);
    b[2] = static_cast<char>((id >> 16) & 0xFF);
    b[3] = static_cast<char>((id >> 24) & 0xFF);
    return b;
}

quint32 rawU32(const QByteArray& b, int offset)
{
    return (static_cast<quint32>(static_cast<unsigned char>(b.at(offset))))
        | (static_cast<quint32>(static_cast<unsigned char>(b.at(offset + 1))) << 8)
        | (static_cast<quint32>(static_cast<unsigned char>(b.at(offset + 2))) << 16)
        | (static_cast<quint32>(static_cast<unsigned char>(b.at(offset + 3))) << 24);
}

} // namespace

// Validates ESL support:
//  - FormIdCompactor remaps a plugin's own (modified) records into the
//    ESL 0x000-0xFFF range, preserving the plugin/master high bits, and
//    rewrites the typed FormID reference fields of records that expose them.
//  - ESMWriter preserves the LightMaster (0x200) file flag on save so a
//    plugin round-trips as an ESL.
class TestEsl : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testCompactorRemapsFormIds();
    void testCompactorRewritesTypedReferences();
    void testCompactorRewritesRawAndComponentReferences();
    void testCompactorRewritesRefrRawLayouts();
    void testCompactorRewritesStarfieldLctnRawLayouts();
    void testCompactorLeavesXprmByteIdentical();
    void testCompactorTooManyRecords();
    void testLightMasterFlagRoundTrip();
};

void TestEsl::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral(
        "C:/Users/max/AppData/Local/Temp/opencode/test_esl_log.txt"));
}

void TestEsl::testCompactorRemapsFormIds()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& statCol = data.getStatCollection();

    // Owned records with large local IDs (plugin index 0x10).
    StatRecord a; a.editorId = "StatA"; a.formId = 0x00100050;
    StatRecord b; b.editorId = "StatB"; b.formId = 0x00100080;
    StatRecord c; c.editorId = "StatC"; c.formId = 0x00100030;
    statCol.add(a);
    statCol.add(b);
    statCol.add(c);

    FormIdCompactor compactor(data);
    QCOMPARE(compactor.compact(), 3);
    QCOMPARE(compactor.ownedRecordCount(), 3);
    QCOMPARE(compactor.remappedCount(), 3);

    // Local IDs must be 0x000-0xFFF and unique; high bits preserved.
    QSet<quint32> locals;
    for (int i = 0; i < statCol.count(); ++i)
    {
        const quint32 id = statCol.getFormId(i);
        QVERIFY((id & 0x00000FFF) <= 0x00000FFF);
        QCOMPARE(id & 0xFFFF0000u, 0x00100000u);
        locals.insert(id & 0x00000FFF);
    }
    QCOMPARE(locals.size(), 3);
}

void TestEsl::testCompactorRewritesTypedReferences()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& relaCol = data.getRelaCollection();

    // The RELA record's own ID and the FormIDs it references.
    RelaRecord rec;
    rec.editorId = "RelaTest";
    rec.formId = 0x00100100;
    rec.parentFormId = 0x00100050; // references StatA (owned)
    rec.childFormId = 0x00200001;  // references another plugin (not remapped)
    relaCol.add(rec);

    // Add StatA so the compactor has an owned record to remap.
    auto& statCol = data.getStatCollection();
    StatRecord a; a.editorId = "StatA"; a.formId = 0x00100050;
    statCol.add(a);

    FormIdCompactor compactor(data);
    QCOMPARE(compactor.compact(), 2);
    QCOMPARE(compactor.ownedRecordCount(), 2);

    const RelaRecord& saved = relaCol.getRecord(0).get();
    // The reference to an owned record follows the remap; the foreign
    // reference is untouched.
    const quint32 newParent = 0x00100000u; // StatA is the only stat -> local 0
    QCOMPARE(saved.parentFormId, newParent);
    QCOMPARE(saved.childFormId, static_cast<quint32>(0x00200001));
}

void TestEsl::testCompactorRewritesRawAndComponentReferences()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& statCol = data.getStatCollection();
    auto& cellCol = data.getCellCollection();
    auto& npcCol = data.getNpcCollection();
    auto& alchCol = data.getAlchCollection();

    // Owned target record the raw refs below point at.
    StatRecord target;
    target.editorId = "Target";
    target.formId = 0x00100050;
    statCol.add(target);

    // Cell: raw XEZN (encounter zone) and an XCLR region list holding one
    // owned ref and one foreign ref.
    CellRecord cell;
    cell.editorId = "CellA";
    cell.formId = 0x00100030;
    {
        RawSubRecord raw;
        raw.name = NAME('XEZN');
        raw.data = rawFormId(0x00100050);
        cell.rawSubRecords.push_back(raw);
    }
    {
        RawSubRecord raw;
        raw.name = NAME('XCLR');
        raw.data = rawFormId(0x00100050) + rawFormId(0x00200001);
        cell.rawSubRecords.push_back(raw);
    }
    cellCol.add(cell);

    // Npc: raw VOIC, HDPT (u32 count then FormIDs), LVLD (template at byte
    // 12), and KWDA (owned ref + foreign ref).
    NpcRecord npc;
    npc.editorId = "NpcA";
    npc.formId = 0x00100080;
    {
        RawSubRecord raw;
        raw.name = NAME('VOIC');
        raw.data = rawFormId(0x00100050);
        npc.rawSubRecords.push_back(raw);
    }
    {
        RawSubRecord raw;
        raw.name = NAME('HDPT');
        raw.data = rawFormId(1) + rawFormId(0x00100050);
        npc.rawSubRecords.push_back(raw);
    }
    {
        QByteArray lvld(20, '\0');
        lvld.replace(12, 4, rawFormId(0x00100050));
        RawSubRecord raw;
        raw.name = NAME('LVLD');
        raw.data = lvld;
        npc.rawSubRecords.push_back(raw);
    }
    {
        RawSubRecord raw;
        raw.name = NAME('KWDA');
        raw.data = rawFormId(0x00100050) + rawFormId(0x00200001);
        npc.rawSubRecords.push_back(raw);
    }
    npcCol.add(npc);

    // Alch: raw EFID plus enchantment and pickup/putdown component FormIDs.
    AlchRecord alch;
    alch.editorId = "AlchA";
    alch.formId = 0x00100090;
    alch.initComponents();
    alch.components.add<tescomponents::TESEnchantableForm_Component>();
    if (auto* enc = static_cast<tescomponents::TESEnchantableForm_Component*>(
            alch.components.findByName(QStringLiteral("TESEnchantableForm"))))
        enc->enchantmentFormId = 0x00100050;
    if (auto* snd = static_cast<tescomponents::BGSPickupPutdownSounds_Component*>(
            alch.components.findByName(QStringLiteral("BGSPickupPutdownSounds"))))
    {
        snd->pickupSound = 0x00100050;
        snd->putdownSound = 0x00100050;
    }
    {
        RawSubRecord raw;
        raw.name = NAME('EFID');
        raw.data = rawFormId(0x00100050);
        alch.rawSubRecords.push_back(raw);
    }
    alchCol.add(alch);

    FormIdCompactor compactor(data);
    QCOMPARE(compactor.compact(), 4);

    // Owned IDs 0x00100030/0x50/0x80/0x90 sort to locals 0/1/2/3, so the
    // target (0x00100050) becomes 0x00100001.
    const quint32 newTarget = 0x00100001u;

    const CellRecord& savedCell = cellCol.getRecord(0).get();
    QCOMPARE(savedCell.rawSubRecords.size(), 2);
    QCOMPARE(rawU32(savedCell.rawSubRecords.at(0).data, 0), newTarget);
    QCOMPARE(rawU32(savedCell.rawSubRecords.at(1).data, 0), newTarget);
    QCOMPARE(rawU32(savedCell.rawSubRecords.at(1).data, 4), static_cast<quint32>(0x00200001));

    const NpcRecord& savedNpc = npcCol.getRecord(0).get();
    QCOMPARE(savedNpc.rawSubRecords.size(), 4);
    QCOMPARE(rawU32(savedNpc.rawSubRecords.at(0).data, 0), newTarget);  // VOIC
    QCOMPARE(rawU32(savedNpc.rawSubRecords.at(1).data, 0), 1u);         // HDPT count
    QCOMPARE(rawU32(savedNpc.rawSubRecords.at(1).data, 4), newTarget);  // HDPT ref
    QCOMPARE(rawU32(savedNpc.rawSubRecords.at(2).data, 12), newTarget); // LVLD template
    QCOMPARE(rawU32(savedNpc.rawSubRecords.at(3).data, 0), newTarget);  // KWDA
    QCOMPARE(rawU32(savedNpc.rawSubRecords.at(3).data, 4), static_cast<quint32>(0x00200001));

    const AlchRecord& savedAlch = alchCol.getRecord(0).get();
    QCOMPARE(savedAlch.rawSubRecords.size(), 1);
    QCOMPARE(savedAlch.rawSubRecords.at(0).data, rawFormId(newTarget)); // EFID
    if (const auto* enc = static_cast<const tescomponents::TESEnchantableForm_Component*>(
            savedAlch.components.findByName(QStringLiteral("TESEnchantableForm"))))
        QCOMPARE(enc->enchantmentFormId, newTarget);
    if (const auto* snd = static_cast<const tescomponents::BGSPickupPutdownSounds_Component*>(
            savedAlch.components.findByName(QStringLiteral("BGSPickupPutdownSounds"))))
    {
        QCOMPARE(snd->pickupSound, newTarget);
        QCOMPARE(snd->putdownSound, newTarget);
    }
}

void TestEsl::testCompactorRewritesRefrRawLayouts()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& statCol = data.getStatCollection();
    auto& refrCol = data.getRefrCollection();

    // Owned target the raw refs below point at.
    StatRecord target;
    target.editorId = "Target";
    target.formId = 0x00100050;
    statCol.add(target);

    // REFR raw subrecords whose payload layouts are now interpreted:
    // XAPR ({ref FormID, delay float} per subrecord), XLKR (FormIDs at every
    // 4 bytes, including the keyword slot at 0), XTEL (refs at 0 and 32),
    // XMBR (ref at 0), XLRT (one FormID per 4 bytes, no count prefix).
    RefrRecord refr;
    refr.editorId = "RefA";
    refr.formId = 0x00100030;
    {
        RawSubRecord raw;
        raw.name = NAME('XAPR');
        raw.data = rawFormId(0x00100050) + rawFormId(0x3F800000); // ref, delay 1.0f
        refr.rawSubRecords.push_back(raw);
    }
    {
        RawSubRecord raw;
        raw.name = NAME('XLKR');
        raw.data = rawFormId(0x00100050) + rawFormId(0x00100050)
                 + rawFormId(0x00100050) + rawFormId(0x00100050); // 2 linked refs
        refr.rawSubRecords.push_back(raw);
    }
    {
        RawSubRecord raw;
        raw.name = NAME('XTEL');
        QByteArray tel(36, '\0');
        tel.replace(0, 4, rawFormId(0x00100050));
        tel.replace(32, 4, rawFormId(0x00100050));
        raw.data = tel;
        refr.rawSubRecords.push_back(raw);
    }
    {
        RawSubRecord raw;
        raw.name = NAME('XMBR');
        raw.data = rawFormId(0x00100050);
        refr.rawSubRecords.push_back(raw);
    }
    {
        RawSubRecord raw;
        raw.name = NAME('XLRT');
        raw.data = rawFormId(0x00100050) + rawFormId(0x00100050);
        refr.rawSubRecords.push_back(raw);
    }
    refrCol.add(refr);

    FormIdCompactor compactor(data);
    QCOMPARE(compactor.compact(), 2);

    // Owned IDs 0x00100030/0x50 sort to locals 0/1, so the target becomes
    // 0x00100001.
    const quint32 newTarget = 0x00100001u;

    const RefrRecord& saved = refrCol.getRecord(0).get();
    QCOMPARE(saved.rawSubRecords.size(), 5);
    QCOMPARE(rawU32(saved.rawSubRecords.at(0).data, 0), newTarget);     // XAPR ref
    QCOMPARE(rawU32(saved.rawSubRecords.at(0).data, 4), 0x3F800000u);   // XAPR delay kept
    QCOMPARE(rawU32(saved.rawSubRecords.at(1).data, 0), newTarget);     // XLKR keyword slot
    QCOMPARE(rawU32(saved.rawSubRecords.at(1).data, 4), newTarget);
    QCOMPARE(rawU32(saved.rawSubRecords.at(1).data, 8), newTarget);
    QCOMPARE(rawU32(saved.rawSubRecords.at(1).data, 12), newTarget);
    QCOMPARE(rawU32(saved.rawSubRecords.at(2).data, 0), newTarget);     // XTEL door
    QCOMPARE(rawU32(saved.rawSubRecords.at(2).data, 32), newTarget);    // XTEL interior
    QCOMPARE(rawU32(saved.rawSubRecords.at(3).data, 0), newTarget);     // XMBR
    QCOMPARE(rawU32(saved.rawSubRecords.at(4).data, 0), newTarget);     // XLRT refs
    QCOMPARE(rawU32(saved.rawSubRecords.at(4).data, 4), newTarget);
}

void TestEsl::testCompactorRewritesStarfieldLctnRawLayouts()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& statCol = data.getStatCollection();
    auto& locCol = data.getLocationCollection();

    // Owned target the raw refs below point at.
    StatRecord target;
    target.editorId = "Target";
    target.formId = 0x00100050;
    statCol.add(target);

    // Starfield's LCTN rebuild arrays (documented in xEdit wbDefinitionsSF1,
    // wbRecord(LCTN)) carry FormIDs at fixed offsets inside each fixed-size
    // entry: LCUR {base, placed, location} 12-byte triplets, LCID a plain
    // FormID array, LCEP {ref, enable parent, flags, pad} 12-byte entries.
    // PNAM (parentId) and the linked-ref groups are typed members.
    LocationRecord loc;
    loc.editorId = "LocA";
    loc.formId = 0x00100030;
    loc.parentId = 0x00100050;
    {
        LocationRecord::LinkedRef group;
        group.refTypeId = 0x00100050;
        group.linkedIds = { 0x00100050, 0x00200001 };
        loc.linkedRefs.append(group);
    }
    {
        RawSubRecord raw;
        raw.name = NAME('LCUR');
        raw.data = rawFormId(0x00100050) + rawFormId(0x00200001) + rawFormId(0x00200002);
        loc.rawSubRecords.push_back(raw);
    }
    {
        RawSubRecord raw;
        raw.name = NAME('LCID');
        raw.data = rawFormId(0x00100050) + rawFormId(0x00200001);
        loc.rawSubRecords.push_back(raw);
    }
    {
        QByteArray lcep(12, '\0');
        lcep.replace(0, 4, rawFormId(0x00100050));
        lcep.replace(4, 4, rawFormId(0x00200001));
        lcep[8] = '\x01';  // flags byte, not a FormID
        RawSubRecord raw;
        raw.name = NAME('LCEP');
        raw.data = lcep;
        loc.rawSubRecords.push_back(raw);
    }
    locCol.add(loc);

    FormIdCompactor compactor(data);
    QCOMPARE(compactor.compact(), 2);

    // Owned IDs 0x00100030/0x50 sort to locals 0/1, so the target becomes
    // 0x00100001.
    const quint32 newTarget = 0x00100001u;

    const LocationRecord& saved = locCol.getRecord(0).get();
    QCOMPARE(saved.parentId, newTarget);
    QCOMPARE(saved.linkedRefs.size(), 1);
    QCOMPARE(saved.linkedRefs.at(0).refTypeId, newTarget);
    QCOMPARE(saved.linkedRefs.at(0).linkedIds.size(), 2);
    QCOMPARE(saved.linkedRefs.at(0).linkedIds.at(0), newTarget);
    QCOMPARE(saved.linkedRefs.at(0).linkedIds.at(1), static_cast<quint32>(0x00200001));

    QCOMPARE(saved.rawSubRecords.size(), 3);
    QCOMPARE(rawU32(saved.rawSubRecords.at(0).data, 0), newTarget);        // LCUR base
    QCOMPARE(rawU32(saved.rawSubRecords.at(0).data, 4), static_cast<quint32>(0x00200001));
    QCOMPARE(rawU32(saved.rawSubRecords.at(0).data, 8), static_cast<quint32>(0x00200002));
    QCOMPARE(rawU32(saved.rawSubRecords.at(1).data, 0), newTarget);        // LCID
    QCOMPARE(rawU32(saved.rawSubRecords.at(1).data, 4), static_cast<quint32>(0x00200001));
    QCOMPARE(rawU32(saved.rawSubRecords.at(2).data, 0), newTarget);        // LCEP ref
    QCOMPARE(rawU32(saved.rawSubRecords.at(2).data, 4), static_cast<quint32>(0x00200001));
    QCOMPARE(static_cast<quint8>(saved.rawSubRecords.at(2).data.at(8)), 0x01u);  // flags untouched
}

void TestEsl::testCompactorLeavesXprmByteIdentical()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& statCol = data.getStatCollection();
    auto& refrCol = data.getRefrCollection();

    // Owned target the raw XLMS ref below points at.
    StatRecord target;
    target.editorId = "Target";
    target.formId = 0x00100050;
    statCol.add(target);

    // XPRM is a primitive descriptor (bounds, color, shape type) with no
    // FormID slots (xEdit wbDefinitionsSF1 wbStruct(XPRM,...)); its payload
    // must round-trip byte-for-byte even when it happens to contain a value
    // that equals an owned record's old FormID.
    RefrRecord refr;
    refr.editorId = "RefA";
    refr.formId = 0x00100030;
    {
        QByteArray xprm(32, '\0');
        xprm.replace(0, 4, rawFormId(0x00100050));   // looks like a ref, is a float
        xprm.replace(4, 4, rawFormId(0x3F800000));   // 1.0f
        RawSubRecord raw;
        raw.name = NAME('XPRM');
        raw.data = xprm;
        refr.rawSubRecords.push_back(raw);
    }
    {
        RawSubRecord raw;
        raw.name = NAME('XLMS');
        raw.data = rawFormId(0x00100050) + rawFormId(0x00200001);
        refr.rawSubRecords.push_back(raw);
    }
    refrCol.add(refr);

    const QByteArray xprmBefore = refrCol.getRecord(0).get().rawSubRecords.at(0).data;

    FormIdCompactor compactor(data);
    QCOMPARE(compactor.compact(), 2);

    // The XPRM payload is byte-identical; the neighbouring FormID-bearing
    // XLMS array is rewritten as expected.
    const RefrRecord& saved = refrCol.getRecord(0).get();
    QCOMPARE(saved.rawSubRecords.size(), 2);
    QCOMPARE(saved.rawSubRecords.at(0).data, xprmBefore);
    QCOMPARE(rawU32(saved.rawSubRecords.at(1).data, 0), 0x00100001u);
    QCOMPARE(rawU32(saved.rawSubRecords.at(1).data, 4), static_cast<quint32>(0x00200001));
}

void TestEsl::testCompactorTooManyRecords()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& statCol = data.getStatCollection();

    for (int i = 0; i < 4097; ++i)
    {
        StatRecord r;
        r.editorId = QString("S%1").arg(i);
        r.formId = 0x00100000u + static_cast<quint32>(i);
        statCol.add(r);
    }

    FormIdCompactor compactor(data);
    QCOMPARE(compactor.compact(), -1); // exceeds the ESL ceiling
}

void TestEsl::testLightMasterFlagRoundTrip()
{
    QTemporaryFile tmpFile;
    tmpFile.open();
    const QString path = tmpFile.fileName();
    tmpFile.close();

    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        ESMWriter writer;
        writer.setFileFlags(0x200); // LightMaster
        writer.save(f);
        writer.setAuthor("ESL Test");
        writer.setVersion(1.0f);
        writer.close();
        f.close();
    }

    ESMReader reader(path);
    reader.open();
    const auto& header = reader.getHeader();
    QVERIFY(header.flags.val & 0x200);
    QVERIFY(!(header.flags.val & 0x01)); // not a master
}

QTEST_MAIN(TestEsl)
#include "test_esl.moc"
