#include "armor_editor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "Armorrecord.hpp"
#include "logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QDialog>
#include <QColor>

static QString enchantmentName(const Data* data, quint32 formId)
{
    if (formId == 0)
        return QStringLiteral("None");

    const auto& records = data->getEnchCollection().getRecords();
    for (const auto& rec : records) {
        if (rec.get().formId == formId)
            return rec.get().editorId;
    }
    return QStringLiteral("0x%1").arg(formId, 8, 16, QChar('0')).toUpper();
}

ArmorEditor::ArmorEditor(Data* data, ArmorRecord* armor, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mRecord(armor),
      mEditorIdEdit(nullptr),
      mFullNameEdit(nullptr),
      mValueSpin(nullptr),
      mWeightSpin(nullptr),
      mArmorSpin(nullptr),
      mHealthSpin(nullptr)
{
    mOriginalEditorId = mRecord->editorId;
    setupUI();
    loadFromArmor();
}

void ArmorEditor::setupUI()
{
    setWindowTitle("Armor Editor");
    setMinimumSize(400, 300);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox("Armor Information");
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow("Editor ID:", mEditorIdEdit);

    mFullNameEdit = new QLineEdit();
    infoLayout->addRow("Full Name:", mFullNameEdit);

    infoLayout->addRow("", new QLabel("<b>Stats</b>"));

    mValueSpin = new QSpinBox();
    mValueSpin->setRange(-999999, 999999);
    mValueSpin->setSingleStep(10);
    infoLayout->addRow("Value:", mValueSpin);

    mWeightSpin = new QSpinBox();
    mWeightSpin->setRange(0, 9999);
    mWeightSpin->setSingleStep(10);
    infoLayout->addRow("Weight:", mWeightSpin);

    mArmorSpin = new QSpinBox();
    mArmorSpin->setRange(0, 9999);
    infoLayout->addRow("Armor Rating:", mArmorSpin);

    mHealthSpin = new QSpinBox();
    mHealthSpin->setRange(0, 9999);
    infoLayout->addRow("Health:", mHealthSpin);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* compareBtn = new QPushButton("Compare With...");
    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(compareBtn);
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(compareBtn, &QPushButton::clicked, this, &ArmorEditor::compareArmor);
    connect(saveBtn, &QPushButton::clicked, this, &ArmorEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void ArmorEditor::loadFromArmor()
{
    mEditorIdEdit->setText(mRecord->editorId);
    mFullNameEdit->setText(mRecord->fullName);
    mValueSpin->setValue(mRecord->value);
    mWeightSpin->setValue(static_cast<int>(mRecord->weight * 100.0f));
    mArmorSpin->setValue(mRecord->armorRating);
    mHealthSpin->setValue(static_cast<int>(mRecord->health));
}

bool ArmorEditor::validate()
{
    QString editorId = mEditorIdEdit->text().trimmed();
    if (editorId.isEmpty())
    {
        QMessageBox::warning(this, "Validation Error", "Editor ID cannot be empty.");
        return false;
    }

    auto* data = static_cast<Data*>(mData);
    if (data && data->getArmorCollection().searchId(editorId) >= 0)
    {
        if (editorId != mRecord->editorId)
        {
            QMessageBox::warning(this, "Validation Error", "An armor with this Editor ID already exists.");
            return false;
        }
    }

    return true;
}

void ArmorEditor::saveRecord()
{
    if (!validate())
    {
        return;
    }

    {
        auto results = ColumnValidator::validateArmor(*mRecord, mData, mOriginalEditorId);
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

    mRecord->editorId = mEditorIdEdit->text();
    mRecord->fullName = mFullNameEdit->text();
    mRecord->value = mValueSpin->value();
    mRecord->weight = static_cast<float>(mWeightSpin->value()) / 100.0f;
    mRecord->armorRating = mArmorSpin->value();
    mRecord->health = static_cast<float>(mHealthSpin->value());

    accept();
}

void ArmorEditor::compareArmor()
{
    QStringList armorIds;
    const auto& records = mData->getArmorCollection().getRecords();

    for (const auto& rec : records) {
        const auto& ar = rec.get();
        if (ar.editorId != mRecord->editorId)
            armorIds << ar.editorId;
    }

    if (armorIds.isEmpty()) {
        QMessageBox::information(this, "Compare", "No other armor pieces available for comparison.");
        return;
    }

    bool ok = false;
    QString selected = QInputDialog::getItem(this, "Compare Armor",
        "Select armor to compare with:", armorIds, 0, false, &ok);

    if (!ok || selected.isEmpty())
        return;

    const auto& cmpRecord = mData->getArmorCollection().getRecord(selected);
    const ArmorRecord& cmp = cmpRecord.get();

    auto* dialog = new QDialog(this);
    dialog->setWindowTitle(QString("Comparing: %1 vs %2").arg(mRecord->editorId, selected));
    dialog->setMinimumSize(450, 280);

    auto* layout = new QVBoxLayout(dialog);
    auto* table = new QTableWidget(5, 3, dialog);
    table->setHorizontalHeaderLabels({"Stat", mRecord->editorId, selected});
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);

    struct Row {
        QString stat;
        double curVal;
        double cmpVal;
        bool higherIsBetter;
    };

    QVector<Row> rows = {
        {"Armor Rating", (double)mRecord->armorRating, (double)cmp.armorRating, true},
        {"Weight",       (double)mRecord->weight,       (double)cmp.weight,       false},
        {"Value",        (double)mRecord->value,         (double)cmp.value,        true},
        {"Health",       (double)mRecord->health,        (double)cmp.health,       true},
    };

    for (int i = 0; i < rows.size(); ++i) {
        const auto& row = rows[i];

        auto* statItem = new QTableWidgetItem(row.stat);
        statItem->setFlags(statItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(i, 0, statItem);

        auto* curItem = new QTableWidgetItem(QString::number(row.curVal, 'f', 1));
        curItem->setFlags(curItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(i, 1, curItem);

        auto* cmpItem = new QTableWidgetItem(QString::number(row.cmpVal, 'f', 1));
        cmpItem->setFlags(cmpItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(i, 2, cmpItem);

        if (row.curVal != row.cmpVal) {
            bool curBetter = row.higherIsBetter ? (row.curVal > row.cmpVal) : (row.curVal < row.cmpVal);
            if (curBetter) {
                curItem->setBackground(QColor(144, 238, 144));
                cmpItem->setBackground(QColor(255, 182, 193));
            } else {
                curItem->setBackground(QColor(255, 182, 193));
                cmpItem->setBackground(QColor(144, 238, 144));
            }
        }
    }

    table->resizeColumnsToContents();
    layout->addWidget(table);

    auto* closeBtn = new QPushButton("Close");
    layout->addWidget(closeBtn);
    QObject::connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);

    dialog->exec();
    dialog->deleteLater();
}
