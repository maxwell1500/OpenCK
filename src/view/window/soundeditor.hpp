#pragma once

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

class Ba2Archive;
class WaveformWidget;
class QGroupBox;

class SoundEditor : public QDialog
{
    Q_OBJECT

public:
    explicit SoundEditor(QWidget* parent = nullptr);

private slots:
    void browseArchive();
    void loadArchive(const QString& path);
    void onFileSelected(int index);
    void extractSelected();
    void extractAll();
    void onPlay();
    void onPause();
    void onStop();
    void onTrimSelection();
    void onSetVolume();
    void onFadeIn();
    void onFadeOut();
    void onSaveAudio();
    void onSaveProperties();
    void onSelectionChanged(int startSample, int endSample);

private:
    void setupUi();
    void loadSoundIntoWaveform(int index);

    Ba2Archive* mArchive;
    int mSelectedIndex;

    QListWidget* mFileList;
    WaveformWidget* mWaveform;
    QLabel* mSelectionLabel;

    QPushButton* mPlayBtn;
    QPushButton* mPauseBtn;
    QPushButton* mStopBtn;
    QPushButton* mTrimBtn;
    QPushButton* mVolumeBtn;
    QPushButton* mFadeInBtn;
    QPushButton* mFadeOutBtn;
    QPushButton* mSaveAudioBtn;

    QLineEdit* mNameEdit;
    QDoubleSpinBox* mMinDistSpin;
    QDoubleSpinBox* mMaxDistSpin;
    QDoubleSpinBox* mVolumeSpin;
    QSpinBox* mPrioritySpin;
    QComboBox* mTypeCombo;
    QPushButton* mSavePropsBtn;
};
