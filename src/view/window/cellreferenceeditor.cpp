#include "cellreferenceeditor.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QHeaderView>
#include <QMessageBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>

static const quint32 REF_DISABLED = 0x01;
static const quint32 REF_HIDDEN   = 0x02;

CellReferenceEditor::CellReferenceEditor(const QVector<CellRefEntry>& references,
                                         const QVector<FormPickerEntry>& formEntries,
                                         QWidget* parent)
    : QDialog(parent)
    , mReferences(references)
    , mFormEntries(formEntries)
    , mTable(nullptr)
    , mAddBtn(nullptr)
    , mRemoveBtn(nullptr)
    , mSaveBtn(nullptr)
    , mCancelBtn(nullptr)
{
    setupUI();
    populateTable();
}

void CellReferenceEditor::setupUI()
{
    setWindowTitle("Cell References");
    setMinimumSize(900, 500);

    auto* mainLayout = new QVBoxLayout(this);

    mTable = new QTableWidget(this);
    mTable->setColumnCount(10);
    mTable->setHorizontalHeaderLabels({
        "FormID", "Base Object",
        "Pos X", "Pos Y", "Pos Z",
        "Rot X", "Rot Y", "Rot Z",
        "Scale", "Flags"
    });
    mTable->horizontalHeader()->setStretchLastSection(true);
    mTable->setSelectionBehavior(QTableWidget::SelectRows);
    mTable->setSelectionMode(QTableWidget::SingleSelection);
    mTable->setAlternatingRowColors(true);
    mainLayout->addWidget(mTable);

    auto* btnLayout = new QHBoxLayout();
    mAddBtn = new QPushButton("Add Reference");
    mRemoveBtn = new QPushButton("Remove Selected");
    btnLayout->addWidget(mAddBtn);
    btnLayout->addWidget(mRemoveBtn);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    auto* dialogBtnLayout = new QHBoxLayout();
    dialogBtnLayout->addStretch();
    mSaveBtn = new QPushButton("Save");
    mCancelBtn = new QPushButton("Cancel");
    dialogBtnLayout->addWidget(mSaveBtn);
    dialogBtnLayout->addWidget(mCancelBtn);
    mainLayout->addLayout(dialogBtnLayout);

    connect(mAddBtn, &QPushButton::clicked, this, &CellReferenceEditor::addReference);
    connect(mRemoveBtn, &QPushButton::clicked, this, &CellReferenceEditor::removeSelected);
    connect(mSaveBtn, &QPushButton::clicked, this, &CellReferenceEditor::saveReferences);
    connect(mCancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void CellReferenceEditor::populateTable()
{
    mTable->setRowCount(mReferences.size());

    for (int i = 0; i < mReferences.size(); ++i)
    {
        setRowFromReference(i, mReferences[i]);
    }
}

void CellReferenceEditor::setRowFromReference(int row, const CellRefEntry& ref)
{
    auto createReadOnlyItem = [](const QString& text) -> QTableWidgetItem* {
        auto* item = new QTableWidgetItem(text);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        return item;
    };

    mTable->setItem(row, 0, createReadOnlyItem(QString::number(ref.formId)));
    auto* basePicker = new FormPickerWidget(mFormEntries, ref.baseObject, this);
    basePicker->setMaximumHeight(180);
    mTable->setCellWidget(row, 1, basePicker);

    auto* posXSpin = new QDoubleSpinBox();
    posXSpin->setRange(-100000.0, 100000.0);
    posXSpin->setValue(ref.posX);
    posXSpin->setDecimals(3);
    mTable->setCellWidget(row, 2, posXSpin);

    auto* posYSpin = new QDoubleSpinBox();
    posYSpin->setRange(-100000.0, 100000.0);
    posYSpin->setValue(ref.posY);
    posYSpin->setDecimals(3);
    mTable->setCellWidget(row, 3, posYSpin);

    auto* posZSpin = new QDoubleSpinBox();
    posZSpin->setRange(-100000.0, 100000.0);
    posZSpin->setValue(ref.posZ);
    posZSpin->setDecimals(3);
    mTable->setCellWidget(row, 4, posZSpin);

    auto* rotXSpin = new QDoubleSpinBox();
    rotXSpin->setRange(-6.28, 6.28);
    rotXSpin->setValue(ref.rotX);
    rotXSpin->setDecimals(4);
    mTable->setCellWidget(row, 5, rotXSpin);

    auto* rotYSpin = new QDoubleSpinBox();
    rotYSpin->setRange(-6.28, 6.28);
    rotYSpin->setValue(ref.rotY);
    rotYSpin->setDecimals(4);
    mTable->setCellWidget(row, 6, rotYSpin);

    auto* rotZSpin = new QDoubleSpinBox();
    rotZSpin->setRange(-6.28, 6.28);
    rotZSpin->setValue(ref.rotZ);
    rotZSpin->setDecimals(4);
    mTable->setCellWidget(row, 7, rotZSpin);

    auto* scaleSpin = new QDoubleSpinBox();
    scaleSpin->setRange(0.01, 100.0);
    scaleSpin->setValue(ref.scale);
    scaleSpin->setDecimals(2);
    mTable->setCellWidget(row, 8, scaleSpin);

    auto* flagsWidget = new QWidget();
    auto* flagsLayout = new QHBoxLayout(flagsWidget);
    flagsLayout->setContentsMargins(2, 2, 2, 2);
    auto* disabledCheck = new QCheckBox("Disabled");
    auto* hiddenCheck = new QCheckBox("Hidden");
    disabledCheck->setChecked(ref.isDisabled());
    hiddenCheck->setChecked(ref.isHidden());
    flagsLayout->addWidget(disabledCheck);
    flagsLayout->addWidget(hiddenCheck);
    mTable->setCellWidget(row, 9, flagsWidget);
}

CellRefEntry CellReferenceEditor::getReferenceFromRow(int row) const
{
    CellRefEntry ref;

    auto* formIdItem = mTable->item(row, 0);
    auto* baseObjItem = mTable->item(row, 1);
    if (formIdItem) {
        bool ok = false;
        ref.formId = formIdItem->text().toUInt(&ok);
        if (!ok) ref.formId = 0;
    }
    if (auto* picker = dynamic_cast<FormPickerWidget*>(mTable->cellWidget(row, 1)))
    {
        ref.baseObject = picker->value();
    }
    else if (baseObjItem) {
        bool ok = false;
        ref.baseObject = baseObjItem->text().toUInt(&ok);
        if (!ok) ref.baseObject = 0;
    }

    auto getDouble = [&](int col) -> double {
        auto* spin = qobject_cast<QDoubleSpinBox*>(mTable->cellWidget(row, col));
        return spin ? spin->value() : 0.0;
    };

    ref.posX = static_cast<float>(getDouble(2));
    ref.posY = static_cast<float>(getDouble(3));
    ref.posZ = static_cast<float>(getDouble(4));
    ref.rotX = static_cast<float>(getDouble(5));
    ref.rotY = static_cast<float>(getDouble(6));
    ref.rotZ = static_cast<float>(getDouble(7));
    ref.scale = static_cast<float>(getDouble(8));

    auto* flagsWidget = mTable->cellWidget(row, 9);
    if (flagsWidget)
    {
        auto* disabledCheck = flagsWidget->findChild<QCheckBox*>();
        auto* hiddenCheck = flagsWidget->findChildren<QCheckBox*>().value(1);
        ref.flags = 0;
        if (disabledCheck && disabledCheck->isChecked()) ref.flags |= REF_DISABLED;
        if (hiddenCheck && hiddenCheck->isChecked()) ref.flags |= REF_HIDDEN;
    }

    return ref;
}

void CellReferenceEditor::addReference()
{
    CellRefEntry ref;
    ref.scale = 1.0f;

    mReferences.append(ref);

    int newRow = mTable->rowCount();
    mTable->setRowCount(newRow + 1);
    setRowFromReference(newRow, ref);
}

void CellReferenceEditor::removeSelected()
{
    int row = mTable->currentRow();
    if (row < 0)
    {
        QMessageBox::information(this, "Remove Reference", "Select a row to remove.");
        return;
    }

    mTable->removeRow(row);
    mReferences.removeAt(row);
}

void CellReferenceEditor::saveReferences()
{
    int rowCount = mTable->rowCount();
    mReferences.clear();
    mReferences.reserve(rowCount);

    for (int i = 0; i < rowCount; ++i)
    {
        mReferences.append(getReferenceFromRow(i));
    }

    accept();
}
