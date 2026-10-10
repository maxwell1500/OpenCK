#include "pack_editor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/collection.hpp"
#include "packagerecord.hpp"
#include "../../libs/files/esm/packagesemantics.hpp"
#include "../widgets/formpickerwidget.hpp"
#include "fieldvalidators.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QComboBox>
#include <QCheckBox>
#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QDialog>

namespace
{

constexpr int kRoleFormId = Qt::UserRole + 1;
constexpr int kRoleTypeName = Qt::UserRole + 2;

// "Any" style placeholder shown for unconstrained schedule fields.
const char* const kAnyLabel = "(any)";

void fillScheduleCombo(QComboBox* combo, int minValue, int maxValue, int anyValue)
{
    if (!combo) return;
    combo->clear();
    combo->addItem(QObject::tr(kAnyLabel));
    for (int v = minValue; v <= maxValue; ++v)
    {
        combo->addItem(QString::number(v), v);
    }
    combo->setCurrentIndex(0);
    (void)anyValue;
}

} // namespace

PackEditor::PackEditor(Data* data, PackageRecord* pack, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mPack(pack)
{
    setupUI();
    loadFromPack();
}

void PackEditor::setupUI()
{
    if (!mPack) return;

    setWindowTitle("Package Editor");
    setMinimumSize(520, 460);

    auto* mainLayout = new QVBoxLayout(this);

    // --- Identity and family -------------------------------------------------
    auto* infoGroup = new QGroupBox("Package");
    auto* infoLayout = new QFormLayout(infoGroup);

    infoLayout->addRow("Editor ID:",
                       new QLabel(mPack->editorId.isEmpty()
                                      ? QString("Package_%1").arg(mPack->formId, 8, 16, QChar('0')).toUpper()
                                      : mPack->editorId));

    mKindCombo = new QComboBox();
    for (quint32 kind = 1; kind <= 30; ++kind)
    {
        mKindCombo->addItem(openck::packageKindName(openck::packageKindFromU32(kind)),
                            kind);
    }
    infoLayout->addRow("Kind:", mKindCombo);
    connect(mKindCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PackEditor::onKindChanged);

    mainLayout->addWidget(infoGroup);

    // --- Schedule -------------------------------------------------------------
    mScheduleGroup = new QGroupBox("Schedule");
    auto* scheduleLayout = new QGridLayout(mScheduleGroup);

    mMonthCombo = new QComboBox();
    mWeekdayCombo = new QComboBox();
    mDateCombo = new QComboBox();
    mHourCombo = new QComboBox();
    mMinuteCombo = new QComboBox();

    fillScheduleCombo(mMonthCombo, 1, 12, 0);
    fillScheduleCombo(mWeekdayCombo, 0, 6, 0);
    fillScheduleCombo(mDateCombo, 1, 31, 0);
    fillScheduleCombo(mHourCombo, 0, 23, 0);
    fillScheduleCombo(mMinuteCombo, 0, 119, 0);

    scheduleLayout->addWidget(new QLabel("Month:"), 0, 0);
    scheduleLayout->addWidget(mMonthCombo, 0, 1);
    scheduleLayout->addWidget(new QLabel("Weekday:"), 0, 2);
    scheduleLayout->addWidget(mWeekdayCombo, 0, 3);
    scheduleLayout->addWidget(new QLabel("Day of month:"), 1, 0);
    scheduleLayout->addWidget(mDateCombo, 1, 1);
    scheduleLayout->addWidget(new QLabel("Hour:"), 1, 2);
    scheduleLayout->addWidget(mHourCombo, 1, 3);
    scheduleLayout->addWidget(new QLabel("Minute:"), 2, 0);
    scheduleLayout->addWidget(mMinuteCombo, 2, 1);

    mDoAllCheck = new QCheckBox("Perform all procedures once");
    scheduleLayout->addWidget(mDoAllCheck, 2, 2, 1, 2);

    mainLayout->addWidget(mScheduleGroup);

    // --- Targets --------------------------------------------------------------
    auto* targetGroup = new QGroupBox("Target references");
    auto* targetLayout = new QVBoxLayout(targetGroup);

    mTargets = new QListWidget();
    mTargets->setSelectionMode(QAbstractItemView::SingleSelection);
    targetLayout->addWidget(mTargets, 1);

    auto* targetButtons = new QHBoxLayout();
    auto* addTarget = new QPushButton("Add target...");
    auto* removeTarget = new QPushButton("Remove selected");
    targetButtons->addWidget(addTarget);
    targetButtons->addWidget(removeTarget);
    targetButtons->addStretch();
    targetLayout->addLayout(targetButtons);
    connect(addTarget, &QPushButton::clicked, this, [this]() {
        const QVector<FormPickerEntry> entries = loadFormEntries();
        QDialog pickerDialog(this);
        pickerDialog.setWindowTitle("Select target reference");
        auto* layout = new QVBoxLayout(&pickerDialog);
        auto* picker = new FormPickerWidget(entries, 0, &pickerDialog);
        picker->setMinimumSize(420, 320);
        layout->addWidget(picker);
        auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                         &pickerDialog);
        layout->addWidget(box);
        connect(box, &QDialogButtonBox::accepted, &pickerDialog, &QDialog::accept);
        connect(box, &QDialogButtonBox::rejected, &pickerDialog, &QDialog::reject);
        if (pickerDialog.exec() == QDialog::Accepted && picker->value() != 0)
        {
            const quint32 formId = picker->value();
            for (const auto& entry : entries)
            {
                if (entry.formId == formId)
                {
                    auto* item = new QListWidgetItem(
                        QString("%1 (%2)").arg(entry.editorId, entry.typeName));
                    item->setData(kRoleFormId, formId);
                    item->setData(kRoleTypeName, entry.typeName);
                    mTargets->addItem(item);
                    refreshIssues();
                    break;
                }
            }
        }
    });
    connect(removeTarget, &QPushButton::clicked, this, [this]() {
        for (auto* item : mTargets->selectedItems())
        {
            delete item;
        }
        refreshIssues();
    });

    mainLayout->addWidget(targetGroup, 1);

    // --- Validation -----------------------------------------------------------
    auto* issueGroup = new QGroupBox("Validation");
    auto* issueLayout = new QVBoxLayout(issueGroup);
    mIssues = new QListWidget();
    issueLayout->addWidget(mIssues);
    mainLayout->addWidget(issueGroup);

    // --- Buttons --------------------------------------------------------------
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);
    connect(saveBtn, &QPushButton::clicked, this, &PackEditor::saveRecord);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    mSaveButton = saveBtn;
    onKindChanged(mKindCombo->currentIndex());
}

QVector<FormPickerEntry> PackEditor::loadFormEntries() const
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
    entries.shrink_to_fit();
    return entries;
}

void PackEditor::onKindChanged(int index)
{
    Q_UNUSED(index);
    // Road kinds need flag bits the editor does not model yet; the schedule
    // and target sections apply to every family.
    mScheduleGroup->setEnabled(true);
    refreshIssues();
}

void PackEditor::loadFromPack()
{
    bool ok = false;
    const openck::PackageData data = openck::decodePackageData(*mPack, &ok);

    // The kind combo is ordered by on-disk value; find the record's entry.
    if (mKindCombo)
    {
        int index = mKindCombo->findData(ok ? data.type : 0);
        if (index < 0) index = 0;
        mKindCombo->setCurrentIndex(index);
    }

    if (ok)
    {
        if (mMonthCombo && data.schedule.month != 0xFF)
            mMonthCombo->setCurrentIndex(
                mMonthCombo->findData(data.schedule.month));
        if (mWeekdayCombo && data.schedule.weekday != 0xFF)
            mWeekdayCombo->setCurrentIndex(
                mWeekdayCombo->findData(data.schedule.weekday));
        if (mDateCombo && data.schedule.date != 0xFF)
            mDateCombo->setCurrentIndex(mDateCombo->findData(data.schedule.date));
        if (mHourCombo && data.schedule.hour != 0xFF)
            mHourCombo->setCurrentIndex(mHourCombo->findData(data.schedule.hour));
        if (mMinuteCombo && data.schedule.minute >= 0)
            mMinuteCombo->setCurrentIndex(mMinuteCombo->findData(data.schedule.minute));
        if (mDoAllCheck) mDoAllCheck->setChecked(data.doAll);
    }

    // Targets come from the record, not the decoded union.
    if (mTargets)
    {
        const QVector<FormPickerEntry> entries = loadFormEntries();
        for (const quint32 targetId : mPack->targetIds)
        {
            QString label = QString("0x%1").arg(targetId, 8, 16, QChar('0')).toUpper();
            for (const auto& entry : entries)
            {
                if (entry.formId == targetId)
                {
                    label = QString("%1 (%2)").arg(entry.editorId, entry.typeName);
                    break;
                }
            }
            QListWidgetItem* item = new QListWidgetItem(label);
            item->setData(kRoleFormId, targetId);
            mTargets->addItem(item);
        }
    }

    refreshIssues();
}

void PackEditor::refreshIssues()
{
    if (!mIssues) return;
    mIssues->clear();

    // Read the current UI back into a decoded view so validation sees the
    // pending edit, not the saved record.
    openck::PackageData data = openck::decodePackageData(*mPack);
    data.type = mKindCombo ? mKindCombo->currentData().toUInt() : data.type;
    if (mMonthCombo) data.schedule.month =
        mMonthCombo->currentData().isValid() ? quint8(mMonthCombo->currentData().toUInt()) : quint8(0xFF);
    if (mWeekdayCombo) data.schedule.weekday =
        mWeekdayCombo->currentData().isValid() ? quint8(mWeekdayCombo->currentData().toUInt()) : quint8(0xFF);
    if (mDateCombo) data.schedule.date =
        mDateCombo->currentData().isValid() ? quint8(mDateCombo->currentData().toUInt()) : quint8(0xFF);
    if (mHourCombo) data.schedule.hour =
        mHourCombo->currentData().isValid() ? quint8(mHourCombo->currentData().toUInt()) : quint8(0xFF);
    if (mMinuteCombo) data.schedule.minute =
        mMinuteCombo->currentData().isValid() ? mMinuteCombo->currentData().toInt() : -1;
    if (mDoAllCheck) data.doAll = mDoAllCheck->isChecked();

    PackageRecord pending = *mPack;
    pending.targetIds.clear();
    for (int i = 0; mTargets && i < mTargets->count(); ++i)
    {
        pending.targetIds.append(
            mTargets->item(i)->data(kRoleFormId).toUInt());
    }

    const QVector<openck::PackageIssue> issues =
        openck::validatePackageData(pending, data);
    bool blocking = false;
    for (const auto& issue : issues)
    {
        auto* item = new QListWidgetItem(issue.message);
        item->setForeground(issue.severity == openck::PackageIssueSeverity::Error
                                ? QColor(0xc6, 0x28, 0x28)
                                : QColor(0xf5, 0x7f, 0x17));
        mIssues->addItem(item);
        if (issue.severity == openck::PackageIssueSeverity::Error) blocking = true;
    }
    if (issues.isEmpty())
    {
        mIssues->addItem(new QListWidgetItem("No problems detected."));
    }
    mIssues->setVisible(true);
    if (mSaveButton) mSaveButton->setEnabled(!blocking);
}

void PackEditor::saveToPack()
{
    if (!mPack) return;

    bool ok = false;
    openck::PackageData data = openck::decodePackageData(*mPack, &ok);

    if (mKindCombo)
        data.type = mKindCombo->currentData().toUInt();
    // Flags carry the "once/do-all" and road/travel bits; keep whatever the
    // file had and only touch what this editor owns.
    if (mMonthCombo) data.schedule.month =
        mMonthCombo->currentData().isValid() ? quint8(mMonthCombo->currentData().toUInt()) : quint8(0xFF);
    if (mWeekdayCombo) data.schedule.weekday =
        mWeekdayCombo->currentData().isValid() ? quint8(mWeekdayCombo->currentData().toUInt()) : quint8(0xFF);
    if (mDateCombo) data.schedule.date =
        mDateCombo->currentData().isValid() ? quint8(mDateCombo->currentData().toUInt()) : quint8(0xFF);
    if (mHourCombo) data.schedule.hour =
        mHourCombo->currentData().isValid() ? quint8(mHourCombo->currentData().toUInt()) : quint8(0xFF);
    if (mMinuteCombo) data.schedule.minute =
        mMinuteCombo->currentData().isValid() ? mMinuteCombo->currentData().toInt() : -1;
    if (mDoAllCheck) data.doAll = mDoAllCheck->isChecked();

    openck::encodePackageData(*mPack, data);

    // packageType is the record-level mirror of the PKDT type word.
    mPack->packageType = data.type;

    // Targets. The record's save path rewrites a PTDT when targetIds no
    // longer matches the stored payload and replays the original bytes
    // otherwise, so editing this vector is enough and must stay the only
    // thing that changes. Fabricating PTDT payloads here would desynchronise
    // targetIds from ptdtRaws and re-emit the wrong bytes.
    mPack->targetIds.clear();
    for (int i = 0; mTargets && i < mTargets->count(); ++i)
    {
        const quint32 formId = mTargets->item(i)->data(kRoleFormId).toUInt();
        if (formId != 0)
        {
            mPack->targetIds.append(formId);
        }
    }
}

void PackEditor::saveRecord()
{
    if (!mPack) return;

    // Re-read the UI and block only on errors, not warnings.
    refreshIssues();
    bool hasError = false;
    for (int i = 0; mIssues && i < mIssues->count(); ++i)
    {
        QListWidgetItem* item = mIssues->item(i);
        if (item->foreground().color() == QColor(0xc6, 0x28, 0x28))
        {
            hasError = true;
        }
    }
    if (hasError)
    {
        QMessageBox::warning(this, tr("Validation Errors"),
                             tr("Fix the reported errors before saving."));
        return;
    }

    saveToPack();
    accept();
}
