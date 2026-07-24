#ifndef WEATHERLIGHTEDITOR_HPP
#define WEATHERLIGHTEDITOR_HPP

#include <QDialog>
#include <QTreeWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QTabWidget>
#include <QComboBox>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QColor>

class Data;
struct GameSetting;
struct GlobalVariable;

class WeatherLightEditor : public QDialog
{
    Q_OBJECT

public:
    WeatherLightEditor(Data* data, QWidget* parent = nullptr);
    ~WeatherLightEditor();

private slots:
    void onNodeSelected(QTreeWidgetItem* item, int column);
    void onAddSetting();
    void onEditSetting();
    void onDeleteSetting();
    void onSave();

    void onLightColorClicked();
    void onAmbientColorClicked();
    void onSpecularColorClicked();

    void onIntensitySliderChanged(int value);
    void onIntensitySpinBoxChanged(int value);
    void onIntensityDoubleChanged(double value);

    void onFlickerToggled(bool checked);

private:
    void setupUI();
    void loadSettings();
    void refreshTree();
    void showSettingDetails(const QString& name, const QString& value);

    void setupLightTab(QWidget* tab);
    void setupShadowsTab(QWidget* tab);
    void setupAmbientTab(QWidget* tab);
    void setupTransitionsTab(QWidget* tab);

    void updateLightColorButton();
    void updateAmbientColorButton();
    void updateSpecularColorButton();

    void syncIntensityFromSlider(int sliderValue);
    void syncIntensityFromDouble(double doubleValue);

    Data* mData;
    QTreeWidget* mTree;
    QTextEdit* mDetailEdit;
    QPushButton* mAddSettingButton;
    QPushButton* mEditButton;
    QPushButton* mDeleteButton;
    QPushButton* mSaveButton;
    QLabel* mStatusLabel;

    QString mSelectedName;
    QString mSelectedValue;

    QTabWidget* mTabWidget;

    // Light Properties data
    int mLightTypeIndex;
    QColor mLightColor;
    double mLightIntensity;
    double mLightAttenuation;
    bool mLightFlicker;
    double mLightFlickerSpeed;

    // Shadow Settings data
    int mShadowTypeIndex;
    double mShadowDepthBias;
    double mShadowDistance;
    int mShadowResolutionIndex;
    bool mShadowCast;
    bool mShadowReceive;

    // Ambient Light data
    QColor mAmbientColor;
    double mAmbientIntensity;
    QColor mSpecularColor;
    double mSpecularPower;
    double mFogNear;
    double mFogFar;

    // Transition data
    double mTransitionSpeed;
    int mTransitionCurveIndex;
    double mPrecipitationFadeTime;
    double mCloudSpeedMultiplier;
    double mSkyTransitionDuration;

    // Light tab widgets
    QComboBox* mLightTypeCombo;
    QPushButton* mLightColorButton;
    QSlider* mIntensitySlider;
    QSpinBox* mIntensitySpinBox;
    QDoubleSpinBox* mIntensityDoubleSpinBox;
    QDoubleSpinBox* mAttenuationSpinBox;
    QCheckBox* mFlickerCheckBox;
    QDoubleSpinBox* mFlickerSpeedSpinBox;

    // Shadow tab widgets
    QComboBox* mShadowTypeCombo;
    QDoubleSpinBox* mShadowDepthBiasSpinBox;
    QDoubleSpinBox* mShadowDistanceSpinBox;
    QComboBox* mShadowResolutionCombo;
    QCheckBox* mCastShadowsCheckBox;
    QCheckBox* mReceiveShadowsCheckBox;

    // Ambient tab widgets
    QPushButton* mAmbientColorButton;
    QDoubleSpinBox* mAmbientIntensitySpinBox;
    QPushButton* mSpecularColorButton;
    QDoubleSpinBox* mSpecularPowerSpinBox;
    QDoubleSpinBox* mFogNearSpinBox;
    QDoubleSpinBox* mFogFarSpinBox;

    // Transitions tab widgets
    QDoubleSpinBox* mTransitionSpeedSpinBox;
    QComboBox* mTransitionCurveCombo;
    QDoubleSpinBox* mPrecipitationFadeSpinBox;
    QDoubleSpinBox* mCloudSpeedMultiplierSpinBox;
    QDoubleSpinBox* mSkyTransitionDurationSpinBox;
    QLineEdit* mRainSoundEdit;
    QLineEdit* mThunderSoundEdit;
    QLineEdit* mWindSoundEdit;
    QLineEdit* mSnowSoundEdit;
};

#endif // WEATHERLIGHTEDITOR_HPP
