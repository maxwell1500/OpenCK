#include "sounddatawidget.hpp"
#include "../libs/files/esm/sounrecord.hpp"
#include "../libs/components/formcomponents.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>

namespace openck {

SoundDataWidget::SoundDataWidget(void* recordPtr,
                                 FormComponents* components,
                                 QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* group = new QGroupBox(QStringLiteral("Sound Data"), this);
    auto* form = new QFormLayout(group);

    auto* soundFileEdit = new QLineEdit(group);
    soundFileEdit->setPlaceholderText(QStringLiteral("Sound file path"));

    auto* flagsSpin = new QSpinBox(group);
    flagsSpin->setRange(0, INT_MAX);

    form->addRow(QStringLiteral("Sound File:"), soundFileEdit);
    form->addRow(QStringLiteral("Flags:"), flagsSpin);
    mainLayout->addWidget(group);

    if (m_recordPtr)
    {
        auto* rec = static_cast<SounRecord*>(m_recordPtr);
        soundFileEdit->setText(rec->soundFile);
        flagsSpin->setValue(static_cast<int>(rec->flags));
    }
}

SoundDataWidget::~SoundDataWidget() = default;

} // namespace openck
