#include "classeditor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "Classrecord.hpp"
#include "fieldvalidators.hpp"
#include "logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>

ClassEditor::ClassEditor(Data* data, ClassRecord* classRec, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mClass(classRec),
      mEditorIdEdit(nullptr),
      mClassNameEdit(nullptr),
      mDescriptionEdit(nullptr),
      mFactionSpin(nullptr),
      mIconPathEdit(nullptr)
{
    setupUI();
    loadFromClass();
}

void ClassEditor::setupUI()
{
    setWindowTitle("Class Editor");
    setMinimumSize(400, 350);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Class Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mClassNameEdit = new QLineEdit();
    infoLayout->addRow("Class Name:", mClassNameEdit);

    mDescriptionEdit = new QPlainTextEdit();
    mDescriptionEdit->setMaximumHeight(60);
    infoLayout->addRow("Description:", mDescriptionEdit);

    infoLayout->addRow("", new QLabel("<b>Details</b>"));

    mFactionSpin = new QSpinBox();
    setIntNonNegativeValidator(mFactionSpin);
    infoLayout->addRow("Faction:", mFactionSpin);

    mIconPathEdit = new QLineEdit();
    infoLayout->addRow("Icon Path:", mIconPathEdit);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &ClassEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void ClassEditor::loadFromClass()
{
    mEditorIdEdit->setText(mClass->editorId);
    mClassNameEdit->setText(mClass->className);
    mDescriptionEdit->setPlainText(mClass->description);
    mFactionSpin->setValue(static_cast<int>(mClass->serviceFlags));
    mIconPathEdit->setText(mClass->iconPath);
}

bool ClassEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getClassCollection().searchId(editorId) >= 0)
    {
        if (editorId != mClass->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "A class with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void ClassEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    {
        auto results = ColumnValidator::validateClass(*mClass, mData);
        QStringList errorMessages;
        for (const auto& r : results) {
            if (r.severity == ColumnValidator::Severity::Error) {
                errorMessages << QString("%1: %2").arg(r.field, r.message);
            }
        }
        if (!errorMessages.isEmpty()) {
            QMessageBox::warning(this, tr("Validation Errors"), errorMessages.join("\n"));
            return;
        }
    }

    mClass->editorId = mEditorIdEdit->text();
    mClass->className = mClassNameEdit->text();
    mClass->description = mDescriptionEdit->toPlainText();
    mClass->serviceFlags = static_cast<quint32>(mFactionSpin->value());
    mClass->iconPath = mIconPathEdit->text();

    accept();
}
