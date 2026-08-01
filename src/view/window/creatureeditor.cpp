#include "creatureeditor.hpp"
#include "../libs/files/esm/creaturerecord.hpp"
#include "../libs/components/formcomponents.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>

namespace openck {

CreatureDataWidget::CreatureDataWidget(void* recordPtr,
                                       FormComponents* components,
                                       QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox(QStringLiteral("Creature Data"), this);
    auto* infoForm = new QFormLayout(infoGroup);

    auto* editorIdEdit = new QLineEdit(infoGroup);
    editorIdEdit->setObjectName(QStringLiteral("editorId"));
    editorIdEdit->setPlaceholderText(QStringLiteral("Editor ID"));

    auto* fullNameEdit = new QLineEdit(infoGroup);
    fullNameEdit->setObjectName(QStringLiteral("fullName"));
    fullNameEdit->setPlaceholderText(QStringLiteral("Full Name"));

    auto* typeSpin = new QSpinBox(infoGroup);
    typeSpin->setRange(0, INT_MAX);

    auto* healthSpin = new QSpinBox(infoGroup);
    healthSpin->setRange(0, 65535);
    auto* magickaSpin = new QSpinBox(infoGroup);
    magickaSpin->setRange(0, 65535);
    auto* fatigueSpin = new QSpinBox(infoGroup);
    fatigueSpin->setRange(0, 65535);
    auto* damageSpin = new QSpinBox(infoGroup);
    damageSpin->setRange(0, 65535);

    infoForm->addRow(QStringLiteral("Editor ID:"), editorIdEdit);
    infoForm->addRow(QStringLiteral("Full Name:"), fullNameEdit);
    infoForm->addRow(QStringLiteral("Type:"), typeSpin);
    infoForm->addRow(QStringLiteral("Health:"), healthSpin);
    infoForm->addRow(QStringLiteral("Magicka:"), magickaSpin);
    infoForm->addRow(QStringLiteral("Fatigue:"), fatigueSpin);
    infoForm->addRow(QStringLiteral("Damage:"), damageSpin);
    mainLayout->addWidget(infoGroup);

    auto* attrGroup = new QGroupBox(QStringLiteral("Attributes"), this);
    auto* attrForm = new QFormLayout(attrGroup);

    auto* strengthSpin = new QSpinBox(attrGroup);
    strengthSpin->setRange(0, 65535);
    auto* intelligenceSpin = new QSpinBox(attrGroup);
    intelligenceSpin->setRange(0, 65535);
    auto* willpowerSpin = new QSpinBox(attrGroup);
    willpowerSpin->setRange(0, 65535);
    auto* agilitySpin = new QSpinBox(attrGroup);
    agilitySpin->setRange(0, 65535);
    auto* speedSpin = new QSpinBox(attrGroup);
    speedSpin->setRange(0, 65535);
    auto* enduranceSpin = new QSpinBox(attrGroup);
    enduranceSpin->setRange(0, 65535);
    auto* personalitySpin = new QSpinBox(attrGroup);
    personalitySpin->setRange(0, 65535);
    auto* luckSpin = new QSpinBox(attrGroup);
    luckSpin->setRange(0, 65535);

    attrForm->addRow(QStringLiteral("Strength:"), strengthSpin);
    attrForm->addRow(QStringLiteral("Intelligence:"), intelligenceSpin);
    attrForm->addRow(QStringLiteral("Willpower:"), willpowerSpin);
    attrForm->addRow(QStringLiteral("Agility:"), agilitySpin);
    attrForm->addRow(QStringLiteral("Speed:"), speedSpin);
    attrForm->addRow(QStringLiteral("Endurance:"), enduranceSpin);
    attrForm->addRow(QStringLiteral("Personality:"), personalitySpin);
    attrForm->addRow(QStringLiteral("Luck:"), luckSpin);
    mainLayout->addWidget(attrGroup);

    if (m_recordPtr)
    {
        auto* rec = static_cast<CreatureRecord*>(m_recordPtr);
        editorIdEdit->setText(rec->editorId);
        fullNameEdit->setText(rec->fullName);
        typeSpin->setValue(static_cast<int>(rec->creaData.type));
        healthSpin->setValue(rec->creaData.health);
        magickaSpin->setValue(rec->creaData.magicka);
        fatigueSpin->setValue(rec->creaData.fatigue);
        damageSpin->setValue(rec->creaData.damage);
        strengthSpin->setValue(rec->creaData.strength);
        intelligenceSpin->setValue(rec->creaData.intelligence);
        willpowerSpin->setValue(rec->creaData.willpower);
        agilitySpin->setValue(rec->creaData.agility);
        speedSpin->setValue(rec->creaData.speed);
        enduranceSpin->setValue(rec->creaData.endurance);
        personalitySpin->setValue(rec->creaData.personality);
        luckSpin->setValue(rec->creaData.luck);
    }
}

CreatureDataWidget::~CreatureDataWidget() = default;

} // namespace openck
