#include "npcrecorddatawidget.hpp"
#include "../libs/files/esm/npcrecord.hpp"
#include "../libs/components/formcomponents.hpp"
#include "../libs/components/tier1_components.hpp"
#include "../libs/components/tier3_components.hpp"
#include "../widgets/recordfieldparse.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace openck {

NpcRecordDataWidget::NpcRecordDataWidget(void* recordPtr,
                                         FormComponents* components,
                                         QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    Q_UNUSED(components);

    auto* mainLayout = new QVBoxLayout(this);

    // Identity section
    auto* idGroup = new QGroupBox(QStringLiteral("Identity"), this);
    auto* idForm = new QFormLayout(idGroup);
    auto* raceEdit = new QLineEdit(idGroup);
    raceEdit->setObjectName(QStringLiteral("race"));
    raceEdit->setPlaceholderText(QStringLiteral("Race Form ID (hex)"));
    auto* classEdit = new QLineEdit(idGroup);
    classEdit->setObjectName(QStringLiteral("class"));
    classEdit->setPlaceholderText(QStringLiteral("Class Form ID (hex)"));
    idForm->addRow(QStringLiteral("Race:"), raceEdit);
    idForm->addRow(QStringLiteral("Class:"), classEdit);
    mainLayout->addWidget(idGroup);

    // Stats section
    auto* statGroup = new QGroupBox(QStringLiteral("Attributes"), this);
    auto* statForm = new QFormLayout(statGroup);
    auto* healthSpin = new QSpinBox(statGroup);
    healthSpin->setObjectName(QStringLiteral("health"));
    healthSpin->setRange(0, 99999);
    auto* magickaSpin = new QSpinBox(statGroup);
    magickaSpin->setObjectName(QStringLiteral("magicka"));
    magickaSpin->setRange(0, 99999);
    auto* staminaSpin = new QSpinBox(statGroup);
    staminaSpin->setObjectName(QStringLiteral("stamina"));
    staminaSpin->setRange(0, 99999);
    statForm->addRow(QStringLiteral("Health:"), healthSpin);
    statForm->addRow(QStringLiteral("Magicka:"), magickaSpin);
    statForm->addRow(QStringLiteral("Stamina:"), staminaSpin);
    mainLayout->addWidget(statGroup);

    loadSession();
}

NpcRecordDataWidget::~NpcRecordDataWidget() = default;

void NpcRecordDataWidget::loadSession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<NpcRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("race")))
        edit->setText(formatFormId(rec->race));
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("class")))
        edit->setText(formatFormId(rec->class_));
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("health")))
        spin->setValue(static_cast<int>(rec->health));
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("magicka")))
        spin->setValue(static_cast<int>(rec->magicka));
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("stamina")))
        spin->setValue(static_cast<int>(rec->stamina));
}

bool NpcRecordDataWidget::validateSession(QString* error)
{
    struct Field { const char* objectName; const char* label; };
    static const Field fields[] = {
        { "race", "Race" },
        { "class", "Class" },
    };
    for (const Field& field : fields)
    {
        auto* edit = findChild<QLineEdit*>(QString::fromLatin1(field.objectName));
        if (!edit) continue;
        quint32 ignored = 0;
        if (!parseFormId(edit->text(), ignored))
        {
            if (error)
                *error = QStringLiteral("%1 is not a valid FormID: \"%2\".")
                             .arg(QString::fromLatin1(field.label), edit->text());
            return false;
        }
    }
    return true;
}

void NpcRecordDataWidget::applySession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<NpcRecord*>(m_recordPtr);
    quint32 id = 0;
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("race")))
        if (parseFormId(edit->text(), id))
            rec->race = id;
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("class")))
        if (parseFormId(edit->text(), id))
            rec->class_ = id;
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("health")))
        rec->health = static_cast<quint32>(spin->value());
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("magicka")))
        rec->magicka = static_cast<quint32>(spin->value());
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("stamina")))
        rec->stamina = static_cast<quint32>(spin->value());
}

} // namespace openck
