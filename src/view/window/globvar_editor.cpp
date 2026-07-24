#include "globvar_editor.hpp"

#include "../../model/world/data.hpp"
#include "../../../libs/files/esm/glob.hpp"
#include "../../model/tools/columnvalidator.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>

GlobVarEditor::GlobVarEditor(Data* data, GlobalVariable* glob, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mGlob(glob),
      mEditorIdEdit(nullptr),
      mValueSpin(nullptr),
      mFlagCombo(nullptr)
{
    setupUI();
    loadFromGlob();
}

void GlobVarEditor::setupUI()
{
    setWindowTitle("Global Variable Editor");
    setMinimumSize(350, 200);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Global Variable Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mValueSpin = new QDoubleSpinBox();
    mValueSpin->setRange(-999999, 999999);
    mValueSpin->setDecimals(4);
    mValueSpin->setSingleStep(0.01);
    infoLayout->addRow("Value:", mValueSpin);

    mFlagCombo = new QComboBox();
    mFlagCombo->addItem("None");
    mFlagCombo->addItem("Constant");
    infoLayout->addRow("Flags:", mFlagCombo);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &GlobVarEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void GlobVarEditor::loadFromGlob()
{
    mEditorIdEdit->setText(mGlob->editorId);
    VariantType vt = mGlob->value.getType();
    double val = 0.0;
    if (vt == Var_Float) val = mGlob->value.getFloat();
    else if (vt == Var_Int) val = mGlob->value.getInt();
    else if (vt == Var_Short) val = mGlob->value.getShort();
    else if (vt == Var_Bool) val = mGlob->value.getBool() ? 1.0 : 0.0;
    mValueSpin->setValue(val);
    mFlagCombo->setCurrentIndex(mGlob->constant ? 1 : 0);
}

void GlobVarEditor::saveToGlob()
{
    mGlob->value.setFloat(static_cast<float>(mValueSpin->value()));
    mGlob->constant = mFlagCombo->currentIndex() == 1;
}

bool GlobVarEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getGlobCollection().searchId(editorId) >= 0)
    {
        if (editorId != mGlob->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "A global variable with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void GlobVarEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    auto results = ColumnValidator::validateGlobal(*mGlob, mData);
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

    saveToGlob();
    accept();
}
