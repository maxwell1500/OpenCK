#ifndef SPELL_EDITOR_HPP
#define SPELL_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QListWidget>
#include <QDoubleSpinBox>

class QLabel;
class QPushButton;
class Data;
struct SpellRecord;
class NifViewportWidget;

class SpellEditor : public QDialog
{
    Q_OBJECT

public:
    SpellEditor(Data* data, SpellRecord* spell, QWidget* parent = nullptr);

    SpellRecord* getRecord() const { return mRecord; }

private slots:
    void saveRecord();
    void addEffect();
    void removeEffect();
    void onEffectSelectionChanged();
    void previewEffect();

private:
    bool validate();
    void setupUI();
    void loadFromSpell();
    void updateEffectPreview();
    void showEffectPreview(const QString& artPath);

    Data* mData;
    SpellRecord* mRecord;
    QString mOriginalEditorId;

    QLineEdit* mEditorIdEdit;
    QLineEdit* mFullNameEdit;
    QSpinBox* mCostSpin;
    QSpinBox* mCastingSoundSpin;
    QListWidget* mEffectsList;
    QComboBox* mEffectCombo;
    QDoubleSpinBox* mMagnitudeSpin;
    QSpinBox* mDurationSpin;
    QSpinBox* mAreaSpin;

    QLabel* mPreviewNameLabel;
    QLabel* mPreviewSchoolLabel;
    QLabel* mPreviewStatsLabel;
    QPushButton* mPreviewBtn;
    NifViewportWidget* mPreviewViewport;
};

#endif // SPELL_EDITOR_HPP
