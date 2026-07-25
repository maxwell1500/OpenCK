#include "classdatawidget.hpp"
#include "../libs/files/esm/classrecord.hpp"
#include "../libs/components/formcomponents.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>

namespace openck {

ClassDataWidget::ClassDataWidget(void* recordPtr,
                                 FormComponents* components,
                                 QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* group = new QGroupBox(QStringLiteral("Class Data"), this);
    auto* form = new QFormLayout(group);

    auto* classNameEdit = new QLineEdit(group);
    classNameEdit->setPlaceholderText(QStringLiteral("Class name"));

    auto* descEdit = new QLineEdit(group);
    descEdit->setPlaceholderText(QStringLiteral("Description"));

    auto* serviceFlagsSpin = new QSpinBox(group);
    serviceFlagsSpin->setRange(0, INT_MAX);

    auto* iconPathEdit = new QLineEdit(group);
    iconPathEdit->setPlaceholderText(QStringLiteral("Icon path"));

    form->addRow(QStringLiteral("Class Name:"), classNameEdit);
    form->addRow(QStringLiteral("Description:"), descEdit);
    form->addRow(QStringLiteral("Service Flags:"), serviceFlagsSpin);
    form->addRow(QStringLiteral("Icon Path:"), iconPathEdit);
    mainLayout->addWidget(group);

    if (m_recordPtr)
    {
        auto* rec = static_cast<ClassRecord*>(m_recordPtr);
        classNameEdit->setText(rec->className);
        descEdit->setText(rec->description);
        serviceFlagsSpin->setValue(static_cast<int>(rec->serviceFlags));
        iconPathEdit->setText(rec->iconPath);
    }
}

ClassDataWidget::~ClassDataWidget() = default;

} // namespace openck
