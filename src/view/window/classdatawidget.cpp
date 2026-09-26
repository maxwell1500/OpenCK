#include "classdatawidget.hpp"
#include "../libs/files/esm/classrecord.hpp"
#include "../libs/components/formcomponents.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace openck {

ClassDataWidget::ClassDataWidget(void* recordPtr,
                                 FormComponents* components,
                                 QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    Q_UNUSED(components);

    auto* mainLayout = new QVBoxLayout(this);

    auto* group = new QGroupBox(QStringLiteral("Class Data"), this);
    auto* form = new QFormLayout(group);

    auto* classNameEdit = new QLineEdit(group);
    classNameEdit->setObjectName(QStringLiteral("className"));
    classNameEdit->setPlaceholderText(QStringLiteral("Class name"));

    auto* descEdit = new QLineEdit(group);
    descEdit->setObjectName(QStringLiteral("description"));
    descEdit->setPlaceholderText(QStringLiteral("Description"));

    auto* serviceFlagsSpin = new QSpinBox(group);
    serviceFlagsSpin->setObjectName(QStringLiteral("serviceFlags"));
    serviceFlagsSpin->setRange(0, INT_MAX);

    auto* iconPathEdit = new QLineEdit(group);
    iconPathEdit->setObjectName(QStringLiteral("iconPath"));
    iconPathEdit->setPlaceholderText(QStringLiteral("Icon path"));

    form->addRow(QStringLiteral("Class Name:"), classNameEdit);
    form->addRow(QStringLiteral("Description:"), descEdit);
    form->addRow(QStringLiteral("Service Flags:"), serviceFlagsSpin);
    form->addRow(QStringLiteral("Icon Path:"), iconPathEdit);
    mainLayout->addWidget(group);

    loadSession();
}

ClassDataWidget::~ClassDataWidget() = default;

void ClassDataWidget::loadSession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<ClassRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("className")))
        edit->setText(rec->className);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("description")))
        edit->setText(rec->description);
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("serviceFlags")))
        spin->setValue(static_cast<int>(rec->serviceFlags));
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("iconPath")))
        edit->setText(rec->iconPath);
}

bool ClassDataWidget::validateSession(QString* error)
{
    Q_UNUSED(error);
    return true;
}

void ClassDataWidget::applySession()
{
    if (!m_recordPtr) return;
    auto* rec = static_cast<ClassRecord*>(m_recordPtr);
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("className")))
        rec->className = edit->text();
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("description")))
        rec->description = edit->text();
    if (auto* spin = findChild<QSpinBox*>(QStringLiteral("serviceFlags")))
        rec->serviceFlags = static_cast<quint32>(spin->value());
    if (auto* edit = findChild<QLineEdit*>(QStringLiteral("iconPath")))
        rec->iconPath = edit->text();
}

} // namespace openck
