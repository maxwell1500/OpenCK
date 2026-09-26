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
    Q_UNUSED(components);

    auto* mainLayout = new QVBoxLayout(this);

    auto* group = new QGroupBox(QStringLiteral("Sound Data"), this);
    auto* form = new QFormLayout(group);

    auto* soundFileEdit = new QLineEdit(group);
    soundFileEdit->setObjectName(QStringLiteral("soundFile"));
    soundFileEdit->setPlaceholderText(QStringLiteral("Sound file path"));

    auto* flagsSpin = new QSpinBox(group);
    flagsSpin->setObjectName(QStringLiteral("flags"));
    flagsSpin->setRange(0, INT_MAX);

    form->addRow(QStringLiteral("Sound File:"), soundFileEdit);
    form->addRow(QStringLiteral("Flags:"), flagsSpin);
    mainLayout->addWidget(group);

    loadSession();
}

SoundDataWidget::~SoundDataWidget() = default;

void SoundDataWidget::loadSession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<SounRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("soundFile")))
        edit->setText(rec->soundFile);
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("flags")))
        spin->setValue(static_cast<int>(rec->flags));
}

bool SoundDataWidget::validateSession(QString* error)
{
    Q_UNUSED(error);
    // Every control here is a bounded spin box or free text, so there is
    // nothing that can be malformed; the hook exists for consistency with the
    // other record widgets.
    return true;
}

void SoundDataWidget::applySession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<SounRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("soundFile")))
        rec->soundFile = edit->text();
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("flags")))
        rec->flags = static_cast<quint32>(spin->value());
}

} // namespace openck
