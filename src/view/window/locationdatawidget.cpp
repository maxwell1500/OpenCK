#include "locationdatawidget.hpp"

#include "../libs/files/esm/locationrecord.hpp"
#include "../libs/components/formcomponents.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

LocationDataWidget::LocationDataWidget(void* recordPtr,
                                       openck::FormComponents*,
                                       QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox(QStringLiteral("Location Data"), this);
    auto* infoForm = new QFormLayout(infoGroup);

    auto* editorIdEdit = new QLineEdit(infoGroup);
    editorIdEdit->setObjectName(QStringLiteral("editorId"));
    editorIdEdit->setPlaceholderText(QStringLiteral("Editor ID"));

    auto* nameEdit = new QLineEdit(infoGroup);
    nameEdit->setObjectName(QStringLiteral("name"));
    nameEdit->setPlaceholderText(QStringLiteral("Location Name"));

    auto* parentSpin = new QSpinBox(infoGroup);
    parentSpin->setRange(0, INT_MAX);
    auto* xSpin = new QSpinBox(infoGroup);
    xSpin->setRange(0, INT_MAX);
    auto* ySpin = new QSpinBox(infoGroup);
    ySpin->setRange(0, INT_MAX);
    auto* zSpin = new QSpinBox(infoGroup);
    zSpin->setRange(0, INT_MAX);

    infoForm->addRow(QStringLiteral("Editor ID:"), editorIdEdit);
    infoForm->addRow(QStringLiteral("Location Name:"), nameEdit);
    infoForm->addRow(QStringLiteral("Parent ID:"), parentSpin);
    infoForm->addRow(QStringLiteral("X:"), xSpin);
    infoForm->addRow(QStringLiteral("Y:"), ySpin);
    infoForm->addRow(QStringLiteral("Z:"), zSpin);
    mainLayout->addWidget(infoGroup);

    if (m_recordPtr)
    {
        auto* rec = static_cast<LocationRecord*>(m_recordPtr);
        editorIdEdit->setText(rec->editorId);
        nameEdit->setText(rec->locationName);
        parentSpin->setValue(static_cast<int>(rec->parentId));
        xSpin->setValue(static_cast<int>(rec->x));
        ySpin->setValue(static_cast<int>(rec->y));
        zSpin->setValue(static_cast<int>(rec->z));
    }
}

LocationDataWidget::~LocationDataWidget() = default;
