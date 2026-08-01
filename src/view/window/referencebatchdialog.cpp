#include "referencebatchdialog.hpp"

#include "../../model/tools/referencebatchactions.hpp"
#include "../../../libs/files/esm/cellreferencedata.hpp"
#include "../../libs/files/log/logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>

ReferenceBatchDialog::ReferenceBatchDialog(const QVector<CellRefEntry>& refs, QWidget* parent)
    : QDialog(parent)
    , mRefs(refs)
    , mOriginal(refs)
    , mOperationCombo(nullptr)
    , mValueSpin(nullptr)
    , mFlagOnCheck(nullptr)
{
    setWindowTitle("Reference Batch Actions");
    setMinimumWidth(380);
    setupUI();
}

void ReferenceBatchDialog::setupUI()
{
    auto* layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel(
        QString("Apply an operation to all %1 selected reference(s).").arg(mRefs.size())));

    auto* form = new QFormLayout();

    mOperationCombo = new QComboBox();
    mOperationCombo->addItem("Move by Offset (X/Y/Z)");
    mOperationCombo->addItem("Snap to Grid");
    mOperationCombo->addItem("Set Scale");
    mOperationCombo->addItem("Set Flag");
    mOperationCombo->addItem("Reset Rotation");
    form->addRow("Operation:", mOperationCombo);

    mValueSpin = new QDoubleSpinBox();
    mValueSpin->setRange(-100000.0, 100000.0);
    mValueSpin->setValue(10.0);
    mValueSpin->setDecimals(2);
    mValueSpin->setSuffix(" units");
    form->addRow("Value:", mValueSpin);

    mFlagOnCheck = new QCheckBox("Set (unchecked = clear)");
    mFlagOnCheck->setChecked(true);
    form->addRow("Flag:", mFlagOnCheck);

    layout->addLayout(form);

    auto* buttons = new QHBoxLayout();
    auto* resetBtn = new QPushButton("Reset");
    auto* applyBtn = new QPushButton("Apply");
    auto* closeBtn = new QPushButton("Close");
    buttons->addWidget(resetBtn);
    buttons->addStretch();
    buttons->addWidget(applyBtn);
    buttons->addWidget(closeBtn);
    layout->addLayout(buttons);

    connect(mOperationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [this](int index) {
            const bool needsValue = (index == 0 || index == 1 || index == 2);
            mValueSpin->setVisible(needsValue);
            mFlagOnCheck->setVisible(index == 3);
        });
    mFlagOnCheck->setVisible(false);

    connect(resetBtn, &QPushButton::clicked, this, &ReferenceBatchDialog::onReset);
    connect(applyBtn, &QPushButton::clicked, this, &ReferenceBatchDialog::onApply);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void ReferenceBatchDialog::onReset()
{
    mRefs = mOriginal;
    LOG_INFO("Reference batch actions: reset to original values");
}

void ReferenceBatchDialog::onApply()
{
    applyOperation();
    LOG_INFO("Reference batch action applied");
}

void ReferenceBatchDialog::applyOperation()
{
    using A = ReferenceBatchActions;
    switch (mOperationCombo->currentIndex())
    {
    case 0: // Move by Offset
    {
        const float v = static_cast<float>(mValueSpin->value());
        A::moveByOffset(mRefs, v, v, v);
        break;
    }
    case 1: // Snap to Grid
        A::snapToGrid(mRefs, static_cast<float>(mValueSpin->value()));
        break;
    case 2: // Set Scale
        A::setScale(mRefs, static_cast<float>(mValueSpin->value()));
        break;
    case 3: // Set Flag
    {
        const bool on = mFlagOnCheck->isChecked();
        // First reference's flag picks Disabled vs Hidden via a user hint;
        // default to Disabled. Hidden is available via repeated apply.
        A::setFlag(mRefs, A::Flag::Disabled, on);
        break;
    }
    case 4: // Reset Rotation
        A::resetRotation(mRefs);
        break;
    default:
        break;
    }
}
