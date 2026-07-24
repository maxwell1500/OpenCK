#include "lcrteditor.hpp"

#include "../../model/world/data.hpp"
#include "lcrt.hpp"
#include "logger.hpp"
#include "../../model/tools/columnvalidator.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>

LcrtEditor::LcrtEditor(Data* data, LocationRefType* lcrt, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mLcrt(lcrt),
      mEditorIdEdit(nullptr),
      mColorEdit(nullptr)
{
    setupUI();
    loadFromLcrt();
}

void LcrtEditor::setupUI()
{
    setWindowTitle("Location Reference Type Editor");
    setMinimumSize(400, 200);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Location Reference Type Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mColorEdit = new QLineEdit();
    mColorEdit->setMaxLength(8);
    infoLayout->addRow("Color:", mColorEdit);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &LcrtEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void LcrtEditor::loadFromLcrt()
{
    mEditorIdEdit->setText(mLcrt->editorId);
    mColorEdit->setText(QString::number(mLcrt->color, 16).toUpper().rightJustified(8, '0'));
}

bool LcrtEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getLcrtCollection().searchId(editorId) >= 0)
    {
        if (editorId != mLcrt->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "A location reference type with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void LcrtEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    auto results = ColumnValidator::validateLcrt(*mLcrt, mData);
    QStringList errorMessages;
    for (const auto& r : results)
    {
        if (r.severity == ColumnValidator::Severity::Error)
        {
            errorMessages << QString("%1: %2").arg(r.field, r.message);
        }
    }
    if (!errorMessages.isEmpty())
    {
        QMessageBox::warning(this, tr("Validation Errors"), errorMessages.join("\n"));
        return;
    }

    mLcrt->editorId = mEditorIdEdit->text();

    QString colorText = mColorEdit->text().remove("#").remove("0x").toUpper();
    bool ok = false;
    uint32_t color = colorText.toUInt(&ok, 16);
    if (ok)
    {
        mLcrt->color = color;
    }

    accept();
}
