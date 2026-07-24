#include "pack_editor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "packagerecord.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>

PackEditor::PackEditor(Data* data, PackageRecord* pack, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mPack(pack),
      mEditorIdEdit(nullptr),
      mPackageTypeSpin(nullptr),
      mTargetTypeSpin(nullptr),
      mTargetIdsSpin(nullptr),
      mParametersSpin(nullptr)
{
    setupUI();
    loadFromPack();
}

void PackEditor::setupUI()
{
    setWindowTitle("Package Editor");
    setMinimumSize(400, 250);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Package Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mPackageTypeSpin = new QSpinBox();
    mPackageTypeSpin->setRange(0, 9999);
    infoLayout->addRow("Package Type:", mPackageTypeSpin);

    mTargetTypeSpin = new QSpinBox();
    mTargetTypeSpin->setRange(0, 9999);
    infoLayout->addRow("Target Type:", mTargetTypeSpin);

    mTargetIdsSpin = new QSpinBox();
    mTargetIdsSpin->setRange(0, 9999);
    mTargetIdsSpin->setReadOnly(true);
    infoLayout->addRow("Target IDs (read-only):", mTargetIdsSpin);

    mParametersSpin = new QSpinBox();
    mParametersSpin->setRange(0, 9999);
    mParametersSpin->setReadOnly(true);
    infoLayout->addRow("Parameters (read-only):", mParametersSpin);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &PackEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void PackEditor::loadFromPack()
{
    mEditorIdEdit->setText(mPack->editorId);
    mPackageTypeSpin->setValue(static_cast<int>(mPack->packageType));
    mTargetTypeSpin->setValue(static_cast<int>(mPack->targetType));
    mTargetIdsSpin->setValue(static_cast<int>(mPack->targetIds.size()));
    mParametersSpin->setValue(static_cast<int>(mPack->parameters.size()));
}

void PackEditor::saveToPack()
{
    mPack->editorId = mEditorIdEdit->text();
    mPack->packageType = static_cast<quint32>(mPackageTypeSpin->value());
    mPack->targetType = static_cast<quint32>(mTargetTypeSpin->value());
    Q_UNUSED(mTargetIdsSpin)
    Q_UNUSED(mParametersSpin)
}

bool PackEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getPackCollection().searchId(editorId) >= 0)
    {
        if (editorId != mPack->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "A package with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void PackEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    {
        auto results = ColumnValidator::validatePackage(*mPack, mData);
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

    saveToPack();
    accept();
}
