#include "weaponeditor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/tools/columnvalidator.hpp"
#include "weaprecord.hpp"
#include "fieldvalidators.hpp"
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
#include <QCoreApplication>

static QString weaponTypeName(quint32 type)
{
    switch (type) {
    case 0: return QCoreApplication::translate("WeaponEditor", "Melee");
    case 1: return QCoreApplication::translate("WeaponEditor", "Ranged");
    case 2: return QCoreApplication::translate("WeaponEditor", "Thrown");
    case 3: return QCoreApplication::translate("WeaponEditor", "ThrownRanged");
    default: return QCoreApplication::translate("WeaponEditor", "Unknown");
    }
}

static QString enchantmentName(const Data* data, quint32 formId)
{
    if (formId == 0)
        return QCoreApplication::translate("WeaponEditor", "None");

    const auto& records = data->getEnchCollection().getRecords();
    for (const auto& rec : records) {
        if (rec.get().formId == formId)
            return rec.get().editorId;
    }
    return QStringLiteral("0x%1").arg(formId, 8, 16, QChar('0')).toUpper();
}

WeaponEditor::WeaponEditor(Data* data, WeaponRecord* weapon, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mRecord(weapon),
      mEditorIdEdit(nullptr),
      mFullNameEdit(nullptr),
      mDamageSpin(nullptr),
      mSpeedSpin(nullptr),
      mReachSpin(nullptr),
      mWeightSpin(nullptr),
      mAttackSpin(nullptr),
      mValueSpin(nullptr),
      mTypeCombo(nullptr)
{
    mOriginalEditorId = mRecord->editorId;
    setupUI();
    loadFromWeapon();
}

void WeaponEditor::setupUI()
{
    setWindowTitle(tr("Weapon Editor"));
    setMinimumSize(450, 400);

    auto* mainLayout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox(tr("Weapon Information"));
    auto* infoLayout = new QFormLayout(infoGroup);

    mEditorIdEdit = new QLineEdit();
    mEditorIdEdit->setReadOnly(true);
    infoLayout->addRow(tr("Editor ID:"), mEditorIdEdit);

    mFullNameEdit = new QLineEdit();
    infoLayout->addRow(tr("Full Name:"), mFullNameEdit);

    mTypeCombo = new QComboBox();
    mTypeCombo->addItems({tr("Melee"), tr("Ranged"), tr("Thrown"), tr("ThrownRanged")});
    infoLayout->addRow(tr("Type:"), mTypeCombo);

    infoLayout->addRow("", new QLabel("<b>" + tr("Combat Stats") + "</b>"));

    mDamageSpin = new QDoubleSpinBox();
    setDamageValidator(mDamageSpin);
    infoLayout->addRow(tr("Damage:"), mDamageSpin);

    mSpeedSpin = new QDoubleSpinBox();
    setSpeedValidator(mSpeedSpin);
    infoLayout->addRow(tr("Speed:"), mSpeedSpin);

    mReachSpin = new QDoubleSpinBox();
    setReachValidator(mReachSpin);
    infoLayout->addRow(tr("Reach:"), mReachSpin);

    mAttackSpin = new QSpinBox();
    setAttackTypeValidator(mAttackSpin);
    infoLayout->addRow(tr("Attack Type:"), mAttackSpin);

    infoLayout->addRow("", new QLabel("<b>" + tr("Item Stats") + "</b>"));

    mWeightSpin = new QDoubleSpinBox();
    setWeightValidator(mWeightSpin);
    infoLayout->addRow(tr("Weight:"), mWeightSpin);

    mValueSpin = new QSpinBox();
    setValueValidator(mValueSpin);
    infoLayout->addRow(tr("Value:"), mValueSpin);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* compareBtn = new QPushButton(tr("Compare With..."));
    auto* saveBtn = new QPushButton(tr("Save"));
    auto* cancelBtn = new QPushButton(tr("Cancel"));
    buttonLayout->addWidget(compareBtn);
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(compareBtn, &QPushButton::clicked, this, &WeaponEditor::compareWeapon);
    connect(saveBtn, &QPushButton::clicked, this, &WeaponEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void WeaponEditor::loadFromWeapon()
{
    mEditorIdEdit->setText(mRecord->editorId);
    mFullNameEdit->setText(mRecord->fullName);
    mDamageSpin->setValue(mRecord->damage);
    mSpeedSpin->setValue(mRecord->speed);
    mReachSpin->setValue(mRecord->reach);
    mWeightSpin->setValue(mRecord->weight);
    mValueSpin->setValue(mRecord->value);
}

void WeaponEditor::saveRecord()
{
    if (!validateWeapon())
    {
        return;
    }

    {
        auto results = ColumnValidator::validateWeapon(*mRecord, mData, mOriginalEditorId);
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
    mRecord->damage = mDamageSpin->value();
    mRecord->speed = mSpeedSpin->value();
    mRecord->reach = mReachSpin->value();
    mRecord->weight = mWeightSpin->value();
    mRecord->value = mValueSpin->value();
    accept();
}

bool WeaponEditor::validateWeapon()
{
    int damage = mDamageSpin->value();
    if (damage < 0 || damage > 99999)
    {
        QMessageBox::warning(this, "Validation Error", 
            QString("Damage value %1 is invalid. Must be 0-99999.").arg(damage));
        return false;
    }

    double speed = mSpeedSpin->value();
    if (speed < 0 || speed > 999)
    {
        QMessageBox::warning(this, "Validation Error", 
            QString("Speed value %1 is invalid. Must be 0-999.").arg(speed));
        return false;
    }

    double weight = mWeightSpin->value();
    if (weight < 0 || weight > 999)
    {
        QMessageBox::warning(this, "Validation Error", 
            QString("Weight value %1 is invalid. Must be 0-999.").arg(weight));
        return false;
    }

    int value = mValueSpin->value();
    if (value > 9999999)
    {
        QMessageBox::warning(this, "Validation Error", 
            QString("Value %1 is invalid. Must be 0-9999999.").arg(value));
        return false;
    }

    return true;
}

void WeaponEditor::compareWeapon()
{
    QStringList weaponIds;
    const auto& records = mData->getWeaponCollection().getRecords();

    for (const auto& rec : records) {
        const auto& wr = rec.get();
        if (wr.editorId != mRecord->editorId)
            weaponIds << wr.editorId;
    }

    if (weaponIds.isEmpty()) {
        QMessageBox::information(this, "Compare", "No other weapons available for comparison.");
        return;
    }

    bool ok = false;
    QString selected = QInputDialog::getItem(this, "Compare Weapons",
        "Select weapon to compare with:", weaponIds, 0, false, &ok);

    if (!ok || selected.isEmpty())
        return;

    const auto& cmpRecord = mData->getWeaponCollection().getRecord(selected);
    const WeaponRecord& cmp = cmpRecord.get();

    auto* dialog = new QDialog(this);
    dialog->setWindowTitle(QString("Comparing: %1 vs %2").arg(mRecord->editorId, selected));
    dialog->setMinimumSize(500, 350);

    auto* layout = new QVBoxLayout(dialog);
    auto* table = new QTableWidget(7, 3, dialog);
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
        bool showAsDouble;
    };

    QVector<Row> rows = {
        {"Damage",     mRecord->damage,     cmp.damage,     true,  true},
        {"Speed",      mRecord->speed,      cmp.speed,      true,  true},
        {"Reach",      mRecord->reach,      cmp.reach,      true,  true},
        {"Weight",     mRecord->weight,     cmp.weight,     false, true},
        {"Value",      (double)mRecord->value, (double)cmp.value, true, false},
    };

    for (int i = 0; i < rows.size(); ++i) {
        const auto& row = rows[i];

        auto* statItem = new QTableWidgetItem(row.stat);
        statItem->setFlags(statItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(i, 0, statItem);

        auto* curItem = new QTableWidgetItem(row.showAsDouble
            ? QString::number(row.curVal, 'f', 2) : QString::number((int)row.curVal));
        curItem->setFlags(curItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(i, 1, curItem);

        auto* cmpItem = new QTableWidgetItem(row.showAsDouble
            ? QString::number(row.cmpVal, 'f', 2) : QString::number((int)row.cmpVal));
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

    // Enchantment
    {
        QString curEnch = enchantmentName(mData, mRecord->enchantment);
        QString cmpEnch = enchantmentName(mData, cmp.enchantment);

        auto* statItem = new QTableWidgetItem("Enchantment");
        statItem->setFlags(statItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(5, 0, statItem);

        auto* curItem = new QTableWidgetItem(curEnch);
        curItem->setFlags(curItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(5, 1, curItem);

        auto* cmpItem = new QTableWidgetItem(cmpEnch);
        cmpItem->setFlags(cmpItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(5, 2, cmpItem);
    }

    // Type
    {
        QString curType = weaponTypeName(mRecord->weaponType);
        QString cmpType = weaponTypeName(cmp.weaponType);

        auto* statItem = new QTableWidgetItem("Type");
        statItem->setFlags(statItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(6, 0, statItem);

        auto* curItem = new QTableWidgetItem(curType);
        curItem->setFlags(curItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(6, 1, curItem);

        auto* cmpItem = new QTableWidgetItem(cmpType);
        cmpItem->setFlags(cmpItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(6, 2, cmpItem);
    }

    table->resizeColumnsToContents();
    layout->addWidget(table);

    auto* closeBtn = new QPushButton("Close");
    layout->addWidget(closeBtn);
    QObject::connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);

    dialog->exec();
    dialog->deleteLater();
}
