#include "animationeditor.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QTimer>
#include <QSplitter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <cmath>

#include "../../libs/files/nifanim/nifanimation.hpp"
#include "../../libs/files/nif/nifparser.hpp"
#include "../../model/tools/movekeyframecommand.hpp"
#include "../../model/tools/addkeyframecommand.hpp"
#include "../../model/tools/removekeyframecommand.hpp"
#include "../../model/tools/commandundoadapter.hpp"
#include "timeline.hpp"
#include "nodegraphwidget.hpp"
#include "logger.hpp"

namespace {

void quaternionToEuler(float w, float x, float y, float z,
                       float& rx, float& ry, float& rz)
{
    const float radToDeg = 180.0f / 3.14159265358979323846f;

    float sinr = 2.0f * (w * x + y * z);
    float cosr = 1.0f - 2.0f * (x * x + y * y);
    rx = std::atan2(sinr, cosr) * radToDeg;

    float sinp = 2.0f * (w * y - z * x);
    if (std::abs(sinp) >= 1.0f)
        ry = std::copysign(90.0f, sinp);
    else
        ry = std::asin(sinp) * radToDeg;

    float siny = 2.0f * (w * z + x * y);
    float cosy = 1.0f - 2.0f * (y * y + z * z);
    rz = std::atan2(siny, cosy) * radToDeg;
}

void collectNodeAnimations(const Nif::Node* node,
                           QVector<QPair<QString, QVector<Nif::NiKeyframeController>>>& out)
{
    if (!node) return;
    if (!node->animations.isEmpty()) {
        out.append({node->name, node->animations});
    }
    for (const auto* child : node->children) {
        collectNodeAnimations(child, out);
    }
}

NifAnimation* loadAnimationFromNif(const QString& path)
{
    Nif::NifParser parser;
    if (!parser.load(path))
        return nullptr;

    Nif::Node* root = parser.getRoot();
    if (!root)
        return nullptr;

    QVector<QPair<QString, QVector<Nif::NiKeyframeController>>> nodeAnims;
    collectNodeAnimations(root, nodeAnims);

    if (nodeAnims.isEmpty())
        return nullptr;

    auto* anim = new NifAnimation();
    anim->name = QFileInfo(path).baseName();

    AnimClip clip;
    clip.name = anim->name;

    for (const auto& [nodeName, controllers] : nodeAnims) {
        for (const auto& ctrl : controllers) {
            if (ctrl.keyframes.isEmpty()) continue;

            AnimChannel ch;
            ch.boneName = nodeName;
            ch.type = ctrl.clipName.isEmpty() ? "NiKeyframeData" : ctrl.clipName;

            for (const auto& tk : ctrl.keyframes) {
                AnimKeyframe kf;
                kf.time = tk.time;
                kf.tx = tk.translation.x;
                kf.ty = tk.translation.y;
                kf.tz = tk.translation.z;
                quaternionToEuler(tk.rotation.w, tk.rotation.x,
                                  tk.rotation.y, tk.rotation.z,
                                  kf.rx, kf.ry, kf.rz);
                ch.keyframes.append(kf);
            }

            if (!ch.keyframes.isEmpty()) {
                ch.duration = ch.keyframes.last().time;
                clip.channels.append(ch);
            }
        }
    }

    if (clip.channels.isEmpty()) {
        delete anim;
        return nullptr;
    }

    float maxDur = 0.0f;
    for (const auto& ch : clip.channels) {
        if (ch.duration > maxDur) maxDur = ch.duration;
    }
    clip.duration = maxDur;

    anim->clips.append(clip);
    return anim;
}

} // namespace

AnimationEditor::AnimationEditor(QWidget* parent)
    : QDialog(parent)
    , mAnimation(nullptr)
    , mSelectedClip(-1)
    , mPlaying(false)
    , mCurrentFrame(0)
    , mSelectedTime(0.0f)
    , mGraphWidget(nullptr)
{
    setWindowTitle(tr("Animation Editor"));
    setMinimumSize(900, 600);

    mUndoStack = new QUndoStack(this);

    auto* mainLayout = new QVBoxLayout(this);

    // --- NIF browser ---
    auto* nifGroup = new QGroupBox(tr("NIF Animation Source"));
    auto* nifLayout = new QHBoxLayout(nifGroup);

    auto* browseBtn = new QPushButton(tr("Open NIF with Animation..."));
    nifLayout->addWidget(browseBtn);
    mainLayout->addWidget(nifGroup);

    connect(browseBtn, &QPushButton::clicked, this, &AnimationEditor::browseNif);

    // --- Top area: clip list + timeline + property panel ---
    auto* topSplitter = new QSplitter(Qt::Horizontal);

    // Clip list (left)
    auto* clipsGroup = new QGroupBox(tr("Animation Clips"));
    auto* clipsLayout = new QVBoxLayout(clipsGroup);
    mClipList = new QListWidget();
    clipsLayout->addWidget(mClipList);
    topSplitter->addWidget(clipsGroup);

    connect(mClipList, &QListWidget::currentRowChanged,
            this, &AnimationEditor::onClipSelected);

    // Center: Timeline + controls
    auto* centerWidget = new QWidget();
    auto* centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setContentsMargins(0, 0, 0, 0);

    mTimeline = new TimelineWidget();
    mTimeline->setMinimumHeight(150);
    centerLayout->addWidget(mTimeline, 1);

    // Transport controls
    auto* transportLayout = new QHBoxLayout();
    auto* playBtn = new QPushButton(tr("Play"));
    playBtn->setEnabled(false);
    playBtn->setObjectName("playBtn");
    auto* stopBtn = new QPushButton(tr("Stop"));
    stopBtn->setEnabled(false);
    stopBtn->setObjectName("stopBtn");
    auto* exportBtn = new QPushButton(tr("Export"));
    auto* importBtn = new QPushButton(tr("Import"));

    transportLayout->addWidget(playBtn);
    transportLayout->addWidget(stopBtn);
    transportLayout->addStretch();
    transportLayout->addWidget(exportBtn);
    transportLayout->addWidget(importBtn);
    centerLayout->addLayout(transportLayout);

    connect(playBtn, &QPushButton::clicked, this, &AnimationEditor::playAnimation);
    connect(stopBtn, &QPushButton::clicked, this, &AnimationEditor::stopAnimation);
    connect(exportBtn, &QPushButton::clicked, this, &AnimationEditor::onExportAnimation);
    connect(importBtn, &QPushButton::clicked, this, &AnimationEditor::onImportAnimation);

    // Marker toolbar
    auto* markerGroup = new QGroupBox(tr("Markers"));
    auto* markerLayout = new QHBoxLayout(markerGroup);
    mAddMarkerBtn = new QPushButton(tr("Add Marker"));
    mRemoveMarkerBtn = new QPushButton(tr("Remove Marker"));
    mRemoveMarkerBtn->setEnabled(false);
    mMarkerList = new QListWidget();
    mMarkerList->setMaximumHeight(80);
    markerLayout->addWidget(mAddMarkerBtn);
    markerLayout->addWidget(mRemoveMarkerBtn);
    markerLayout->addWidget(mMarkerList, 1);
    centerLayout->addWidget(markerGroup);

    connect(mAddMarkerBtn, &QPushButton::clicked, this, &AnimationEditor::onAddMarker);
    connect(mRemoveMarkerBtn, &QPushButton::clicked, this, &AnimationEditor::onRemoveMarker);
    connect(mMarkerList, &QListWidget::currentRowChanged, this, [this](int row) {
        mRemoveMarkerBtn->setEnabled(row >= 0);
        if (row >= 0 && row < mTimeline->markers().size()) {
            mTimeline->setCurrentTime(mTimeline->markers()[row].time);
        }
    });

    // Timeline slider
    mTimeSlider = new QSlider(Qt::Horizontal);
    mTimeSlider->setObjectName("timeSlider");
    mTimeSlider->setEnabled(false);
    centerLayout->addWidget(mTimeSlider);

    auto* timeLayout = new QHBoxLayout();
    mTimeLabel = new QLabel(tr("0.00s / 0.00s"));
    mDurationLabel = new QLabel(tr("Duration: -"));
    timeLayout->addWidget(mTimeLabel);
    timeLayout->addStretch();
    timeLayout->addWidget(mDurationLabel);
    centerLayout->addLayout(timeLayout);

    topSplitter->addWidget(centerWidget);

    connect(mTimeSlider, &QSlider::valueChanged,
            this, &AnimationEditor::onSliderValueChanged);

    // Property panel (right)
    auto* propGroup = new QGroupBox(tr("Keyframe Properties"));
    auto* propLayout = new QFormLayout(propGroup);

    mPropBoneLabel = new QLabel(tr("--"));
    mPropTimeSpin = new QDoubleSpinBox();
    mPropTimeSpin->setRange(0.0, 9999.0);
    mPropTimeSpin->setDecimals(3);
    mPropTimeSpin->setSuffix(tr(" s"));
    mPropTimeSpin->setEnabled(false);

    mPropTxSpin = new QDoubleSpinBox();
    mPropTxSpin->setRange(-99999.0, 99999.0);
    mPropTxSpin->setDecimals(4);
    mPropTySpin = new QDoubleSpinBox();
    mPropTySpin->setRange(-99999.0, 99999.0);
    mPropTySpin->setDecimals(4);
    mPropTzSpin = new QDoubleSpinBox();
    mPropTzSpin->setRange(-99999.0, 99999.0);
    mPropTzSpin->setDecimals(4);

    mPropRxSpin = new QDoubleSpinBox();
    mPropRxSpin->setRange(-360.0, 360.0);
    mPropRxSpin->setDecimals(4);
    mPropRySpin = new QDoubleSpinBox();
    mPropRySpin->setRange(-360.0, 360.0);
    mPropRySpin->setDecimals(4);
    mPropRzSpin = new QDoubleSpinBox();
    mPropRzSpin->setRange(-360.0, 360.0);
    mPropRzSpin->setDecimals(4);

    propLayout->addRow(tr("Bone:"), mPropBoneLabel);
    propLayout->addRow(tr("Time:"), mPropTimeSpin);
    propLayout->addRow(tr("TX:"), mPropTxSpin);
    propLayout->addRow(tr("TY:"), mPropTySpin);
    propLayout->addRow(tr("TZ:"), mPropTzSpin);
    propLayout->addRow(tr("RX:"), mPropRxSpin);
    propLayout->addRow(tr("RY:"), mPropRySpin);
    propLayout->addRow(tr("RZ:"), mPropRzSpin);

    topSplitter->addWidget(propGroup);
    topSplitter->setStretchFactor(0, 0);
    topSplitter->setStretchFactor(1, 1);
    topSplitter->setStretchFactor(2, 0);

    mainLayout->addWidget(topSplitter, 1);

    // --- Behavior graph tab (Phase 22): node-graph canvas ---
    auto* graphTab = new QWidget();
    auto* graphLayout = new QVBoxLayout(graphTab);
    mGraphWidget = new NodeGraphWidget();
    mGraphWidget->setGraph(&mNodeGraph);

    auto* graphButtonRow = new QHBoxLayout();
    auto* addNodeBtn = new QPushButton(tr("Add Node..."));
    graphButtonRow->addWidget(addNodeBtn);
    graphButtonRow->addStretch();
    graphLayout->addLayout(graphButtonRow);
    graphLayout->addWidget(mGraphWidget, 1);

    connect(addNodeBtn, &QPushButton::clicked, this, [this]() {
        if (!mGraphWidget)
            return;
        bool ok = false;
        const QString type = QInputDialog::getItem(this, tr("Add Node"),
            tr("Node type:"), NodeGraphWidget::paletteTypes(), 0, false, &ok);
        if (!ok || type.isEmpty())
            return;
        const QString label = QInputDialog::getText(this, tr("Add Node"),
            tr("Label:"), QLineEdit::Normal, type, &ok);
        if (!ok)
            return;
        mGraphWidget->addNode(type, label.isEmpty() ? type : label,
            QPointF(mGraphWidget->width() / 2.0, mGraphWidget->height() / 2.0));
    });

    auto* tabWidget = new QTabWidget();
    tabWidget->addTab(graphTab, tr("Behavior Graph"));
    mainLayout->addWidget(tabWidget, 0);

    // Connect property spinboxes
    connect(mPropTxSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &AnimationEditor::onPropertyChanged);
    connect(mPropTySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &AnimationEditor::onPropertyChanged);
    connect(mPropTzSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &AnimationEditor::onPropertyChanged);
    connect(mPropRxSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &AnimationEditor::onPropertyChanged);
    connect(mPropRySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &AnimationEditor::onPropertyChanged);
    connect(mPropRzSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &AnimationEditor::onPropertyChanged);

    // Connect TimelineWidget signals
    connect(mTimeline, &TimelineWidget::timeChanged,
            this, &AnimationEditor::onTimelineTimeChanged);
    connect(mTimeline, &TimelineWidget::keyframeSelected,
            this, &AnimationEditor::onTimelineKeyframeSelected);
    connect(mTimeline, &TimelineWidget::keyframeMoved,
            this, &AnimationEditor::onTimelineKeyframeMoved);
    connect(mTimeline, &TimelineWidget::keyframeAdded,
            this, &AnimationEditor::onTimelineKeyframeAdded);
    connect(mTimeline, &TimelineWidget::keyframeRemoved,
            this, &AnimationEditor::onTimelineKeyframeRemoved);
}

void AnimationEditor::browseNif()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open NIF"),
        "", tr("NIF Files (*.nif)"));
    if (!path.isEmpty()) {
        loadAnimation(path);
    }
}

void AnimationEditor::loadAnimation(const QString& path)
{
    delete mAnimation;
    mAnimation = nullptr;
    mClipList->clear();
    mSelectedClip = -1;
    mPlaying = false;
    mCurrentFrame = 0;
    mSelectedBone.clear();
    mSelectedTime = 0.0f;
    mUndoStack->clear();

    auto* playBtn = findChild<QPushButton*>("playBtn");
    auto* stopBtn = findChild<QPushButton*>("stopBtn");
    auto* slider = findChild<QSlider*>("timeSlider");
    if (playBtn) playBtn->setEnabled(false);
    if (stopBtn) stopBtn->setEnabled(false);
    if (slider) {
        slider->setEnabled(false);
        slider->setValue(0);
    }

    mTimeline->setClip(nullptr);

    mAnimation = loadAnimationFromNif(path);
    if (mAnimation) {
        setWindowTitle(tr("Animation Editor - %1").arg(mAnimation->name));

        for (int i = 0; i < mAnimation->clips.size(); ++i) {
            mClipList->addItem(QString("%1 (%2 bones)")
                .arg(mAnimation->clips[i].name)
                .arg(mAnimation->clips[i].channels.size()));
        }

        if (!mAnimation->clips.isEmpty()) {
            mClipList->setCurrentRow(0);
            onClipSelected(0);
        }

        LOG_INFO(QString("Loaded animation: %1 (%2 clips, %3 keyframes)")
                     .arg(mAnimation->name)
                     .arg(mAnimation->clipCount())
                     .arg(mAnimation->totalKeyframeCount()));
    } else {
        QMessageBox::information(this, tr("No Animation"),
                                 tr("No animation data found in this NIF file."));
    }
}

float AnimationEditor::clipDuration() const
{
    if (!mAnimation || mSelectedClip < 0) return 0.0f;
    const auto& clip = mAnimation->clips[mSelectedClip];
    float maxDuration = 0.0f;
    for (const auto& ch : clip.channels) {
        if (!ch.keyframes.isEmpty() && ch.keyframes.last().time > maxDuration) {
            maxDuration = ch.keyframes.last().time;
        }
    }
    return maxDuration;
}

void AnimationEditor::onClipSelected(int index)
{
    if (index < 0 || !mAnimation) return;
    mSelectedClip = index;

    const auto& clip = mAnimation->clips[index];
    auto* playBtn = findChild<QPushButton*>("playBtn");
    auto* slider = findChild<QSlider*>("timeSlider");

    if (playBtn) playBtn->setEnabled(true);
    if (slider) slider->setEnabled(true);

    float maxDuration = clipDuration();

    mDurationLabel->setText(QString("Duration: %1s").arg(maxDuration, 0, 'f', 2));

    if (slider) {
        slider->setRange(0, qMax(1, static_cast<int>(maxDuration * 100)));
        slider->setValue(0);
    }

    updateTimeline();
}

void AnimationEditor::updateTimeline()
{
    if (!mAnimation || mSelectedClip < 0) {
        mTimeline->setClip(nullptr);
        return;
    }

    const auto& clip = mAnimation->clips[mSelectedClip];
    mTimeline->setClip(&clip);
    mTimeline->setCurrentTime(0.0f);
    mTimeSlider->setValue(0);
    mTimeLabel->setText(QString("0.00s / %1s").arg(clipDuration(), 0, 'f', 2));

    // Sync markers from NifAnimation to TimelineWidget
    mTimeline->clearMarkers();
    for (const auto& m : mAnimation->markers) {
        mTimeline->addMarker(m.time, m.name, m.color);
    }
    updateMarkerList();

    LOG_INFO(QString("Timeline updated: clip '%1' with %2 channels")
                 .arg(clip.name)
                 .arg(clip.channels.size()));
}

void AnimationEditor::onTimelineTimeChanged(float time)
{
    if (mPlaying) return;
    int sliderValue = static_cast<int>(time * 100);
    mTimeSlider->blockSignals(true);
    mTimeSlider->setValue(sliderValue);
    mTimeSlider->blockSignals(false);
    mTimeLabel->setText(QString("%1s / %2s").arg(time, 0, 'f', 2)
                            .arg(clipDuration(), 0, 'f', 2));
}

void AnimationEditor::onTimelineKeyframeSelected(const QString& boneName, float time)
{
    mSelectedBone = boneName;
    mSelectedTime = time;
    updatePropertyPanel();
}

void AnimationEditor::onTimelineKeyframeMoved(const QString& boneName, float oldTime, float newTime)
{
    if (!mAnimation || mSelectedClip < 0) return;
    const QString clipName = mAnimation->clips[mSelectedClip].name;

    auto* cmd = new MoveKeyframeCommand(mAnimation, clipName, boneName, oldTime, newTime,
                                        tr("Move keyframe '%1' from %2 to %3")
                                            .arg(boneName)
                                            .arg(oldTime, 0, 'f', 3)
                                            .arg(newTime, 0, 'f', 3));
    mUndoStack->push(new CommandUndoAdapter(cmd));
    updateTimeline();
    mSelectedTime = newTime;
    updatePropertyPanel();
}

void AnimationEditor::onTimelineKeyframeAdded(const QString& boneName, float time)
{
    if (!mAnimation || mSelectedClip < 0) return;
    const QString clipName = mAnimation->clips[mSelectedClip].name;

    AnimKeyframe kf;
    kf.time = time;
    kf.tx = 0.0f;
    kf.ty = 0.0f;
    kf.tz = 0.0f;
    kf.rx = 0.0f;
    kf.ry = 0.0f;
    kf.rz = 0.0f;

    auto* cmd = new AddKeyframeCommand(mAnimation, clipName, boneName, kf,
                                       tr("Add keyframe at %1 for '%2'")
                                           .arg(time, 0, 'f', 3)
                                           .arg(boneName));
    mUndoStack->push(new CommandUndoAdapter(cmd));
    updateTimeline();
}

void AnimationEditor::onTimelineKeyframeRemoved(const QString& boneName, float time)
{
    if (!mAnimation || mSelectedClip < 0) return;
    const QString clipName = mAnimation->clips[mSelectedClip].name;

    auto* cmd = new RemoveKeyframeCommand(mAnimation, clipName, boneName, time,
                                          tr("Remove keyframe at %1 for '%2'")
                                              .arg(time, 0, 'f', 3)
                                              .arg(boneName));
    mUndoStack->push(new CommandUndoAdapter(cmd));
    updateTimeline();
}

void AnimationEditor::updatePropertyPanel()
{
    if (mSelectedBone.isEmpty() || !mAnimation || mSelectedClip < 0) {
        mPropBoneLabel->setText(tr("--"));
        mPropTimeSpin->setValue(0);
        mPropTimeSpin->setEnabled(false);
        mPropTxSpin->setValue(0); mPropTxSpin->setEnabled(false);
        mPropTySpin->setValue(0); mPropTySpin->setEnabled(false);
        mPropTzSpin->setValue(0); mPropTzSpin->setEnabled(false);
        mPropRxSpin->setValue(0); mPropRxSpin->setEnabled(false);
        mPropRySpin->setValue(0); mPropRySpin->setEnabled(false);
        mPropRzSpin->setValue(0); mPropRzSpin->setEnabled(false);
        return;
    }

    const auto& clip = mAnimation->clips[mSelectedClip];
    for (const auto& ch : clip.channels) {
        if (ch.boneName != mSelectedBone) continue;
        for (const auto& kf : ch.keyframes) {
            if (qFuzzyCompare(kf.time, mSelectedTime)) {
                mPropBoneLabel->setText(ch.boneName);

                // Block signals to avoid triggering onPropertyChanged
                auto blocks = QList<QObject*>{mPropTxSpin, mPropTySpin, mPropTzSpin,
                                              mPropRxSpin, mPropRySpin, mPropRzSpin};
                for (auto* obj : blocks) obj->blockSignals(true);

                mPropTimeSpin->setValue(kf.time);
                mPropTimeSpin->setEnabled(false);
                mPropTxSpin->setValue(kf.tx); mPropTxSpin->setEnabled(true);
                mPropTySpin->setValue(kf.ty); mPropTySpin->setEnabled(true);
                mPropTzSpin->setValue(kf.tz); mPropTzSpin->setEnabled(true);
                mPropRxSpin->setValue(kf.rx); mPropRxSpin->setEnabled(true);
                mPropRySpin->setValue(kf.ry); mPropRySpin->setEnabled(true);
                mPropRzSpin->setValue(kf.rz); mPropRzSpin->setEnabled(true);

                for (auto* obj : blocks) obj->blockSignals(false);

                return;
            }
        }
    }

    mSelectedBone.clear();
    updatePropertyPanel();
}

void AnimationEditor::onPropertyChanged()
{
    if (mSelectedBone.isEmpty() || !mAnimation || mSelectedClip < 0) return;

    // NOTE: Property edits are applied directly to keyframes without going
    // through the undo stack. Undo only affects keyframe add/move/remove
    // operations. Implementing undo for property edits would require a
    // dedicated SetKeyframePropertiesCommand for each change.

    const QString clipName = mAnimation->clips[mSelectedClip].name;

    // Find and update the keyframe directly (property panel edits are immediate)
    auto& clip = mAnimation->clips[mSelectedClip];
    for (auto& ch : clip.channels) {
        if (ch.boneName != mSelectedBone) continue;
        for (auto& kf : ch.keyframes) {
            if (qFuzzyCompare(kf.time, mSelectedTime)) {
                kf.tx = static_cast<float>(mPropTxSpin->value());
                kf.ty = static_cast<float>(mPropTySpin->value());
                kf.tz = static_cast<float>(mPropTzSpin->value());
                kf.rx = static_cast<float>(mPropRxSpin->value());
                kf.ry = static_cast<float>(mPropRySpin->value());
                kf.rz = static_cast<float>(mPropRzSpin->value());

                Q_UNUSED(clipName)
                return;
            }
        }
    }
}

void AnimationEditor::onSliderValueChanged(int value)
{
    float currentTime = value / 100.0f;
    mTimeLabel->setText(QString("%1s / %2s").arg(currentTime, 0, 'f', 2)
                            .arg(clipDuration(), 0, 'f', 2));
    mTimeline->setCurrentTime(currentTime);
}

void AnimationEditor::playAnimation()
{
    if (mPlaying) return;
    if (!mAnimation || mSelectedClip < 0) return;
    float maxDuration = clipDuration();
    if (maxDuration <= 0.0f) return;

    mPlaying = true;
    mCurrentFrame = 0;

    auto* playBtn = findChild<QPushButton*>("playBtn");
    auto* stopBtn = findChild<QPushButton*>("stopBtn");
    if (playBtn) playBtn->setEnabled(false);
    if (stopBtn) stopBtn->setEnabled(true);

    auto* slider = findChild<QSlider*>("timeSlider");
    if (slider) slider->setValue(0);

    QTimer* timer = new QTimer(this);
    timer->setInterval(50);
    connect(timer, &QTimer::timeout, this, [this, maxDuration, timer]() {
        if (!mPlaying) {
            timer->stop();
            timer->deleteLater();
            return;
        }

        float currentTime = mCurrentFrame * 0.05f;
        if (currentTime >= maxDuration) {
            mPlaying = false;
            timer->stop();
            timer->deleteLater();
            auto* pb = findChild<QPushButton*>("playBtn");
            auto* sb = findChild<QPushButton*>("stopBtn");
            if (pb) pb->setEnabled(true);
            if (sb) sb->setEnabled(false);
            return;
        }

        mCurrentFrame++;
        mTimeSlider->blockSignals(true);
        mTimeSlider->setValue(static_cast<int>(currentTime * 100));
        mTimeSlider->blockSignals(false);
        mTimeline->setCurrentTime(currentTime);
        mTimeLabel->setText(QString("%1s / %2s").arg(currentTime, 0, 'f', 2)
                                .arg(maxDuration, 0, 'f', 2));
    });
    timer->start();
    LOG_INFO("Animation playback started");
}

void AnimationEditor::stopAnimation()
{
    mPlaying = false;
    mCurrentFrame = 0;

    auto* playBtn = findChild<QPushButton*>("playBtn");
    auto* stopBtn = findChild<QPushButton*>("stopBtn");
    if (playBtn) playBtn->setEnabled(mSelectedClip >= 0);
    if (stopBtn) stopBtn->setEnabled(false);

    auto* slider = findChild<QSlider*>("timeSlider");
    if (slider) {
        slider->blockSignals(true);
        slider->setValue(0);
        slider->blockSignals(false);
    }
    mTimeline->setCurrentTime(0.0f);
    mTimeLabel->setText(QString("0.00s / %1s").arg(clipDuration(), 0, 'f', 2));

    LOG_INFO("Animation playback stopped");
}

void AnimationEditor::onExportAnimation()
{
    if (!mAnimation) return;

    QString path = QFileDialog::getSaveFileName(this, tr("Export Animation"),
        "", tr("JSON Files (*.json)"));
    if (path.isEmpty()) return;

    QJsonObject root;
    root["name"] = mAnimation->name;

    QJsonArray clipsArray;
    for (const auto& clip : mAnimation->clips) {
        QJsonObject clipObj;
        clipObj["name"] = clip.name;
        clipObj["duration"] = clip.duration;

        QJsonArray channelsArray;
        for (const auto& ch : clip.channels) {
            QJsonObject chObj;
            chObj["boneName"] = ch.boneName;
            chObj["type"] = ch.type;

            QJsonArray kfArray;
            for (const auto& kf : ch.keyframes) {
                QJsonObject kfObj;
                kfObj["time"] = kf.time;
                kfObj["tx"] = kf.tx;
                kfObj["ty"] = kf.ty;
                kfObj["tz"] = kf.tz;
                kfObj["rx"] = kf.rx;
                kfObj["ry"] = kf.ry;
                kfObj["rz"] = kf.rz;
                kfArray.append(kfObj);
            }
            chObj["keyframes"] = kfArray;
            channelsArray.append(chObj);
        }
        clipObj["channels"] = channelsArray;
        clipsArray.append(clipObj);
    }
    root["clips"] = clipsArray;

    QJsonArray markersArray;
    for (const auto& m : mTimeline->markers()) {
        QJsonObject markerObj;
        markerObj["time"] = m.time;
        markerObj["name"] = m.name;
        markerObj["color"] = m.color.name();
        markersArray.append(markerObj);
    }
    root["markers"] = markersArray;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        LOG_INFO(QString("Animation exported to %1").arg(path));
    } else {
        QMessageBox::warning(this, tr("Export Failed"),
                             tr("Could not write to %1").arg(path));
    }
}

void AnimationEditor::onImportAnimation()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Import Animation"),
        "", tr("JSON Files (*.json)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Import Failed"),
                             tr("Could not read from %1").arg(path));
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, tr("Import Error"),
                             tr("JSON parse error: %1").arg(parseError.errorString()));
        return;
    }

    QJsonObject root = doc.object();
    auto* anim = new NifAnimation();
    anim->name = root["name"].toString();

    QJsonArray clipsArray = root["clips"].toArray();
    for (const auto& clipVal : clipsArray) {
        QJsonObject clipObj = clipVal.toObject();
        AnimClip clip;
        clip.name = clipObj["name"].toString();
        clip.duration = static_cast<float>(clipObj["duration"].toDouble());

        QJsonArray channelsArray = clipObj["channels"].toArray();
        for (const auto& chVal : channelsArray) {
            QJsonObject chObj = chVal.toObject();
            AnimChannel ch;
            ch.boneName = chObj["boneName"].toString();
            ch.type = chObj["type"].toString();

            QJsonArray kfArray = chObj["keyframes"].toArray();
            for (const auto& kfVal : kfArray) {
                QJsonObject kfObj = kfVal.toObject();
                AnimKeyframe kf;
                kf.time = static_cast<float>(kfObj["time"].toDouble());
                kf.tx = static_cast<float>(kfObj["tx"].toDouble());
                kf.ty = static_cast<float>(kfObj["ty"].toDouble());
                kf.tz = static_cast<float>(kfObj["tz"].toDouble());
                kf.rx = static_cast<float>(kfObj["rx"].toDouble());
                kf.ry = static_cast<float>(kfObj["ry"].toDouble());
                kf.rz = static_cast<float>(kfObj["rz"].toDouble());
                ch.keyframes.append(kf);
            }

            ch.duration = ch.keyframes.isEmpty() ? 0.0f : ch.keyframes.last().time;
            clip.channels.append(ch);
        }

        anim->clips.append(clip);
    }

    if (root.contains("markers")) {
        QJsonArray markersArray = root["markers"].toArray();
        for (const auto& markerVal : markersArray) {
            QJsonObject markerObj = markerVal.toObject();
            AnimMarker m;
            m.time = static_cast<float>(markerObj["time"].toDouble());
            m.name = markerObj["name"].toString();
            m.color = QColor(markerObj["color"].toString());
            anim->markers.append(m);
        }
    }

    delete mAnimation;
    mAnimation = anim;
    mSelectedClip = -1;
    mClipList->clear();
    mUndoStack->clear();

    setWindowTitle(tr("Animation Editor - %1").arg(mAnimation->name));

    for (int i = 0; i < mAnimation->clips.size(); ++i) {
        mClipList->addItem(QString("%1 (%2 bones)")
            .arg(mAnimation->clips[i].name)
            .arg(mAnimation->clips[i].channels.size()));
    }

    if (!mAnimation->clips.isEmpty()) {
        mClipList->setCurrentRow(0);
        onClipSelected(0);
    }

    LOG_INFO(QString("Animation imported from %1").arg(path));
}

void AnimationEditor::onAddMarker()
{
    float currentTime = mTimeline->currentTime();
    bool ok;
    QString name = QInputDialog::getText(this, tr("Add Marker"),
                                          tr("Marker name:"), QLineEdit::Normal,
                                          QString("Marker %1").arg(mTimeline->markers().size() + 1),
                                          &ok);
    if (ok && !name.isEmpty()) {
        mTimeline->addMarker(currentTime, name);
        updateMarkerList();
    }
}

void AnimationEditor::onRemoveMarker()
{
    int row = mMarkerList->currentRow();
    if (row >= 0) {
        mTimeline->removeMarker(row);
        updateMarkerList();
    }
}

void AnimationEditor::updateMarkerList()
{
    mMarkerList->clear();
    for (const auto& m : mTimeline->markers()) {
        mMarkerList->addItem(QString("%1 [%2s]").arg(m.name).arg(m.time, 0, 'f', 2));
    }
    mRemoveMarkerBtn->setEnabled(mMarkerList->currentRow() >= 0);
}
