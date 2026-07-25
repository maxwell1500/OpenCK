#include "wthrdatawidget.hpp"
#include "../libs/files/esm/wthrrecord.hpp"
#include "../libs/components/formcomponents.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>

namespace openck {

WthrDataWidget::WthrDataWidget(void* recordPtr,
                               FormComponents* components,
                               QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* group = new QGroupBox(QStringLiteral("Weather Data"), this);
    auto* form = new QFormLayout(group);

    auto* sunTextureEdit = new QLineEdit(group);
    sunTextureEdit->setPlaceholderText(QStringLiteral("Sun texture path"));

    auto* flagsSpin = new QSpinBox(group);
    flagsSpin->setRange(0, INT_MAX);

    form->addRow(QStringLiteral("Sun Texture:"), sunTextureEdit);
    form->addRow(QStringLiteral("Flags:"), flagsSpin);
    mainLayout->addWidget(group);

    if (m_recordPtr)
    {
        auto* rec = static_cast<WthrRecord*>(m_recordPtr);
        sunTextureEdit->setText(rec->sunTexture);
        flagsSpin->setValue(static_cast<int>(rec->flags));
    }
}

WthrDataWidget::~WthrDataWidget() = default;

} // namespace openck
