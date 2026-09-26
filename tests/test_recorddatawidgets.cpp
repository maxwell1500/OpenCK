// Round-trip tests for the per-record custom data widgets.
//
// Before RecordEditSession, the majority of these widgets read the record into
// line edits and spin boxes and never wrote them back, so anything the user
// typed was silently discarded. Each test here drives one widget the way the
// dialog does — construct with the session's working record, edit a control,
// validate, apply — and then checks the value actually reached the record
// through commit() and came back through undo().

#include <QApplication>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>

#include "../../src/model/tools/undostack.hpp"
#include "../../src/view/window/recordeditsession.hpp"
#include "../../src/view/window/celldatawidget.hpp"
#include "../../src/view/window/classdatawidget.hpp"
#include "../../src/view/window/dialdatawidget.hpp"
#include "../../src/view/window/factdatawidget.hpp"
#include "../../src/view/window/hazddatawidget.hpp"
#include "../../src/view/window/locationdatawidget.hpp"
#include "../../src/view/window/npcrecorddatawidget.hpp"
#include "../../src/view/window/packdatawidget.hpp"
#include "../../src/view/window/racedatawidget.hpp"
#include "../../src/view/window/regndatawidget.hpp"
#include "../../src/view/window/sounddatawidget.hpp"
#include "../../src/view/window/wthrdatawidget.hpp"
#include "../../src/view/widgets/formdatawidget.hpp"
#include "../../src/view/widgets/recordfieldparse.hpp"

#include "../../src/model/world/collection.hpp"
#include "../../src/model/world/record.hpp"
#include "../../libs/files/esm/cellrecord.hpp"
#include "../../libs/files/esm/classrecord.hpp"
#include "../../libs/files/esm/dialrecord.hpp"
#include "../../libs/files/esm/factrecord.hpp"
#include "../../libs/files/esm/hazdrecord.hpp"
#include "../../libs/files/esm/locationrecord.hpp"
#include "../../libs/files/esm/npcrecord.hpp"
#include "../../libs/files/esm/Packagerecord.hpp"
#include "../../libs/files/esm/racerecord.hpp"
#include "../../libs/files/esm/regionrecord.hpp"
#include "../../libs/files/esm/sounrecord.hpp"
#include "../../libs/files/esm/wthrrecord.hpp"

static int g_failures = 0;

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            qWarning() << "FAIL:" << #cond << "at" << __FILE__ << ":" << __LINE__; \
            ++g_failures; \
        } \
    } while (0)

namespace {

// A record in a collection plus the session the dialog would build for it.
template <typename RecordType>
struct Fixture
{
    Collection<RecordType> collection;
    UndoStack stack;
    std::unique_ptr<openck::TypedRecordEditSession<RecordType>> session;

    explicit Fixture(const RecordType& seed, const QString& name)
    {
        collection.add(seed);
        stack.clear();
        session = std::make_unique<openck::TypedRecordEditSession<RecordType>>(
            &collection, 0, &stack, name);
    }

    RecordType* working() { return static_cast<RecordType*>(session->workingRecord()); }
    const RecordType& live() const { return collection.getRecord(0).get(); }
    openck::FormComponents* components() { return session->workingComponents(); }
};

// The dialog discovers the contract by dynamic_cast; if a widget stops deriving
// from FormDataWidget it silently stops being applied, so check it explicitly.
template <typename Widget>
bool implementsSession(Widget& widget)
{
    return dynamic_cast<openck::FormDataWidget*>(&widget) != nullptr;
}

} // namespace

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    // ---- SOUN ----------------------------------------------------------
    {
        SounRecord rec;
        rec.soundFile = QStringLiteral("Sound\\Fx\\a.wav");
        rec.flags = 3;
        Fixture<SounRecord> fx(rec, QStringLiteral("Edit SOUN"));
        openck::SoundDataWidget w(fx.working(), fx.components());
        CHECK(implementsSession(w));

        auto* file = w.findChild<QLineEdit*>(QStringLiteral("soundFile"));
        CHECK(file != nullptr);
        CHECK(file && file->text() == QStringLiteral("Sound\\Fx\\a.wav"));

        if (file) file->setText(QStringLiteral("Sound\\Fx\\b.wav"));
        w.findChild<QSpinBox*>(QStringLiteral("flags"))->setValue(9);
        w.applySession();
        CHECK(fx.working()->soundFile == QStringLiteral("Sound\\Fx\\b.wav"));
        CHECK(fx.working()->flags == 9u);
        CHECK(fx.session->commit());
        CHECK(fx.live().flags == 9u);
        fx.stack.undo();
        CHECK(fx.live().flags == 3u);
    }

    // ---- WTHR ----------------------------------------------------------
    {
        WthrRecord rec;
        rec.sunTexture = QStringLiteral("textures\\sun.dds");
        rec.flags = 1;
        Fixture<WthrRecord> fx(rec, QStringLiteral("Edit WTHR"));
        openck::WthrDataWidget w(fx.working(), fx.components());
        CHECK(implementsSession(w));

        auto* tex = w.findChild<QLineEdit*>(QStringLiteral("sunTexture"));
        CHECK(tex != nullptr);
        if (tex) tex->setText(QStringLiteral("textures\\moon.dds"));
        w.findChild<QSpinBox*>(QStringLiteral("flags"))->setValue(4);
        w.applySession();
        CHECK(fx.working()->sunTexture == QStringLiteral("textures\\moon.dds"));
        CHECK(fx.session->commit());
        CHECK(fx.live().flags == 4u);
    }

    // ---- REGN ----------------------------------------------------------
    {
        RegionRecord rec;
        rec.editorId = QStringLiteral("OldRegion");
        rec.flags = 2;
        Fixture<RegionRecord> fx(rec, QStringLiteral("Edit REGN"));
        openck::RegnDataWidget w(fx.working(), fx.components());
        CHECK(implementsSession(w));

        auto* edid = w.findChild<QLineEdit*>(QStringLiteral("editorId"));
        CHECK(edid != nullptr);
        if (edid) edid->setText(QStringLiteral("NewRegion"));
        w.findChild<QSpinBox*>(QStringLiteral("flags"))->setValue(5);
        w.applySession();
        CHECK(fx.working()->editorId == QStringLiteral("NewRegion"));
        CHECK(fx.session->commit());
        CHECK(fx.live().editorId == QStringLiteral("NewRegion"));
        fx.stack.undo();
        CHECK(fx.live().editorId == QStringLiteral("OldRegion"));
    }

    // ---- CLAS ----------------------------------------------------------
    {
        ClassRecord rec;
        rec.className = QStringLiteral("Soldier");
        rec.description = QStringLiteral("Grunt");
        rec.serviceFlags = 7;
        rec.iconPath = QStringLiteral("icon.dds");
        Fixture<ClassRecord> fx(rec, QStringLiteral("Edit CLAS"));
        openck::ClassDataWidget w(fx.working(), fx.components());
        CHECK(implementsSession(w));

        w.findChild<QLineEdit*>(QStringLiteral("className"))
            ->setText(QStringLiteral("Officer"));
        w.findChild<QSpinBox*>(QStringLiteral("serviceFlags"))->setValue(11);
        w.applySession();
        CHECK(fx.working()->className == QStringLiteral("Officer"));
        CHECK(fx.working()->serviceFlags == 11u);
        CHECK(fx.session->commit());
        CHECK(fx.live().className == QStringLiteral("Officer"));
        CHECK(fx.live().description == QStringLiteral("Grunt"));  // untouched field survives
    }

    // ---- HAZD: the spin ranges must match the quint8 fields -------------
    {
        HazdRecord rec;
        rec.modelPath = QStringLiteral("hazard.nif");
        rec.limit = 10;
        rec.target = 4;
        rec.flags = 6;
        rec.radius = 12.5f;
        rec.imageSpace = 0x1234;
        Fixture<HazdRecord> fx(rec, QStringLiteral("Edit HAZD"));
        openck::HazdDataWidget w(fx.working(), fx.components());
        CHECK(implementsSession(w));

        auto* limit = w.findChild<QSpinBox*>(QStringLiteral("limit"));
        auto* target = w.findChild<QSpinBox*>(QStringLiteral("target"));
        auto* flags = w.findChild<QSpinBox*>(QStringLiteral("flags"));
        CHECK(limit && limit->maximum() == 255);
        CHECK(target && target->maximum() == 255);
        CHECK(flags && flags->maximum() == 255);

        if (limit) limit->setValue(250);
        if (target) target->setValue(255);
        w.findChild<QDoubleSpinBox*>(QStringLiteral("radius"))->setValue(3.5);
        w.applySession();
        CHECK(fx.working()->limit == quint8(250));
        CHECK(fx.working()->target == quint8(255));
        CHECK(static_cast<float>(fx.working()->radius) == 3.5f);
        CHECK(fx.session->commit());
        CHECK(fx.live().limit == quint8(250));
        CHECK(fx.live().imageSpace == 0x1234u);
    }

    // ---- LCTN ----------------------------------------------------------
    {
        LocationRecord rec;
        rec.editorId = QStringLiteral("LocOld");
        rec.locationName = QStringLiteral("Somewhere");
        rec.parentId = 5;
        rec.x = 1;
        rec.y = 2;
        rec.z = 3;
        Fixture<LocationRecord> fx(rec, QStringLiteral("Edit LCTN"));
        LocationDataWidget w(fx.working(), fx.components());
        CHECK(implementsSession(w));

        w.findChild<QLineEdit*>(QStringLiteral("editorId"))
            ->setText(QStringLiteral("LocNew"));
        w.findChild<QSpinBox*>(QStringLiteral("z"))->setValue(42);
        w.applySession();
        CHECK(fx.working()->editorId == QStringLiteral("LocNew"));
        CHECK(fx.working()->z == 42u);
        CHECK(fx.working()->locationName == QStringLiteral("Somewhere"));
        CHECK(fx.session->commit());
        CHECK(fx.live().z == 42u);
    }

    // ---- RACE: hex list, with validation -------------------------------
    {
        RaceRecord rec;
        rec.raceFlags = 0x10;
        rec.npcVariables = { 0x11u, 0x22u };
        Fixture<RaceRecord> fx(rec, QStringLiteral("Edit RACE"));
        openck::RaceDataWidget w(fx.working(), fx.components());
        CHECK(implementsSession(w));

        auto* list = w.findChild<QListWidget*>(QStringLiteral("npcVariables"));
        CHECK(list != nullptr);
        CHECK(list && list->count() == 2);
        CHECK(list && list->item(0)->text() == QStringLiteral("0x00000011"));

        w.findChild<QSpinBox*>(QStringLiteral("raceFlags"))->setValue(32);
        if (list) {
            list->item(0)->setText(QStringLiteral("0x000000ab"));
            list->addItem(QStringLiteral("zzz"));
        }
        QString error;
        CHECK(!w.validateSession(&error));
        CHECK(!error.isEmpty());

        if (list) list->item(2)->setText(QStringLiteral("0x00000033"));
        CHECK(w.validateSession(&error));
        w.applySession();
        CHECK(fx.working()->raceFlags == 32u);
        CHECK(fx.working()->npcVariables == QVector<quint32>({ 0xabu, 0x22u, 0x33u }));
        CHECK(fx.session->commit());
        CHECK(fx.live().npcVariables.size() == 3);
        fx.stack.undo();
        CHECK(fx.live().npcVariables.size() == 2);
    }

    // ---- DIAL ----------------------------------------------------------
    {
        DialRecord rec;
        rec.topicName = QStringLiteral("Greetings");
        rec.responseIds = { 1u, 2u };
        Fixture<DialRecord> fx(rec, QStringLiteral("Edit DIAL"));
        openck::DialDataWidget w(fx.working(), fx.components());
        CHECK(implementsSession(w));

        auto* topic = w.findChild<QLineEdit*>(QStringLiteral("topicName"));
        CHECK(topic != nullptr);
        auto* list = w.findChild<QListWidget*>(QStringLiteral("responseIds"));
        CHECK(list && list->count() == 2);
        if (topic) topic->setText(QStringLiteral("Farewells"));
        if (list) list->item(1)->setText(QStringLiteral("0x00000099"));
        w.applySession();
        CHECK(fx.working()->topicName == QStringLiteral("Farewells"));
        CHECK(fx.working()->responseIds == QVector<quint32>({ 1u, 0x99u }));
        CHECK(fx.session->commit());
        CHECK(fx.live().topicName == QStringLiteral("Farewells"));
    }

    // ---- PACK ----------------------------------------------------------
    {
        PackageRecord rec;
        rec.editorId = QStringLiteral("PackOld");
        rec.packageType = 1;
        rec.targetType = 2;
        rec.flags = 3;
        rec.targetIds = { 7u };
        Fixture<PackageRecord> fx(rec, QStringLiteral("Edit PACK"));
        PackDataWidget w(fx.working(), fx.components());
        CHECK(implementsSession(w));

        w.findChild<QLineEdit*>(QStringLiteral("editorId"))
            ->setText(QStringLiteral("PackNew"));
        w.findChild<QSpinBox*>(QStringLiteral("packageType"))->setValue(9);
        w.applySession();
        CHECK(fx.working()->editorId == QStringLiteral("PackNew"));
        CHECK(fx.working()->packageType == 9u);
        CHECK(fx.working()->targetType == 2u);  // untouched
        CHECK(fx.session->commit());
        CHECK(fx.live().editorId == QStringLiteral("PackNew"));
    }

    // ---- FACT: string ranks plus a hex relations list ------------------
    {
        FactRecord rec;
        rec.factionName = QStringLiteral("Empire");
        rec.ranks = { QStringLiteral("Recruit"), QStringLiteral("Knight") };
        rec.relations = { 5u };
        Fixture<FactRecord> fx(rec, QStringLiteral("Edit FACT"));
        openck::FactDataWidget w(fx.working(), fx.components());
        CHECK(implementsSession(w));

        auto* ranks = w.findChild<QListWidget*>(QStringLiteral("ranks"));
        auto* relations = w.findChild<QListWidget*>(QStringLiteral("relations"));
        CHECK(ranks && ranks->count() == 2);
        CHECK(relations && relations->count() == 1);
        if (ranks) ranks->item(0)->setText(QStringLiteral("Squire"));
        if (relations) relations->item(0)->setText(QStringLiteral("0x00000042"));
        w.findChild<QSpinBox*>(QStringLiteral("flags"))->setValue(6);
        w.applySession();
        CHECK(fx.working()->ranks == QVector<QString>(
            { QStringLiteral("Squire"), QStringLiteral("Knight") }));
        CHECK(fx.working()->relations == QVector<quint32>({ 0x42u }));
        CHECK(fx.working()->flags == 6u);
        CHECK(fx.session->commit());
        CHECK(fx.live().ranks.size() == 2);
        CHECK(fx.live().relations == QVector<quint32>({ 0x42u }));
    }

    // ---- NPC_: hex FormIDs are validated before they are written --------
    {
        NpcRecord rec;
        rec.race = 0x100;
        rec.class_ = 0x200;
        rec.health = 50;
        rec.magicka = 60;
        rec.stamina = 70;
        Fixture<NpcRecord> fx(rec, QStringLiteral("Edit NPC_"));
        openck::NpcRecordDataWidget w(fx.working(), fx.components());
        CHECK(implementsSession(w));

        auto* race = w.findChild<QLineEdit*>(QStringLiteral("race"));
        auto* klass = w.findChild<QLineEdit*>(QStringLiteral("class"));
        CHECK(race && race->text() == QStringLiteral("0x00000100"));

        if (race) race->setText(QStringLiteral("not-hex"));
        QString error;
        CHECK(!w.validateSession(&error));
        CHECK(!error.isEmpty());
        // A refused edit must not write anything.
        w.applySession();
        CHECK(fx.working()->race == 0x100u);

        if (race) race->setText(QStringLiteral("0x00000abc"));
        if (klass) klass->setText(QStringLiteral("7f"));
        w.findChild<QSpinBox*>(QStringLiteral("health"))->setValue(80);
        CHECK(w.validateSession(&error));
        w.applySession();
        CHECK(fx.working()->race == 0xabcu);   // "0x..." form
        CHECK(fx.working()->class_ == 0x7fu);  // bare hex form
        CHECK(fx.working()->health == 80u);
        CHECK(fx.session->commit());
        CHECK(fx.live().race == 0xabcu);
        fx.stack.undo();
        CHECK(fx.live().race == 0x100u);
    }

    // ---- CELL: coordinates must not go negative into a quint32 ----------
    {
        CellRecord rec;
        rec.cellX = 100;
        rec.cellY = 200;
        rec.owner = 0x30;
        rec.lockLevel = 30;
        Fixture<CellRecord> fx(rec, QStringLiteral("Edit CELL"));
        openck::CellDataWidget w(fx.working(), fx.components());
        CHECK(implementsSession(w));

        auto* x = w.findChild<QSpinBox*>(QStringLiteral("cellX"));
        CHECK(x && x->minimum() == 0);
        if (x) x->setValue(-5);   // clamped by the range
        CHECK(x && x->value() == 0);

        auto* owner = w.findChild<QLineEdit*>(QStringLiteral("owner"));
        CHECK(owner != nullptr);
        if (owner) owner->setText(QStringLiteral("oops"));
        QString error;
        CHECK(!w.validateSession(&error));
        w.applySession();
        CHECK(fx.working()->owner == 0x30u);

        if (owner) owner->setText(QStringLiteral("0x000000ff"));
        w.findChild<QSpinBox*>(QStringLiteral("cellY"))->setValue(250);
        CHECK(w.validateSession(&error));
        w.applySession();
        CHECK(fx.working()->cellX == 0u);
        CHECK(fx.working()->cellY == 250u);
        CHECK(fx.working()->owner == 0xffu);
        CHECK(fx.session->commit());
        CHECK(fx.live().owner == 0xffu);
    }

    // ---- parseFormId / formatFormId round trip --------------------------
    {
        quint32 value = 0;
        CHECK(openck::parseFormId(QStringLiteral("0x1A2B"), value));
        CHECK(value == 0x1A2Bu);
        CHECK(openck::parseFormId(QStringLiteral("1a2b"), value));
        CHECK(value == 0x1A2Bu);
        CHECK(openck::parseFormId(QStringLiteral("  0x1a2b  "), value));
        CHECK(value == 0x1A2Bu);
        CHECK(!openck::parseFormId(QString(), value));
        CHECK(!openck::parseFormId(QStringLiteral("0x"), value));
        CHECK(!openck::parseFormId(QStringLiteral("zz"), value));
        CHECK(openck::formatFormId(0x1A2Bu) == QStringLiteral("0x00001a2b"));
    }

    if (g_failures == 0)
    {
        qDebug() << "test_recorddatawidgets: all checks passed";
        return 0;
    }
    qWarning() << "test_recorddatawidgets:" << g_failures << "check(s) failed";
    return 1;
}
