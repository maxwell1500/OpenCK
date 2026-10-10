#ifndef PACK_EDITOR_HPP
#define PACK_EDITOR_HPP

#include <QDialog>
#include <QDateTime>

#include <QPushButton>
#include <QStringList>
#include <QVector>

#include "../../libs/files/esm/Packagerecord.hpp"

class QComboBox;
class QCheckBox;
class QListWidget;
class QWidget;
class Data;
struct FormPickerEntry;

// Dialog for editing a PACK record's semantics: the package family, the
// schedule the engine gates it on, and the target references. The dialog
// works on a decoded semantic view and writes back through
// openck::encodePackageData, so payload bytes the editor does not own are
// preserved and the record still round-trips exactly.
class PackEditor : public QDialog
{
    Q_OBJECT

public:
    PackEditor(Data* data, PackageRecord* pack, QWidget* parent = nullptr);

private slots:
    void onKindChanged(int index);
    void saveRecord();

private:
    void setupUI();
    void loadFromPack();
    void saveToPack();
    void refreshIssues();
    QVector<FormPickerEntry> loadFormEntries() const;

    Data* mData;
    PackageRecord* mPack;
    PackageRecord mBackup;

    QWidget* mScheduleGroup = nullptr;
    QListWidget* mTargets = nullptr;
    QComboBox* mKindCombo = nullptr;
    QComboBox* mMonthCombo = nullptr;
    QComboBox* mWeekdayCombo = nullptr;
    QComboBox* mDateCombo = nullptr;
    QComboBox* mHourCombo = nullptr;
    QComboBox* mMinuteCombo = nullptr;
    QCheckBox* mDoAllCheck = nullptr;
    QListWidget* mIssues = nullptr;
    QPushButton* mSaveButton = nullptr;
};

#endif // PACK_EDITOR_HPP
