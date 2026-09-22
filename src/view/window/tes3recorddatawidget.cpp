#include "tes3recorddatawidget.hpp"
#include "rawsubrecordwidget.hpp"
#include "../../../libs/files/esm/Tes3record.hpp"
#include "../../../libs/files/esm/common.hpp"
#include "../../../libs/components/formcomponents.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QVBoxLayout>

namespace openck {

Tes3RecordDataWidget::Tes3RecordDataWidget(void* recordPtr,
                                           FormComponents* components,
                                           QWidget* parent)
    : QWidget(parent)
    , m_recordPtr(recordPtr)
{
    Q_UNUSED(components);

    auto* mainLayout = new QVBoxLayout(this);

    auto* idGroup = new QGroupBox(QStringLiteral("TES3 Record"), this);
    auto* idForm = new QFormLayout(idGroup);

    auto* codeEdit = new QLineEdit(idGroup);
    codeEdit->setObjectName(QStringLiteral("recordCode"));
    codeEdit->setReadOnly(true);
    idForm->addRow(QStringLiteral("Record Code:"), codeEdit);

    auto* editorIdEdit = new QLineEdit(idGroup);
    editorIdEdit->setObjectName(QStringLiteral("editorId"));
    editorIdEdit->setReadOnly(true);
    idForm->addRow(QStringLiteral("Editor ID:"), editorIdEdit);

    auto* formIdEdit = new QLineEdit(idGroup);
    formIdEdit->setObjectName(QStringLiteral("formId"));
    formIdEdit->setReadOnly(true);
    idForm->addRow(QStringLiteral("Form ID:"), formIdEdit);

    auto* flagsEdit = new QLineEdit(idGroup);
    flagsEdit->setObjectName(QStringLiteral("flags"));
    flagsEdit->setReadOnly(true);
    idForm->addRow(QStringLiteral("Flags:"), flagsEdit);

    mainLayout->addWidget(idGroup);

    auto* rawGroup = new QGroupBox(QStringLiteral("Raw subrecords"), this);
    auto* rawLayout = new QVBoxLayout(rawGroup);
    auto* rawWidget = new RawSubrecordWidget(rawGroup);
    rawWidget->setObjectName(QStringLiteral("rawSubrecords"));
    rawLayout->addWidget(rawWidget);
    mainLayout->addWidget(rawGroup);

    if (m_recordPtr)
    {
        auto* rec = static_cast<Tes3Record*>(m_recordPtr);
        codeEdit->setText(nameToQString(rec->code));
        editorIdEdit->setText(rec->editorId);
        formIdEdit->setText(
            QStringLiteral("0x%1").arg(rec->formId, 8, 16, QChar('0')));
        flagsEdit->setText(
            QStringLiteral("0x%1").arg(rec->flags, 8, 16, QChar('0')));
        rawWidget->setSubrecords(rec->rawSubRecords);
    }
}

Tes3RecordDataWidget::~Tes3RecordDataWidget() = default;

} // namespace openck
