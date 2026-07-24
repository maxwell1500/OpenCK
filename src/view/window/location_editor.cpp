#include "location_editor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "LocationRecord.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QMessageBox>

LocationEditor::LocationEditor(Data* data, LocationRecord* location, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mLocation(location),
      mEditorIdEdit(nullptr),
      mLocationNameEdit(nullptr),
      mParentIdSpin(nullptr),
      mXSpin(nullptr),
      mYSpin(nullptr),
      mZSpin(nullptr)
{
    setupUI();
    loadFromLocation();
}

void LocationEditor::setupUI()
{
    setWindowTitle("Location Editor");
    setMinimumSize(400, 350);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Location Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mLocationNameEdit = new QLineEdit();
    infoLayout->addRow("Location Name:", mLocationNameEdit);

    infoLayout->addRow("", new QLabel("<b>Properties</b>"));

    mParentIdSpin = new QSpinBox();
    mParentIdSpin->setRange(0, 9999);
    infoLayout->addRow("Parent ID:", mParentIdSpin);

    infoLayout->addRow("", new QLabel("<b>Position</b>"));

    mXSpin = new QSpinBox();
    mXSpin->setRange(-999999, 999999);
    infoLayout->addRow("X:", mXSpin);

    mYSpin = new QSpinBox();
    mYSpin->setRange(-999999, 999999);
    infoLayout->addRow("Y:", mYSpin);

    mZSpin = new QSpinBox();
    mZSpin->setRange(-999999, 999999);
    infoLayout->addRow("Z:", mZSpin);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &LocationEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void LocationEditor::loadFromLocation()
{
    mEditorIdEdit->setText(mLocation->editorId);
    mLocationNameEdit->setText(mLocation->locationName);
    mParentIdSpin->setValue(mLocation->parentId);
    mXSpin->setValue(mLocation->x);
    mYSpin->setValue(mLocation->y);
    mZSpin->setValue(mLocation->z);
}

bool LocationEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getLocationCollection().searchId(editorId) >= 0)
    {
        if (editorId != mLocation->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "A location with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void LocationEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    {
        auto results = ColumnValidator::validateLocation(*mLocation, mData);
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

    mLocation->editorId = mEditorIdEdit->text();
    mLocation->locationName = mLocationNameEdit->text();
    mLocation->parentId = mParentIdSpin->value();
    mLocation->x = mXSpin->value();
    mLocation->y = mYSpin->value();
    mLocation->z = mZSpin->value();

    accept();
}
