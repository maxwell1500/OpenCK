#include "wthrdatawidget.hpp"
#include "../libs/files/esm/wthrrecord.hpp"
#include "../libs/components/formcomponents.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace openck {

WthrDataWidget::WthrDataWidget(void* recordPtr,
                               FormComponents* components,
                               QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    Q_UNUSED(components);

    auto* mainLayout = new QVBoxLayout(this);

    auto* group = new QGroupBox(QStringLiteral("Weather Data"), this);
    auto* form = new QFormLayout(group);

    auto* sunTextureEdit = new QLineEdit(group);
    sunTextureEdit->setObjectName(QStringLiteral("sunTexture"));
    sunTextureEdit->setPlaceholderText(QStringLiteral("Sun texture path"));

    auto* flagsSpin = new QSpinBox(group);
    flagsSpin->setObjectName(QStringLiteral("flags"));
    flagsSpin->setRange(0, INT_MAX);

    form->addRow(QStringLiteral("Sun Texture:"), sunTextureEdit);
    form->addRow(QStringLiteral("Flags:"), flagsSpin);
    mainLayout->addWidget(group);

    loadSession();
}

WthrDataWidget::~WthrDataWidget() = default;

void WthrDataWidget::loadSession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<WthrRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("sunTexture")))
        edit->setText(rec->sunTexture);
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("flags")))
        spin->setValue(static_cast<int>(rec->flags));
}

bool WthrDataWidget::validateSession(QString* error)
{
    Q_UNUSED(error);
    return true;
}

void WthrDataWidget::applySession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<WthrRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("sunTexture")))
        rec->sunTexture = edit->text();
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("flags")))
        rec->flags = static_cast<quint32>(spin->value());
}

} // namespace openck
