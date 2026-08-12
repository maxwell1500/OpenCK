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
    auto* mainLayout = new QVBoxLayout(this);

    auto* group = new QGroupBox(QStringLiteral("Region Data"), this);
    auto* form = new QFormLayout(group);

    auto* editorIdEdit = new QLineEdit(group);
    editorIdEdit->setPlaceholderText(QStringLiteral("Editor ID"));

    auto* flagsSpin = new QSpinBox(group);
    flagsSpin->setRange(0, INT_MAX);

    form->addRow(QStringLiteral("Editor ID:"), editorIdEdit);
    form->addRow(QStringLiteral("Flags:"), flagsSpin);
    mainLayout->addWidget(group);

    if (m_recordPtr)
    {
        auto* rec = static_cast<RegionRecord*>(m_recordPtr);
        editorIdEdit->setText(rec->editorId);
        flagsSpin->setValue(static_cast<int>(rec->flags));
    }
}

RegnDataWidget::~RegnDataWidget() = default;

} // namespace openck
