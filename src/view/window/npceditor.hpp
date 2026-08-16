#ifndef NPCEDITOR_H
#define NPCEDITOR_H

#include <QDialog>
#include <QTabWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QListWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QGroupBox>

struct NpcRecord;
class Data;

class NpcEditor : public QDialog
{
    Q_OBJECT

public:
    NpcEditor(Data* data, NpcRecord* npc, QWidget* parent = nullptr);
    ~NpcEditor();

    NpcRecord* getRecord() const { return mRecord; }

private slots:
    void saveChanges();
    void cancelEdit();

private:
    void setupUI();
    void loadFromNpc();
    void saveToNpc();
    bool validateNpc();

    Data* mData;
    NpcRecord* mRecord;
    QString mOriginalEditorId;
    QTabWidget* mTabWidget;

    QLineEdit* mEditorIdEdit;
    QLineEdit* mFullNameEdit;
    QSpinBox* mLevelSpin;
    QComboBox* mRaceCombo;
    QComboBox* mSexCombo;
    QComboBox* mClassCombo;
    QComboBox* mFactionCombo;

    QSpinBox* mHealthSpin;
    QSpinBox* mMagickaSpin;
    QSpinBox* mStaminaSpin;

    QSpinBox* mAggressionSpin;
    QSpinBox* mConfidenceSpin;
    QSpinBox* mMoralitySpin;
    QSpinBox* mEnergySpin;
    QSpinBox* mMoodSpin;
    QSpinBox* mDispositionSpin;

    QListWidget* mSpellsList;
    QTableWidget* mInventoryTable;
    QListWidget* mRelationshipsList;

    QLineEdit* mHeadPartHead;
    QLineEdit* mHeadPartEyes;
    QLineEdit* mHeadPartBrows;
    QLineEdit* mHeadPartNose;
    QLineEdit* mHeadPartMouth;
    QLineEdit* mHeadPartHair;
    QLineEdit* mHeadPartBeard;
    QLineEdit* mHeadPartScar;
    QComboBox* mSkinToneCombo;
    QSlider* mWeightSlider;
    QSpinBox* mWeightSpin;
    QSlider* mHeightSlider;
    QSpinBox* mHeightSpin;
    QComboBox* mVoiceTypeCombo;

    QCheckBox* mIsLeveledCheck;
    QGroupBox* mLeveledGroup;
    QSpinBox* mMinLevelSpin;
    QSpinBox* mMaxLevelSpin;
    QLineEdit* mLeveledTemplateEdit;
    QDoubleSpinBox* mLevelMultiplierSpin;

    void addSpellRow(const QString& spellId);
    void addInventoryRow(const QString& formId, const QString& count, const QString& slot, bool equipped);
    void addRelationshipRow(const QString& factionId);
};

#endif // NPCEDITOR_H
