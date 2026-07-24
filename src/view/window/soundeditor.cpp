#include "soundeditor.hpp"
#include "waveformwidget.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QSplitter>
#include <QInputDialog>
#include <QDir>
#include <QFileInfo>

#include "../../libs/files/ba2/ba2archive.hpp"
#include "logger.hpp"
#include <QSettings>

SoundEditor::SoundEditor(QWidget* parent)
    : QDialog(parent)
    , mArchive(nullptr)
    , mSelectedIndex(-1)
{
    setWindowTitle(tr("Sound Editor"));
    setMinimumSize(780, 620);
    setupUi();
}

void SoundEditor::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    auto* splitter = new QSplitter(Qt::Vertical, this);

    // === Top: archive browser + file list ===
    auto* topWidget = new QWidget();
    auto* topLayout = new QVBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);

    auto* archiveGroup = new QGroupBox(tr("BA2 Archive Browser"));
    auto* aLayout = new QHBoxLayout(archiveGroup);
    auto* browseBtn = new QPushButton(tr("Open BA2 Archive..."));
    aLayout->addWidget(browseBtn);
    topLayout->addWidget(archiveGroup);
    connect(browseBtn, &QPushButton::clicked, this, &SoundEditor::browseArchive);

    auto* fileListGroup = new QGroupBox(tr("Sounds"));
    auto* fLayout = new QVBoxLayout(fileListGroup);
    mFileList = new QListWidget();
    mFileList->setSelectionMode(QAbstractItemView::SingleSelection);
    fLayout->addWidget(mFileList);

    auto* listBtnLayout = new QHBoxLayout();
    auto* extractBtn = new QPushButton(tr("Extract Selected"));
    auto* extractAllBtn = new QPushButton(tr("Extract All"));
    listBtnLayout->addWidget(extractBtn);
    listBtnLayout->addWidget(extractAllBtn);
    fLayout->addLayout(listBtnLayout);
    topLayout->addWidget(fileListGroup);

    connect(mFileList, &QListWidget::currentRowChanged, this, &SoundEditor::onFileSelected);
    connect(extractBtn, &QPushButton::clicked, this, &SoundEditor::extractSelected);
    connect(extractAllBtn, &QPushButton::clicked, this, &SoundEditor::extractAll);

    splitter->addWidget(topWidget);

    // === Bottom: waveform + toolbar + properties ===
    auto* bottomWidget = new QWidget();
    auto* bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(0, 0, 0, 0);

    // Waveform
    auto* waveformGroup = new QGroupBox(tr("Waveform Display"));
    auto* wfLayout = new QVBoxLayout(waveformGroup);

    mWaveform = new WaveformWidget();
    mWaveform->setMinimumHeight(150);
    wfLayout->addWidget(mWaveform);

    // Playback + editing toolbar
    auto* toolLayout = new QHBoxLayout();
    mPlayBtn = new QPushButton(tr("Play"));
    mPauseBtn = new QPushButton(tr("Pause"));
    mStopBtn = new QPushButton(tr("Stop"));
    toolLayout->addWidget(mPlayBtn);
    toolLayout->addWidget(mPauseBtn);
    toolLayout->addWidget(mStopBtn);
    toolLayout->addSpacing(16);
    mTrimBtn = new QPushButton(tr("Trim"));
    mVolumeBtn = new QPushButton(tr("Volume"));
    mFadeInBtn = new QPushButton(tr("Fade In"));
    mFadeOutBtn = new QPushButton(tr("Fade Out"));
    toolLayout->addWidget(mTrimBtn);
    toolLayout->addWidget(mVolumeBtn);
    toolLayout->addWidget(mFadeInBtn);
    toolLayout->addWidget(mFadeOutBtn);
    toolLayout->addSpacing(16);
    mSaveAudioBtn = new QPushButton(tr("Save Audio"));
    toolLayout->addWidget(mSaveAudioBtn);
    toolLayout->addStretch();
    mSelectionLabel = new QLabel(tr("No selection"));
    toolLayout->addWidget(mSelectionLabel);
    wfLayout->addLayout(toolLayout);

    bottomLayout->addWidget(waveformGroup);

    // Properties editor (TDS-012)
    auto* propsGroup = new QGroupBox(tr("Sound Properties"));
    auto* propLayout = new QFormLayout(propsGroup);

    mNameEdit = new QLineEdit();
    mNameEdit->setPlaceholderText(tr("(select a sound)"));

    mMinDistSpin = new QDoubleSpinBox();
    mMinDistSpin->setRange(0.0, 10000.0);
    mMinDistSpin->setDecimals(1);
    mMinDistSpin->setValue(0.0);

    mMaxDistSpin = new QDoubleSpinBox();
    mMaxDistSpin->setRange(0.0, 10000.0);
    mMaxDistSpin->setDecimals(1);
    mMaxDistSpin->setValue(100.0);

    mVolumeSpin = new QDoubleSpinBox();
    mVolumeSpin->setRange(0.0, 2.0);
    mVolumeSpin->setDecimals(2);
    mVolumeSpin->setValue(1.0);

    mPrioritySpin = new QSpinBox();
    mPrioritySpin->setRange(0, 255);
    mPrioritySpin->setValue(0);

    mTypeCombo = new QComboBox();
    mTypeCombo->addItems({tr("SFX (Standard)"), tr("Music"), tr("Voice"), tr("Ambience")});
    mTypeCombo->setCurrentIndex(0);

    propLayout->addRow(tr("Name"), mNameEdit);
    propLayout->addRow(tr("Min Distance"), mMinDistSpin);
    propLayout->addRow(tr("Max Distance"), mMaxDistSpin);
    propLayout->addRow(tr("Volume"), mVolumeSpin);
    propLayout->addRow(tr("Priority"), mPrioritySpin);
    propLayout->addRow(tr("Type"), mTypeCombo);

    mSavePropsBtn = new QPushButton(tr("Save Properties"));
    propLayout->addRow("", mSavePropsBtn);

    bottomLayout->addWidget(propsGroup);

    splitter->addWidget(bottomWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    mainLayout->addWidget(splitter);

    // Connect signals
    connect(mPlayBtn, &QPushButton::clicked, this, &SoundEditor::onPlay);
    connect(mPauseBtn, &QPushButton::clicked, this, &SoundEditor::onPause);
    connect(mStopBtn, &QPushButton::clicked, this, &SoundEditor::onStop);
    connect(mTrimBtn, &QPushButton::clicked, this, &SoundEditor::onTrimSelection);
    connect(mVolumeBtn, &QPushButton::clicked, this, &SoundEditor::onSetVolume);
    connect(mFadeInBtn, &QPushButton::clicked, this, &SoundEditor::onFadeIn);
    connect(mFadeOutBtn, &QPushButton::clicked, this, &SoundEditor::onFadeOut);
    connect(mSaveAudioBtn, &QPushButton::clicked, this, &SoundEditor::onSaveAudio);
    connect(mSavePropsBtn, &QPushButton::clicked, this, &SoundEditor::onSaveProperties);
    connect(mWaveform, &WaveformWidget::selectionChanged, this, &SoundEditor::onSelectionChanged);
}

void SoundEditor::browseArchive()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open BA2 Archive"),
        "", tr("BA2 Archives (*.ba2)"));
    if (!path.isEmpty()) {
        loadArchive(path);
    }
}

void SoundEditor::loadArchive(const QString& path)
{
    delete mArchive;
    mArchive = nullptr;
    mFileList->clear();
    mSelectedIndex = -1;

    mArchive = new Ba2Archive();
    if (mArchive->open(path)) {
        setWindowTitle(tr("Sound Editor - %1").arg(mArchive->name()));

        for (const auto& entry : mArchive->entries()) {
            QString lower = entry.relativePath.toLower();
            if (lower.endsWith(".wav") || lower.endsWith(".ogg")) {
                mFileList->addItem(entry.relativePath);
            }
        }

        LOG_INFO(QString("Loaded %1 sounds from BA2 archive: %2")
                     .arg(mFileList->count()).arg(mArchive->name()));
    } else {
        delete mArchive;
        mArchive = nullptr;
        QMessageBox::critical(this, tr("Error"),
                              tr("Failed to open BA2 archive:\n%1").arg(path));
    }
}

void SoundEditor::onFileSelected(int index)
{
    if (index < 0 || !mArchive) return;
    mSelectedIndex = index;

    const auto& entry = mArchive->entries()[index];
    mNameEdit->setText(entry.relativePath);

    LOG_INFO(QString("Selected sound: %1").arg(entry.relativePath));
    loadSoundIntoWaveform(index);
}

void SoundEditor::loadSoundIntoWaveform(int index)
{
    if (!mArchive || index < 0 || index >= mArchive->fileCount()) return;

    const auto& entry = mArchive->entries()[index];
    QString lower = entry.relativePath.toLower();
    if (!lower.endsWith(".wav")) {
        mWaveform->clear();
        mSelectionLabel->setText(tr("Only WAV files supported for waveform display"));
        return;
    }

    QString tmpPath = QDir::tempPath() + "/openck_sound_preview.wav";
    if (mArchive->extract(static_cast<quint32>(index), tmpPath)) {
        if (!mWaveform->loadAudio(tmpPath)) {
            mWaveform->clear();
            mSelectionLabel->setText(tr("Failed to decode audio"));
        } else {
            mSelectionLabel->setText(tr("Duration: %1s | %2 samples @ %3 Hz")
                .arg(mWaveform->durationSeconds(), 0, 'f', 2)
                .arg(mWaveform->totalSamples())
                .arg(mWaveform->sampleRate()));
        }
        QFile::remove(tmpPath);
    } else {
        mWaveform->clear();
        mSelectionLabel->setText(tr("Failed to extract for preview"));
    }
}

void SoundEditor::extractSelected()
{
    if (mSelectedIndex < 0 || !mArchive) return;

    const auto& entry = mArchive->entries()[mSelectedIndex];
    QString savePath = QFileDialog::getSaveFileName(this, tr("Extract Sound"),
        entry.relativePath.section('/', -1),
        tr("Audio Files (*.wav *.ogg);;All Files (*.*)"));
    if (savePath.isEmpty()) return;

    if (mArchive->extract(static_cast<quint32>(mSelectedIndex), savePath)) {
        QMessageBox::information(this, tr("Extracted"),
                                 tr("Sound extracted to:\n%1").arg(savePath));
    } else {
        QMessageBox::critical(this, tr("Extraction Failed"),
                              tr("Failed to extract: %1").arg(entry.relativePath));
    }
}

void SoundEditor::extractAll()
{
    if (!mArchive || mFileList->count() == 0) return;

    QString dir = QFileDialog::getExistingDirectory(this, tr("Extract All Sounds"));
    if (dir.isEmpty()) return;

    int extracted = 0;
    int failed = 0;

    for (int i = 0; i < mArchive->fileCount(); ++i) {
        const auto& entry = mArchive->entries()[i];
        QString savePath = dir + "/" + entry.relativePath;

        if (mArchive->extract(static_cast<quint32>(i), savePath)) {
            extracted++;
        } else {
            failed++;
        }
    }

    QMessageBox::information(this, tr("Extraction Complete"),
                             tr("Extracted: %1\nFailed: %2").arg(extracted).arg(failed));
    LOG_INFO(QString("Extract all: %1 succeeded, %2 failed").arg(extracted).arg(failed));
}

void SoundEditor::onSelectionChanged(int startSample, int endSample)
{
    double startSec = static_cast<double>(startSample) / mWaveform->sampleRate();
    double endSec = static_cast<double>(endSample) / mWaveform->sampleRate();
    double durSec = endSec - startSec;
    mSelectionLabel->setText(tr("Selection: %1 - %2 (%3s)")
        .arg(startSec, 0, 'f', 3)
        .arg(endSec, 0, 'f', 3)
        .arg(durSec, 0, 'f', 3));
}

void SoundEditor::onPlay()
{
    mWaveform->play();
}

void SoundEditor::onPause()
{
    mWaveform->pause();
}

void SoundEditor::onStop()
{
    mWaveform->stop();
}

void SoundEditor::onTrimSelection()
{
    if (!mWaveform->hasSelection()) {
        QMessageBox::information(this, tr("Trim"), tr("Please select a region in the waveform first."));
        return;
    }
    mWaveform->trim(mWaveform->selectionStart(), mWaveform->selectionEnd());
    mSelectionLabel->setText(tr("Trimmed to selection"));
}

void SoundEditor::onSetVolume()
{
    bool ok = false;
    double vol = QInputDialog::getDouble(this, tr("Set Volume"),
        tr("Volume factor (0.0 - 2.0):"), 1.0, 0.0, 2.0, 2, &ok);
    if (ok) {
        mWaveform->setVolume(static_cast<float>(vol));
    }
}

void SoundEditor::onFadeIn()
{
    bool ok = false;
    int samples = QInputDialog::getInt(this, tr("Fade In"),
        tr("Number of samples:"), mWaveform->totalSamples() / 4, 1,
        mWaveform->totalSamples(), 1, &ok);
    if (ok) {
        mWaveform->fadeIn(samples);
    }
}

void SoundEditor::onFadeOut()
{
    bool ok = false;
    int samples = QInputDialog::getInt(this, tr("Fade Out"),
        tr("Number of samples:"), mWaveform->totalSamples() / 4, 1,
        mWaveform->totalSamples(), 1, &ok);
    if (ok) {
        mWaveform->fadeOut(samples);
    }
}

void SoundEditor::onSaveAudio()
{
    if (mWaveform->totalSamples() == 0) {
        QMessageBox::information(this, tr("Save Audio"), tr("No audio data to save."));
        return;
    }
    QString path = QFileDialog::getSaveFileName(this, tr("Save Audio"),
        "", tr("WAV Files (*.wav)"));
    if (path.isEmpty()) return;

    if (mWaveform->saveAudio(path)) {
        QMessageBox::information(this, tr("Saved"), tr("Audio saved to:\n%1").arg(path));
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save audio."));
    }
}

void SoundEditor::onSaveProperties()
{
    if (mSelectedIndex < 0 || !mArchive)
    {
        QMessageBox::warning(this, tr("Error"), tr("No sound selected."));
        return;
    }

    QString entryName = mFileList->item(mSelectedIndex)->text();

    QSettings settings("OpenCK", "OpenCK");
    settings.beginGroup("SoundProperties");
    settings.beginGroup(entryName);
    settings.setValue("name", mNameEdit->text());
    settings.setValue("minDistance", mMinDistSpin->value());
    settings.setValue("maxDistance", mMaxDistSpin->value());
    settings.setValue("volume", mVolumeSpin->value());
    settings.setValue("priority", mPrioritySpin->value());
    settings.setValue("type", mTypeCombo->currentText());
    settings.endGroup();
    settings.endGroup();

    QMessageBox::information(this, tr("Properties Saved"),
        tr("Sound properties saved for:\n%1").arg(entryName));
    LOG_INFO(QString("Saved properties for: %1").arg(entryName));
}
