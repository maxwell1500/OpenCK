#include "npcrecorddatawidget.hpp"
#include "../libs/files/esm/npcrecord.hpp"
#include "../libs/components/formcomponents.hpp"
#include "../libs/components/tier1_components.hpp"
#include "../libs/components/tier3_components.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

namespace openck {

NpcRecordDataWidget::NpcRecordDataWidget(void* recordPtr,
                                         FormComponents* components,
                                         QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    auto* mainLayout = new QVBoxLayout(this);

    // Identity section
    auto* idGroup = new QGroupBox(QStringLiteral("Identity"), this);
    auto* idForm = new QFormLayout(idGroup);
    auto* raceEdit = new QLineEdit(idGroup);
    raceEdit->setPlaceholderText(QStringLiteral("Race Form ID (hex)"));
    auto* classEdit = new QLineEdit(idGroup);
    classEdit->setPlaceholderText(QStringLiteral("Class Form ID (hex)"));
    idForm->addRow(QStringLiteral("Race:"), raceEdit);
    idForm->addRow(QStringLiteral("Class:"), classEdit);
    mainLayout->addWidget(idGroup);

    // Stats section
    auto* statGroup = new QGroupBox(QStringLiteral("Attributes"), this);
    auto* statForm = new QFormLayout(statGroup);
    auto* healthSpin = new QSpinBox(statGroup);
    healthSpin->setRange(0, 99999);
    auto* magickaSpin = new QSpinBox(statGroup);
    magickaSpin->setRange(0, 99999);
    auto* staminaSpin = new QSpinBox(statGroup);
    staminaSpin->setRange(0, 99999);
    statForm->addRow(QStringLiteral("Health:"), healthSpin);
    statForm->addRow(QStringLiteral("Magicka:"), magickaSpin);
    statForm->addRow(QStringLiteral("Stamina:"), staminaSpin);
    mainLayout->addWidget(statGroup);

    // If we have a valid record pointer, load current values
    if (m_recordPtr)
    {
        auto* rec = static_cast<NpcRecord*>(m_recordPtr);
        raceEdit->setText(QStringLiteral("0x%1").arg(rec->race, 8, 16, QChar('0')));
        classEdit->setText(QStringLiteral("0x%1").arg(rec->class_, 8, 16, QChar('0')));
        healthSpin->setValue(rec->health);
        magickaSpin->setValue(rec->magicka);
        staminaSpin->setValue(rec->stamina);
    }
}

NpcRecordDataWidget::~NpcRecordDataWidget() = default;

} // namespace openck
