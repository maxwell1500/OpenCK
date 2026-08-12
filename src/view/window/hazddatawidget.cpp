#include "hazddatawidget.hpp"
#include "../libs/files/esm/hazdrecord.hpp"
#include "../libs/components/formcomponents.hpp"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace openck {

HazdDataWidget::HazdDataWidget(void* recordPtr,
                               FormComponents* components,
                               QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* group = new QGroupBox(QStringLiteral("Hazard Data"), this);
    auto* form = new QFormLayout(group);

    auto* modelPathEdit = new QLineEdit(group);
    modelPathEdit->setPlaceholderText(QStringLiteral("Model path"));

    auto* limitSpin = new QSpinBox(group);
    limitSpin->setRange(0, INT_MAX);

    auto* radiusSpin = new QDoubleSpinBox(group);
    radiusSpin->setRange(0.0, 1e6);
    radiusSpin->setDecimals(2);

    auto* lifetimeSpin = new QDoubleSpinBox(group);
    lifetimeSpin->setRange(0.0, 1e6);
    lifetimeSpin->setDecimals(2);

    auto* imageSpaceSpin = new QSpinBox(group);
    imageSpaceSpin->setRange(0, INT_MAX);

    auto* targetSpin = new QSpinBox(group);
    targetSpin->setRange(0, INT_MAX);

    auto* flagsSpin = new QSpinBox(group);
    flagsSpin->setRange(0, INT_MAX);

    form->addRow(QStringLiteral("Model Path:"), modelPathEdit);
    form->addRow(QStringLiteral("Limit:"), limitSpin);
    form->addRow(QStringLiteral("Radius:"), radiusSpin);
    form->addRow(QStringLiteral("Lifetime:"), lifetimeSpin);
    form->addRow(QStringLiteral("Image Space:"), imageSpaceSpin);
    form->addRow(QStringLiteral("Target:"), targetSpin);
    form->addRow(QStringLiteral("Flags:"), flagsSpin);
    mainLayout->addWidget(group);

    if (m_recordPtr)
    {
        auto* rec = static_cast<HazdRecord*>(m_recordPtr);
        modelPathEdit->setText(rec->modelPath);
        limitSpin->setValue(static_cast<int>(rec->limit));
        radiusSpin->setValue(static_cast<double>(rec->radius));
        lifetimeSpin->setValue(static_cast<double>(rec->lifetime));
        imageSpaceSpin->setValue(static_cast<int>(rec->imageSpace));
        targetSpin->setValue(static_cast<int>(rec->target));
        flagsSpin->setValue(static_cast<int>(rec->flags));
    }
}

HazdDataWidget::~HazdDataWidget() = default;

} // namespace openck
