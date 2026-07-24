#ifndef MAGIC_EDITOR_HPP
#define MAGIC_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>

class Data;
struct MagicRecord;

class MagicEditor : public QDialog
{
    Q_OBJECT

public:
    MagicEditor(Data* data, MagicRecord* magic, QWidget* parent = nullptr);

private slots:
    void saveRecord();
    void browseIconPath();
    void browseHitEffectArt();
    void updateIconPreview();

private:
    bool validate();
    void setupUI();
    void loadFromMagic();
    void saveToMagic();

    Data* mData;
    MagicRecord* mMagic;

    QLineEdit* mEditorIdEdit;
    QSpinBox* mSchoolsSpin;
    QSpinBox* mDamageTypeSpin;
    QSpinBox* mCastingSoundSpin;
    QLineEdit* mIconPathEdit;
    QLineEdit* mModelPathEdit;
    QSpinBox* mEffectsSpin;

    // Visual Assets
    QLineEdit* mHitEffectArtPathEdit;
    QPushButton* mIconBrowseBtn;
    QPushButton* mHitEffectBrowseBtn;
    QLabel* mIconPreview;

    // Dual Casting
    QComboBox* mSecondaryEffectCombo;
    QDoubleSpinBox* mDualCastCostMultiplier;
    QDoubleSpinBox* mDualCastMagnitudeMultiplier;
    QDoubleSpinBox* mDualCastDurationMultiplier;
};

#endif // MAGIC_EDITOR_HPP
