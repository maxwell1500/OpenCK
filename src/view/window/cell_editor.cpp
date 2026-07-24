#include "cell_editor.hpp"
#include "cellreferenceeditor.hpp"
#include "nifviewportwidget.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "CellRecord.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QDataStream>

CellEditor::CellEditor(Data* data, CellRecord* cell, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mCell(cell),
      mEditorIdEdit(nullptr),
      mCellNameEdit(nullptr),
      mCellXSpin(nullptr),
      mCellYSpin(nullptr),
      mOwnerSpin(nullptr),
      mLockLevelSpin(nullptr)
{
    setupUI();
    loadFromCell();
}

void CellEditor::setupUI()
{
    setWindowTitle("Cell Editor");
    setMinimumSize(400, 400);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Cell Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mCellNameEdit = new QLineEdit();
    infoLayout->addRow("Cell Name:", mCellNameEdit);

    infoLayout->addRow("", new QLabel("<b>Properties</b>"));

    mCellXSpin = new QSpinBox();
    mCellXSpin->setRange(-99999, 99999);
    infoLayout->addRow("Cell X:", mCellXSpin);

    mCellYSpin = new QSpinBox();
    mCellYSpin->setRange(-99999, 99999);
    infoLayout->addRow("Cell Y:", mCellYSpin);

    mOwnerSpin = new QSpinBox();
    mOwnerSpin->setRange(0, 9999);
    infoLayout->addRow("Owner:", mOwnerSpin);

    mLockLevelSpin = new QSpinBox();
    mLockLevelSpin->setRange(0, 100);
    infoLayout->addRow("Lock Level:", mLockLevelSpin);

    mainLayout->addWidget(infoGroup);

    auto* refBtn = new QPushButton("References...");
    infoLayout->addRow("", refBtn);
    connect(refBtn, &QPushButton::clicked, this, &CellEditor::openReferences);

    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &CellEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void CellEditor::loadFromCell()
{
    mEditorIdEdit->setText(mCell->editorId);
    mCellNameEdit->setText(mCell->cellName);
    mCellXSpin->setValue(mCell->cellX);
    mCellYSpin->setValue(mCell->cellY);
    mOwnerSpin->setValue(mCell->owner);
    mLockLevelSpin->setValue(mCell->lockLevel);
}

bool CellEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getCellCollection().searchId(editorId) >= 0)
    {
        if (editorId != mCell->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "A cell with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void CellEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    {
        auto results = ColumnValidator::validateCell(*mCell, mData);
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

    mCell->editorId = mEditorIdEdit->text();
    mCell->cellName = mCellNameEdit->text();
    mCell->cellX = mCellXSpin->value();
    mCell->cellY = mCellYSpin->value();
    mCell->owner = mOwnerSpin->value();
    mCell->lockLevel = mLockLevelSpin->value();

    accept();
}

NifViewportWidget* CellEditor::findViewport() const
{
    QWidget* w = parentWidget();
    while (w) {
        if (auto* viewport = w->findChild<NifViewportWidget*>()) {
            return viewport;
        }
        w = w->parentWidget();
    }
    return nullptr;
}

void CellEditor::openReferences()
{
    CellRecord editedState = *mCell;
    CellReferenceEditor editor(&editedState, this);
    if (editor.exec() == QDialog::Accepted)
    {
        QVector<CellRefEntry> refs = editor.getReferences();

        static const NAME REFR_NAME = 'REFR';

        for (auto it = editedState.rawSubRecords.begin(); it != editedState.rawSubRecords.end();)
        {
            if (it->name == REFR_NAME)
            {
                it = editedState.rawSubRecords.erase(it);
            }
            else
            {
                ++it;
            }
        }

        for (const auto& ref : refs)
        {
            QByteArray data;
            QDataStream stream(&data, QIODevice::WriteOnly);
            stream.setByteOrder(QDataStream::LittleEndian);

            stream << ref.formId << ref.baseObject
                   << ref.posX << ref.posY << ref.posZ
                   << ref.rotX << ref.rotY << ref.rotZ
                   << ref.scale << ref.flags;

            RawSubRecord raw;
            raw.name = REFR_NAME;
            raw.data = data;
            editedState.rawSubRecords.append(raw);
        }

        mCell->rawSubRecords = editedState.rawSubRecords;

        if (NifViewportWidget* viewport = findViewport())
        {
            QVector<ViewportCellRef> cellRefs;
            cellRefs.reserve(refs.size());
            for (const auto& ref : refs)
            {
                ViewportCellRef vref;
                vref.position = QVector3D(ref.posX, ref.posY, ref.posZ);
                vref.enabled = !ref.isDisabled();
                cellRefs.append(vref);
            }
            viewport->setCellReferences(cellRefs);
        }
    }
}
