#ifndef SPELLWIZARD_HPP
#define SPELLWIZARD_HPP

#include <QWizard>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QListWidget>
#include <QTableWidget>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>

struct SpellRecord;
class Data;

class SpellWizard : public QWizard
{
    Q_OBJECT

public:
    explicit SpellWizard(Data* data, QWidget* parent = nullptr);

    SpellRecord result() const;

private:
    Data* mData;

    // Page 1: Basic Info
    QWizardPage* mBasicPage;
    QLineEdit* mEditorIdEdit;
    QLineEdit* mSpellNameEdit;
    QComboBox* mSpellTypeCombo;
    QComboBox* mSchoolCombo;
    QSpinBox* mCostSpin;

    // Page 2: Magic Effects
    QWizardPage* mEffectsPage;
    QListWidget* mEffectList;
    QComboBox* mEffectCombo;
    QDoubleSpinBox* mMagnitudeSpin;
    QSpinBox* mDurationSpin;
    QSpinBox* mAreaSpin;

    // Page 3: Visuals/Audio
    QWizardPage* mVisualsPage;
    QLineEdit* mCastingArtEdit;
    QLineEdit* mHitEffectEdit;
    QLineEdit* mProjectileEdit;
    QSpinBox* mCastingSoundSpin;

    QPushButton* mPreviewCastingArtBtn;
    QPushButton* mPreviewHitEffectBtn;
    QPushButton* mPreviewCastingSoundBtn;

    // Summary Page
    QWizardPage* mSummaryPage;
    QLabel* mSummaryLabel;

    QVector<quint32> mSelectedEffects;
    QVector<float> mEffectMagnitudes;
    QVector<int> mEffectDurations;
    QVector<int> mEffectAreas;

    void setupBasicPage();
    void setupEffectsPage();
    void setupVisualsPage();
    void setupSummaryPage();
    void populateEffectList();
    void onAddEffect();
    void onRemoveEffect();
    void updateSummary();

private slots:
    void onPreviewCastingArt();
    void onPreviewHitEffect();
    void onPreviewCastingSound();
};

#endif // SPELLWIZARD_HPP
