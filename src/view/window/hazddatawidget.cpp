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

namespace {
// HazdRecord stores limit, target and flags as quint8, so the spin boxes are
// capped at 255. They used to allow INT_MAX, which let the user type a value
// that was silently truncated on the way into the record.
constexpr int kByteMax = 255;
}

HazdDataWidget::HazdDataWidget(void* recordPtr,
                               FormComponents* components,
                               QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    Q_UNUSED(components);

    auto* mainLayout = new QVBoxLayout(this);

    auto* group = new QGroupBox(QStringLiteral("Hazard Data"), this);
    auto* form = new QFormLayout(group);

    auto* modelPathEdit = new QLineEdit(group);
    modelPathEdit->setObjectName(QStringLiteral("modelPath"));
    modelPathEdit->setPlaceholderText(QStringLiteral("Model path"));

    auto* limitSpin = new QSpinBox(group);
    limitSpin->setObjectName(QStringLiteral("limit"));
    limitSpin->setRange(0, kByteMax);

    auto* radiusSpin = new QDoubleSpinBox(group);
    radiusSpin->setObjectName(QStringLiteral("radius"));
    radiusSpin->setRange(0.0, 1e6);
    radiusSpin->setDecimals(2);

    auto* lifetimeSpin = new QDoubleSpinBox(group);
    lifetimeSpin->setObjectName(QStringLiteral("lifetime"));
    lifetimeSpin->setRange(0.0, 1e6);
    lifetimeSpin->setDecimals(2);

    auto* imageSpaceSpin = new QSpinBox(group);
    imageSpaceSpin->setObjectName(QStringLiteral("imageSpace"));
    imageSpaceSpin->setRange(0, INT_MAX);

    auto* targetSpin = new QSpinBox(group);
    targetSpin->setObjectName(QStringLiteral("target"));
    targetSpin->setRange(0, kByteMax);

    auto* flagsSpin = new QSpinBox(group);
    flagsSpin->setObjectName(QStringLiteral("flags"));
    flagsSpin->setRange(0, kByteMax);

    form->addRow(QStringLiteral("Model Path:"), modelPathEdit);
    form->addRow(QStringLiteral("Limit:"), limitSpin);
    form->addRow(QStringLiteral("Radius:"), radiusSpin);
    form->addRow(QStringLiteral("Lifetime:"), lifetimeSpin);
    form->addRow(QStringLiteral("Image Space:"), imageSpaceSpin);
    form->addRow(QStringLiteral("Target:"), targetSpin);
    form->addRow(QStringLiteral("Flags:"), flagsSpin);
    mainLayout->addWidget(group);

    loadSession();
}

HazdDataWidget::~HazdDataWidget() = default;

void HazdDataWidget::loadSession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<HazdRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("modelPath")))
        edit->setText(rec->modelPath);
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("limit")))
        spin->setValue(static_cast<int>(rec->limit));
    if (auto* spin = findChild<QDoubleSpinBox*>(QStringLiteral("radius")))
        spin->setValue(static_cast<double>(rec->radius));
    if (auto* spin = findChild<QDoubleSpinBox*>(QStringLiteral("lifetime")))
        spin->setValue(static_cast<double>(rec->lifetime));
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("imageSpace")))
        spin->setValue(static_cast<int>(rec->imageSpace));
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("target")))
        spin->setValue(static_cast<int>(rec->target));
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("flags")))
        spin->setValue(static_cast<int>(rec->flags));
}

bool HazdDataWidget::validateSession(QString* error)
{
    Q_UNUSED(error);
    return true;
}

void HazdDataWidget::applySession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<HazdRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("modelPath")))
        rec->modelPath = edit->text();
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("limit")))
        rec->limit = static_cast<quint8>(spin->value());
    if (auto* spin = findChild<QDoubleSpinBox*>(QStringLiteral("radius")))
        rec->radius = static_cast<float>(spin->value());
    if (auto* spin = findChild<QDoubleSpinBox*>(QStringLiteral("lifetime")))
        rec->lifetime = static_cast<float>(spin->value());
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("imageSpace")))
        rec->imageSpace = static_cast<quint32>(spin->value());
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("target")))
        rec->target = static_cast<quint8>(spin->value());
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("flags")))
        rec->flags = static_cast<quint8>(spin->value());
}

} // namespace openck
