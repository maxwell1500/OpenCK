#pragma once

#include <QDialog>
#include <QListWidget>
#include <QSlider>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QUndoStack>
#include <memory>

#include "../../../model/tools/nodegraph.hpp"

class NifAnimation;
class TimelineWidget;
class NodeGraphWidget;
class NifBlockFile;
namespace Nif { class NifParser; }

class AnimationEditor : public QDialog
{
    Q_OBJECT

public:
    explicit AnimationEditor(QWidget* parent = nullptr);
    ~AnimationEditor() override;
    bool loadNif(const QString& path);
    bool saveNif();

    const QString& sourceNifPath() const { return mSourceNifPath; }
    NifAnimation* animation() const { return mAnimation; }
    QUndoStack* undoStack() const { return mUndoStack; }
    const NifBlockFile* sourceBlockFile() const { return mSourceBlockFile.get(); }
    const Nif::NifParser* sourceParser() const { return mSourceParser.get(); }

private slots:
    void browseNif();
    void loadAnimation(const QString& path);
    void onClipSelected(int index);
    void playAnimation();
    void stopAnimation();
    void onSliderValueChanged(int value);

    void onTimelineTimeChanged(float time);
    void onTimelineKeyframeSelected(const QString& boneName, float time);
    void onTimelineKeyframeMoved(const QString& boneName, float oldTime, float newTime);
    void onTimelineKeyframeAdded(const QString& boneName, float time);
    void onTimelineKeyframeRemoved(const QString& boneName, float time);

    void onPropertyChanged();

    void onExportAnimation();
    void onImportAnimation();
    void onSaveNif();

    void onAddMarker();
    void onRemoveMarker();
    void updateMarkerList();

private:
    void updateTimeline();
    void updatePropertyPanel();
    float clipDuration() const;

    QListWidget* mClipList;
    TimelineWidget* mTimeline;
    QSlider* mTimeSlider;
    QLabel* mTimeLabel;
    QLabel* mDurationLabel;
    NifAnimation* mAnimation;
    QString mSourceNifPath;
    std::unique_ptr<NifBlockFile> mSourceBlockFile;
    std::unique_ptr<Nif::NifParser> mSourceParser;
    int mSelectedClip;
    bool mPlaying;
    int mCurrentFrame;
    QUndoStack* mUndoStack;

    QListWidget* mMarkerList;
    QPushButton* mAddMarkerBtn;
    QPushButton* mRemoveMarkerBtn;

    QLabel* mPropBoneLabel;
    QDoubleSpinBox* mPropTimeSpin;
    QDoubleSpinBox* mPropTxSpin;
    QDoubleSpinBox* mPropTySpin;
    QDoubleSpinBox* mPropTzSpin;
    QDoubleSpinBox* mPropRxSpin;
    QDoubleSpinBox* mPropRySpin;
    QDoubleSpinBox* mPropRzSpin;

    QString mSelectedBone;
    float mSelectedTime;

    // Behavior graph (Phase 22)
    NodeGraph mNodeGraph;
    NodeGraphWidget* mGraphWidget;
};
