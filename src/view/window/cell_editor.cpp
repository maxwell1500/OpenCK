#include "cell_editor.hpp"
#include "cellreferenceeditor.hpp"
#include "formideditorwidget.hpp"
#include "nifviewportwidget.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/tools/addrecordcommand.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "../../model/tools/deleterecordcommandbase.hpp"
#include "../../model/tools/editrecordcommand.hpp"
#include "../../model/tools/macrocommand.hpp"
#include "../../model/tools/setrefrparentcellcommand.hpp"
#include "../../model/tools/undostack.hpp"
#include "CellRecord.hpp"
#include "fieldvalidators.hpp"

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
#include <algorithm>
#include <functional>

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
    setIntRangeValidator(mCellXSpin, -99999, 99999, 1);
    infoLayout->addRow("Cell X:", mCellXSpin);

    mCellYSpin = new QSpinBox();
    setIntRangeValidator(mCellYSpin, -99999, 99999, 1);
    infoLayout->addRow("Cell Y:", mCellYSpin);

    mOwnerSpin = new QSpinBox();
    setIntNonNegativeValidator(mOwnerSpin);
    infoLayout->addRow("Owner:", mOwnerSpin);

    mLockLevelSpin = new QSpinBox();
    setLockLevelValidator(mLockLevelSpin);
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

    for (auto it = mCell->rawSubRecords.begin(); it != mCell->rawSubRecords.end();)
    {
        if (it->name == NAME('REFR'))
            it = mCell->rawSubRecords.erase(it);
        else
            ++it;
    }

    if (!applyReferenceChanges())
    {
        QMessageBox::warning(this, tr("Cell References"), tr("A new reference needs a valid base object."));
        return;
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

QVector<CellRefEntry> CellEditor::loadReferences() const
{
    QVector<CellRefEntry> references;
    if (!mData) return references;

    const auto& collection = mData->getRefrCollection();
    for (const auto& child : mData->childrenOfCell(mCell->formId))
    {
        if (child.type != NAME('REFR')) continue;
        int index = -1;
        for (int i = 0; i < collection.size(); ++i)
        {
            if (collection.getFormId(i) == child.formId)
            {
                index = i;
                break;
            }
        }
        if (index < 0) continue;

        const RefrRecord& record = collection.getRecord(index).get();
        CellRefEntry reference;
        reference.formId = record.formId;
        reference.baseObject = record.baseId;
        reference.posX = record.posX;
        reference.posY = record.posY;
        reference.posZ = record.posZ;
        reference.rotX = record.rotX;
        reference.rotY = record.rotY;
        reference.rotZ = record.rotZ;
        reference.scale = record.scale;
        reference.setDisabled(record.initiallyDisabled);
        references.append(reference);
    }
    return references;
}

QVector<FormPickerEntry> CellEditor::loadFormEntries() const
{
    QVector<FormPickerEntry> entries;
    if (!mData) return entries;
    for (const auto& typed : mData->allCollectionsWithTypes())
    {
        if (!typed.collection) continue;
        const QString typeName = CkId(typed.type).getTypeName();
        for (int i = 0; i < typed.collection->count(); ++i)
        {
            const quint32 formId = typed.collection->getFormId(i);
            if (formId == 0) continue;
            entries.append({formId, typed.collection->getEditorId(i), typeName});
        }
    }
    return entries;
}

bool CellEditor::applyReferenceChanges()
{
    if (!mReferencesEdited) return true;
    if (!mData || !mData->getUndoStack()) return false;

    auto& collection = mData->getRefrCollection();
    auto findIndex = [&collection](quint32 formId) {
        for (int i = 0; i < collection.size(); ++i)
            if (collection.getFormId(i) == formId) return i;
        return -1;
    };

    for (const CellRefEntry& reference : mEditedReferences)
    {
        if (reference.formId == 0 && reference.baseObject == 0)
            return false;
    }

    auto* macro = mData->createMacroCommand(QStringLiteral("Edit cell references"));
    auto* table = qobject_cast<IdTable*>(mData->getTableModel(CkId::Type_Refr_));
    if (!table)
    {
        delete macro;
        return false;
    }
    bool hasChanges = false;
    int nextAppendIndex = collection.size();

    for (const CellRefEntry& originalEntry : mOriginalReferences)
    {
        const auto editedIt = std::find_if(mEditedReferences.cbegin(), mEditedReferences.cend(),
            [&originalEntry](const CellRefEntry& entry) { return entry.formId == originalEntry.formId; });
        if (editedIt == mEditedReferences.cend()) continue;
        const int index = findIndex(originalEntry.formId);
        if (index < 0) continue;

        RefrRecord editedRecord = collection.getRecord(index).get();
        const RefrRecord originalRecord = editedRecord;
        editedRecord.baseId = editedIt->baseObject;
        editedRecord.posX = editedIt->posX;
        editedRecord.posY = editedIt->posY;
        editedRecord.posZ = editedIt->posZ;
        editedRecord.rotX = editedIt->rotX;
        editedRecord.rotY = editedIt->rotY;
        editedRecord.rotZ = editedIt->rotZ;
        editedRecord.scale = editedIt->scale;
        editedRecord.initiallyDisabled = editedIt->isDisabled();
        if (editedRecord != originalRecord)
        {
            macro->addCommand(new EditRecordCommand<RefrRecord>(&collection, index,
                originalRecord, editedRecord, QStringLiteral("Edit cell reference")));
            hasChanges = true;
        }
    }

    for (const CellRefEntry& editedEntry : mEditedReferences)
    {
        if (editedEntry.formId != 0) continue;
        RefrRecord newRecord;
        newRecord.blank();
        newRecord.initComponents();
        newRecord.formId = mData->createNewRecord(CkId::Type_Refr_, QString());
        newRecord.editorId = QStringLiteral("REFR_%1")
            .arg(newRecord.formId, 8, 16, QChar('0')).toUpper();
        newRecord.baseId = editedEntry.baseObject;
        newRecord.posX = editedEntry.posX;
        newRecord.posY = editedEntry.posY;
        newRecord.posZ = editedEntry.posZ;
        newRecord.rotX = editedEntry.rotX;
        newRecord.rotY = editedEntry.rotY;
        newRecord.rotZ = editedEntry.rotZ;
        newRecord.scale = editedEntry.scale;
        newRecord.initiallyDisabled = editedEntry.isDisabled();

        Record<RefrRecord> record(State_ModifiedOnly, nullptr, &newRecord);
        macro->addCommand(new AddRecordCommand(table, &collection, nextAppendIndex++, record,
            QStringLiteral("Add cell reference")));
        macro->addCommand(new SetRefrParentCellCommand(mData, newRecord.formId, 0, mCell->formId));
        hasChanges = true;
    }

    QVector<int> removals;
    for (const CellRefEntry& originalEntry : mOriginalReferences)
    {
        const bool kept = std::any_of(mEditedReferences.cbegin(), mEditedReferences.cend(),
            [&originalEntry](const CellRefEntry& entry) { return entry.formId == originalEntry.formId; });
        if (!kept)
        {
            const int index = findIndex(originalEntry.formId);
            if (index >= 0) removals.append(index);
        }
    }
    std::sort(removals.begin(), removals.end(), std::greater<int>());
    for (int index : removals)
    {
        const quint32 formId = collection.getFormId(index);
        macro->addCommand(new DeleteRecordCommandBase(&collection, index,
            QStringLiteral("Remove cell reference")));
        macro->addCommand(new SetRefrParentCellCommand(mData, formId, mCell->formId, 0));
        hasChanges = true;
    }

    if (!hasChanges)
    {
        delete macro;
        return true;
    }
    mData->getUndoStack()->push(macro);
    mReferencesEdited = false;
    return true;
}

void CellEditor::openReferences()
{
    mOriginalReferences = loadReferences();
    CellReferenceEditor editor(mOriginalReferences, loadFormEntries(), this);
    if (editor.exec() != QDialog::Accepted) return;

    mEditedReferences = editor.getReferences();
    mReferencesEdited = true;

    if (NifViewportWidget* viewport = findViewport())
    {
        QVector<ViewportCellRef> cellRefs;
        cellRefs.reserve(mEditedReferences.size());
        for (const auto& ref : mEditedReferences)
        {
            ViewportCellRef viewportRef;
            viewportRef.position = QVector3D(ref.posX, ref.posY, ref.posZ);
            viewportRef.enabled = !ref.isDisabled();
            cellRefs.append(viewportRef);
        }
        viewport->setCellReferences(cellRefs);
    }
}
