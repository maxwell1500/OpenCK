#include "regndatawidget.hpp"
#include "../libs/files/esm/regionrecord.hpp"
#include "../libs/components/formcomponents.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace openck {

RegnDataWidget::RegnDataWidget(void* recordPtr,
                               FormComponents* components,
                               QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    Q_UNUSED(components);

    auto* mainLayout = new QVBoxLayout(this);

    auto* group = new QGroupBox(QStringLiteral("Region Data"), this);
    auto* form = new QFormLayout(group);

    auto* editorIdEdit = new QLineEdit(group);
    editorIdEdit->setObjectName(QStringLiteral("editorId"));
    editorIdEdit->setPlaceholderText(QStringLiteral("Editor ID"));

    auto* flagsSpin = new QSpinBox(group);
    flagsSpin->setObjectName(QStringLiteral("flags"));
    flagsSpin->setRange(0, INT_MAX);

    form->addRow(QStringLiteral("Editor ID:"), editorIdEdit);
    form->addRow(QStringLiteral("Flags:"), flagsSpin);
    mainLayout->addWidget(group);

    loadSession();
}

RegnDataWidget::~RegnDataWidget() = default;

void RegnDataWidget::loadSession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<RegionRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("editorId")))
        edit->setText(rec->editorId);
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("flags")))
        spin->setValue(static_cast<int>(rec->flags));
}

bool RegnDataWidget::validateSession(QString* error)
{
    Q_UNUSED(error);
    return true;
}

void RegnDataWidget::applySession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<RegionRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("editorId")))
        rec->editorId = edit->text();
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("flags")))
        rec->flags = static_cast<quint32>(spin->value());
}

} // namespace openck
