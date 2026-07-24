#pragma once

#include <QWidget>
#include <QVector>
#include <QPair>
#include <QTimer>
#include <QPointF>
#include <QElapsedTimer>

class WaveformWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WaveformWidget(QWidget* parent = nullptr);

    bool loadAudio(const QString& filePath);
    void setSamples(const QVector<float>& samples, int sampleRate = 44100);
    void setPlayheadPosition(int sample);
    int playheadPosition() const { return mPlayheadPosition; }
    int totalSamples() const { return mSamples.size(); }
    int sampleRate() const { return mSampleRate; }
    double durationSeconds() const;

    void trim(int startSample, int endSample);
    void setVolume(float factor);
    void fadeIn(int numSamples);
    void fadeOut(int numSamples);
    bool saveAudio(const QString& filePath) const;

    void play();
    void pause();
    void stop();
    bool isPlaying() const { return mPlaying; }

    bool hasSelection() const { return mHasSelection; }
    int selectionStart() const { return mSelectionStart; }
    int selectionEnd() const { return mSelectionEnd; }

    void clear();

signals:
    void selectionChanged(int startSample, int endSample);
    void playheadMoved(int sample);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    static constexpr int RulerHeight = 24;
    static constexpr int MinZoom = 1;
    static constexpr int MaxZoom = 4096;

    void rebuildDisplayData();
    int xToSample(int x) const;
    int sampleToX(int sample) const;
    void drawRuler(QPainter& painter, const QRect& rect);
    void drawWaveform(QPainter& painter, const QRect& rect);
    void drawSelection(QPainter& painter, const QRect& rect);
    void drawPlayhead(QPainter& painter, const QRect& rect);
    void startPlaybackTimer();
    void stopPlaybackTimer();

    QVector<float> mSamples;
    QVector<QPair<float, float>> mDisplayData;
    int mSampleRate;
    double mZoom;
    int mOffset;
    int mSelectionStart;
    int mSelectionEnd;
    int mPlayheadPosition;
    bool mHasSelection;

    bool mDragging;
    bool mPanning;
    bool mMiddlePanning;
    QPointF mDragStart;
    int mDragOffsetStart;
    int mDragSelectionStart;
    int mDragSelectionEnd;

    bool mPlaying;
    QTimer mPlaybackTimer;
    QElapsedTimer mElapsedTimer;
    qint64 mPlayStartSample;
};
