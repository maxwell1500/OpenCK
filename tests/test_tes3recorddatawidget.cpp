#include <QtTest>
#include <QApplication>
#include <QGroupBox>
#include <QLineEdit>

#include "../../src/view/window/tes3recorddatawidget.hpp"
#include "../../src/view/window/rawsubrecordwidget.hpp"
#include "../../src/view/window/qtformdialogmanager.hpp"
#include "../../src/model/world/data.hpp"
#include "../../libs/files/esm/Tes3record.hpp"
#include "../../libs/files/esm/common.hpp"
#include "../../libs/files/esm/records.hpp"
#include "../../libs/components/formcomponents.hpp"

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            qWarning() << "FAIL:" << #cond << "at" << __FILE__ << ":" << __LINE__; \
            ++failures; \
        } \
    } while (0)

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    int failures = 0;

    // tes3MappedCodes round-trips through tes3TypeForName: every listed
    // code maps to a real type, and unknown codes fall back to Type_None.
    {
        const QVector<NAME> codes = Data::tes3MappedCodes();
        CHECK(!codes.isEmpty());
        for (NAME code : codes)
            CHECK(Data::tes3TypeForName(code) != CkId::Type_None);
        CHECK(Data::tes3TypeForName(NAME('ZZZZ')) == CkId::Type_None);
        CHECK(codes.contains(NAME('GMST')));
        CHECK(codes.contains(NAME('SKIL')));
        CHECK(codes.contains(NAME('LEVC')));
        CHECK(codes.size() == 41);
    }

    // Null record: widget still builds its identity + raw groups.
    {
        openck::Tes3RecordDataWidget w(nullptr, nullptr);
        CHECK(w.layout() != nullptr);
        auto groups = w.findChildren<QGroupBox*>();
        CHECK(groups.size() == 2);
        CHECK(w.findChild<QLineEdit*>(QStringLiteral("editorId")) != nullptr);
        CHECK(w.findChild<RawSubrecordWidget*>(
                  QStringLiteral("rawSubrecords")) != nullptr);
    }

    // Synthetic record: identity fields and raw subrecords populate.
    {
        Tes3Record rec;
        rec.code = NAME('GMST');
        rec.editorId = QStringLiteral("fMarketMultiplier");
        rec.formId = 0xF0000011;
        rec.flags = 0x00000008;
        RawSubRecord sub;
        sub.name = NAME('DATA');
        sub.data = QByteArray::fromHex("0000803f");
        rec.rawSubRecords.append(sub);

        openck::Tes3RecordDataWidget w(&rec, &rec.components);
        CHECK(w.layout() != nullptr);

        auto* codeEdit = w.findChild<QLineEdit*>(QStringLiteral("recordCode"));
        CHECK(codeEdit && codeEdit->text() == QStringLiteral("GMST"));
        CHECK(codeEdit && codeEdit->isReadOnly());

        auto* editorIdEdit =
            w.findChild<QLineEdit*>(QStringLiteral("editorId"));
        CHECK(editorIdEdit
              && editorIdEdit->text() == QStringLiteral("fMarketMultiplier"));

        auto* formIdEdit = w.findChild<QLineEdit*>(QStringLiteral("formId"));
        CHECK(formIdEdit && formIdEdit->text() == QStringLiteral("0xf0000011"));

        auto* flagsEdit = w.findChild<QLineEdit*>(QStringLiteral("flags"));
        CHECK(flagsEdit && flagsEdit->text() == QStringLiteral("0x00000008"));

        auto* raw = w.findChild<RawSubrecordWidget*>(
            QStringLiteral("rawSubrecords"));
        CHECK(raw && raw->count() == 1);
    }

    // T3: factory key: registerFactory under a T3:-prefixed code key
    // resolves independently of the bare TES4 code, and openOrFocus
    // builds the Tes3RecordDataWidget through the factory.
    {
        auto& mgr = openck::QtFormDialogManager::instance();
        mgr.closeAll();

        const QString key = QStringLiteral("T3:")
            + nameToQString(NAME('GMST'));
        CHECK(!mgr.hasFactory(key));
        mgr.registerFactory(key,
            [](openck::FormComponents* comps, void* recPtr,
               QWidget* parent) -> QWidget* {
                return new openck::Tes3RecordDataWidget(recPtr, comps, parent);
            });
        CHECK(mgr.hasFactory(key));

        Tes3Record rec;
        rec.code = NAME('GMST');
        rec.editorId = QStringLiteral("testSetting");
        mgr.openOrFocus(QStringLiteral("0xf0000001"), key,
                        &rec.components, &rec);
        CHECK(mgr.openCount() == 1);

        mgr.closeAll();
        CHECK(mgr.openCount() == 0);
    }

    if (failures == 0)
    {
        qDebug() << "test_tes3recorddatawidget: all checks passed";
        return 0;
    }
    qWarning() << "test_tes3recorddatawidget:" << failures
               << "check(s) failed";
    return 1;
}
