#ifndef ENCH_EDITOR_HPP
#define ENCH_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QTabWidget>
#include <QComboBox>
#include <QDoubleSpinBox>

class QLabel;
class QPushButton;
class Data;
struct EnchRecord;
class NifViewportWidget;

class EnchEditor : public QDialog
{
    Q_OBJECT

public:
    EnchEditor(Data* data, EnchRecord* ench, QWidget* parent = nullptr);

    EnchRecord* getRecord() const { return mRecord; }

private slots:
    void saveRecord();
    void previewEnchantment();

private:
    bool validate();
    void setupUI();
    void loadFromEnch();
    void showEffectPreview(const QString& artPath);

    Data* mData;
    EnchRecord* mRecord;
    QString mOriginalEditorId;

    QLineEdit* mEditorIdEdit;
    QLineEdit* mNameEdit;
    QSpinBox* mCostLimitSpin;
    QSpinBox* mChargesSpin;
    QSpinBox* mEnchantmentDataSpin;
    QDoubleSpinBox* mChargeSpin;
    QSpinBox* mDurationSpin;
    QDoubleSpinBox* mMagnitudeSpin;
    QComboBox* mTypeCombo;
    QComboBox* mSoulGemCombo;

    QLabel* mPreviewTypeLabel;
    QLabel* mPreviewInfoLabel;
    QPushButton* mPreviewBtn;
    NifViewportWidget* mPreviewViewport;
};

#endif // ENCH_EDITOR_HPP
