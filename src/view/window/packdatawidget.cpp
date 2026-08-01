#include "packdatawidget.hpp"

#include "../libs/files/esm/Packagerecord.hpp"
#include "../libs/components/formcomponents.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>
#include <QVBoxLayout>

PackDataWidget::PackDataWidget(void* recordPtr,
                               openck::FormComponents*,
                               QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox(QStringLiteral("Package Data"), this);
    auto* infoForm = new QFormLayout(infoGroup);

    auto* editorIdEdit = new QLineEdit(infoGroup);
    editorIdEdit->setObjectName(QStringLiteral("editorId"));
    editorIdEdit->setPlaceholderText(QStringLiteral("Editor ID"));

    auto* typeSpin = new QSpinBox(infoGroup);
    typeSpin->setRange(0, INT_MAX);
    auto* targetTypeSpin = new QSpinBox(infoGroup);
    targetTypeSpin->setRange(0, INT_MAX);
    auto* flagsSpin = new QSpinBox(infoGroup);
    flagsSpin->setRange(0, INT_MAX);

    infoForm->addRow(QStringLiteral("Editor ID:"), editorIdEdit);
    infoForm->addRow(QStringLiteral("Package Type:"), typeSpin);
    infoForm->addRow(QStringLiteral("Target Type:"), targetTypeSpin);
    infoForm->addRow(QStringLiteral("Flags:"), flagsSpin);
    mainLayout->addWidget(infoGroup);

    auto* targetsGroup = new QGroupBox(QStringLiteral("Targets"), this);
    auto* targetsLayout = new QVBoxLayout(targetsGroup);
    auto* targetsList = new QListWidget(targetsGroup);
    targetsLayout->addWidget(targetsList);
    mainLayout->addWidget(targetsGroup);

    if (m_recordPtr)
    {
        auto* rec = static_cast<PackageRecord*>(m_recordPtr);
        editorIdEdit->setText(rec->editorId);
        typeSpin->setValue(static_cast<int>(rec->packageType));
        targetTypeSpin->setValue(static_cast<int>(rec->targetType));
        flagsSpin->setValue(static_cast<int>(rec->flags));
        for (quint32 id : rec->targetIds)
            targetsList->addItem(QStringLiteral("0x%1").arg(id, 8, 16, QChar('0')));
    }
}

PackDataWidget::~PackDataWidget() = default;
