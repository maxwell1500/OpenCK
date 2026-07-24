#include "waveformwidget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QFile>
#include <QDataStream>
#include <QFileInfo>
#include <QStyleOptionViewItem>
#include <cmath>

WaveformWidget::WaveformWidget(QWidget* parent)
    : QWidget(parent)
    , mSampleRate(44100)
    , mZoom(1.0)
    , mOffset(0)
    , mSelectionStart(0)
    , mSelectionEnd(0)
    , mPlayheadPosition(0)
    , mHasSelection(false)
    , mDragging(false)
    , mPanning(false)
    , mMiddlePanning(false)
    , mDragOffsetStart(0)
    , mDragSelectionStart(0)
    , mDragSelectionEnd(0)
    , mPlaying(false)
    , mPlayStartSample(0)
{
    setMinimumHeight(120);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    connect(&mPlaybackTimer, &QTimer::timeout, this, [this]() {
        if (!mPlaying || mSamples.isEmpty()) {
            stopPlaybackTimer();
            return;
        }
        double elapsed = mElapsedTimer.elapsed() / 1000.0;
        int currentSample = mPlayStartSample + static_cast<int>(elapsed * mSampleRate);
        if (currentSample >= mSamples.size()) {
            stop();
            return;
        }
        mPlayheadPosition = currentSample;
        update();
        emit playheadMoved(mPlayheadPosition);
    });
}

double WaveformWidget::durationSeconds() const
{
    if (mSampleRate <= 0 || mSamples.isEmpty()) return 0.0;
    return static_cast<double>(mSamples.size()) / mSampleRate;
}

bool WaveformWidget::loadAudio(const QString& filePath)
{
    QFileInfo fi(filePath);
    QString suffix = fi.suffix().toLower();

    if (suffix == "wav") {
        QFile f(filePath);
        if (!f.open(QIODevice::ReadOnly)) return false;

        QDataStream ds(&f);
        ds.setByteOrder(QDataStream::LittleEndian);

        char riffId[4];
        if (ds.readRawData(riffId, 4) != 4 || memcmp(riffId, "RIFF", 4) != 0) return false;

        quint32 fileSize;
        ds >> fileSize;

        char waveId[4];
        if (ds.readRawData(waveId, 4) != 4 || memcmp(waveId, "WAVE", 4) != 0) return false;

        quint16 channels = 0;
        quint32 sampleRate = 0;
        quint16 bitsPerSample = 0;
        quint32 dataSize = 0;
        bool fmtFound = false;

        while (!ds.atEnd()) {
            char chunkId[4];
            if (ds.readRawData(chunkId, 4) != 4) break;
            quint32 chunkSize;
            ds >> chunkSize;

            if (memcmp(chunkId, "fmt ", 4) == 0) {
                quint16 audioFormat;
                ds >> audioFormat;
                if (audioFormat != 1) return false;
                quint32 byteRate;
                quint16 blockAlign;
                ds >> channels >> sampleRate >> byteRate >> blockAlign >> bitsPerSample;
                if (chunkSize > 16) {
                    f.seek(f.pos() + static_cast<qint64>(chunkSize - 16));
                }
                fmtFound = true;
            } else if (memcmp(chunkId, "data", 4) == 0) {
                dataSize = chunkSize;
                break;
            } else {
                f.seek(f.pos() + static_cast<qint64>(chunkSize));
            }
        }

        if (!fmtFound || dataSize == 0 || sampleRate == 0) return false;

        mSampleRate = static_cast<int>(sampleRate);
        QByteArray rawData = f.read(dataSize);
        f.close();

        int bytesPerSample = bitsPerSample / 8;
        int totalSamplesInFile = dataSize / (channels * bytesPerSample);
        mSamples.resize(totalSamplesInFile);

        const char* ptr = rawData.constData();
        for (int i = 0; i < totalSamplesInFile; ++i) {
            float sum = 0.0f;
            for (quint16 ch = 0; ch < channels; ++ch) {
                if (bitsPerSample == 16) {
                    qint16 val;
                    memcpy(&val, ptr, 2);
                    sum += val / 32768.0f;
                    ptr += 2;
                } else if (bitsPerSample == 8) {
                    qint8 val = static_cast<qint8>(*ptr);
                    sum += val / 128.0f;
                    ptr += 1;
                } else if (bitsPerSample == 24) {
                    qint32 val = static_cast<qint32>(static_cast<quint8>(ptr[0]))
                               | (static_cast<qint32>(static_cast<qint8>(ptr[1])) << 8)
                               | (static_cast<qint32>(static_cast<qint8>(ptr[2])) << 16);
                    sum += val / 8388608.0f;
                    ptr += 3;
                } else if (bitsPerSample == 32) {
                    qint32 val;
                    memcpy(&val, ptr, 4);
                    sum += val / 2147483648.0f;
                    ptr += 4;
                }
            }
            mSamples[i] = sum / channels;
        }

        mZoom = 1.0;
        mOffset = 0;
        mSelectionStart = 0;
        mSelectionEnd = 0;
        mHasSelection = false;
        mPlayheadPosition = 0;
        rebuildDisplayData();
        update();
        return true;
    }

    return false;
}

void WaveformWidget::setSamples(const QVector<float>& samples, int sampleRate)
{
    stop();
    mSamples = samples;
    mSampleRate = sampleRate;
    mZoom = 1.0;
    mOffset = 0;
    mSelectionStart = 0;
    mSelectionEnd = 0;
    mHasSelection = false;
    mPlayheadPosition = 0;
    rebuildDisplayData();
    update();
}

void WaveformWidget::setPlayheadPosition(int sample)
{
    if (sample < 0) sample = 0;
    if (sample >= mSamples.size()) sample = mSamples.size() - 1;
    mPlayheadPosition = sample;
    update();
    emit playheadMoved(mPlayheadPosition);
}

void WaveformWidget::clear()
{
    stop();
    mSamples.clear();
    mDisplayData.clear();
    mSelectionStart = 0;
    mSelectionEnd = 0;
    mHasSelection = false;
    mPlayheadPosition = 0;
    mZoom = 1.0;
    mOffset = 0;
    update();
}

void WaveformWidget::rebuildDisplayData()
{
    mDisplayData.clear();
    if (mSamples.isEmpty()) return;

    int w = width();
    if (w <= 0) w = 800;

    mDisplayData.resize(w);
    int samplesPerPixel = qMax(1, static_cast<int>(mZoom));

    for (int x = 0; x < w; ++x) {
        int startIdx = mOffset + x * samplesPerPixel;
        int endIdx = qMin(startIdx + samplesPerPixel, mSamples.size());

        if (startIdx >= mSamples.size()) {
            mDisplayData[x] = {0.0f, 0.0f};
            continue;
        }

        float minVal = 1.0f;
        float maxVal = -1.0f;
        for (int i = startIdx; i < endIdx; ++i) {
            float v = mSamples[i];
            if (v < minVal) minVal = v;
            if (v > maxVal) maxVal = v;
        }
        mDisplayData[x] = {minVal, maxVal};
    }
}

int WaveformWidget::xToSample(int x) const
{
    int samplesPerPixel = qMax(1, static_cast<int>(mZoom));
    return mOffset + x * samplesPerPixel;
}

int WaveformWidget::sampleToX(int sample) const
{
    int samplesPerPixel = qMax(1, static_cast<int>(mZoom));
    return (sample - mOffset) / samplesPerPixel;
}

void WaveformWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    QRect fullRect = rect();
    painter.fillRect(fullRect, QColor(30, 30, 30));

    if (mSamples.isEmpty()) {
        painter.setPen(QColor(120, 120, 120));
        painter.drawText(fullRect, Qt::AlignCenter, tr("No audio loaded"));
        return;
    }

    QRect rulerRect(0, 0, width(), RulerHeight);
    QRect waveRect(0, RulerHeight, width(), height() - RulerHeight);

    painter.save();
    painter.setClipRect(waveRect);

    drawSelection(painter, waveRect);
    drawWaveform(painter, waveRect);
    drawPlayhead(painter, waveRect);

    painter.restore();
    drawRuler(painter, rulerRect);

    painter.setPen(QColor(60, 60, 60));
    painter.drawLine(0, RulerHeight, width(), RulerHeight);
}

void WaveformWidget::drawRuler(QPainter& painter, const QRect& rect)
{
    painter.fillRect(rect, QColor(45, 45, 45));

    int samplesPerPixel = qMax(1, static_cast<int>(mZoom));
    double secondsPerPixel = static_cast<double>(samplesPerPixel) / mSampleRate;

    double rulerStep = 0.1;
    if (secondsPerPixel > 0.5) rulerStep = 1.0;
    if (secondsPerPixel > 2.0) rulerStep = 5.0;
    if (secondsPerPixel > 10.0) rulerStep = 30.0;

    double startTime = static_cast<double>(mOffset) / mSampleRate;
    double firstTick = std::ceil(startTime / rulerStep) * rulerStep;

    painter.setPen(QColor(150, 150, 150));
    QFont font = painter.font();
    font.setPixelSize(10);
    painter.setFont(font);

    for (double t = firstTick; ; t += rulerStep) {
        int sample = static_cast<int>(t * mSampleRate);
        int x = sampleToX(sample);
        if (x > width()) break;
        if (x < 0) continue;

        int tickHeight = 6;
        painter.drawLine(x, rect.bottom() - tickHeight, x, rect.bottom());

        int minutes = static_cast<int>(t) / 60;
        double seconds = t - minutes * 60.0;
        QString label;
        if (minutes > 0)
            label = QString("%1:%2").arg(minutes).arg(seconds, 0, 'f', 1);
        else
            label = QString("%1s").arg(seconds, 0, 'f', 2);

        painter.drawText(x + 2, rect.bottom() - tickHeight - 2, label);
    }

    double subStep = rulerStep / 5.0;
    if (subStep * secondsPerPixel > 2.0) {
        painter.setPen(QColor(80, 80, 80));
        double subStart = std::ceil(startTime / subStep) * subStep;
        for (double t = subStart; ; t += subStep) {
            int sample = static_cast<int>(t * mSampleRate);
            int x = sampleToX(sample);
            if (x > width()) break;
            if (x < 0) continue;
            painter.drawLine(x, rect.bottom() - 3, x, rect.bottom());
        }
    }
}

void WaveformWidget::drawWaveform(QPainter& painter, const QRect& rect)
{
    if (mDisplayData.isEmpty()) return;

    int centerY = rect.top() + rect.height() / 2;
    int halfHeight = rect.height() / 2 - 4;

    QVector<QPointF> upperPoly;
    QVector<QPointF> lowerPoly;
    upperPoly.reserve(mDisplayData.size() * 2);
    lowerPoly.reserve(mDisplayData.size() * 2);

    for (int x = 0; x < mDisplayData.size() && x < width(); ++x) {
        float minVal = mDisplayData[x].first;
        float maxVal = mDisplayData[x].second;

        double upperY = centerY - maxVal * halfHeight;
        double lowerY = centerY - minVal * halfHeight;

        upperPoly.append(QPointF(x, upperY));
        lowerPoly.append(QPointF(x, lowerY));
    }

    QPainterPath upperPath;
    if (!upperPoly.isEmpty()) {
        upperPath.moveTo(upperPoly.first().x(), centerY);
        for (const auto& p : upperPoly)
            upperPath.lineTo(p.x(), p.y());
        upperPath.lineTo(upperPoly.last().x(), centerY);
        upperPath.closeSubpath();
    }
    painter.fillPath(upperPath, QColor(0, 180, 80));

    QPainterPath lowerPath;
    if (!lowerPoly.isEmpty()) {
        lowerPath.moveTo(lowerPoly.first().x(), centerY);
        for (const auto& p : lowerPoly)
            lowerPath.lineTo(p.x(), p.y());
        lowerPath.lineTo(lowerPoly.last().x(), centerY);
        lowerPath.closeSubpath();
    }
    painter.fillPath(lowerPath, QColor(0, 140, 200));

    painter.setPen(QColor(0, 200, 100));
    for (int x = 0; x < upperPoly.size(); ++x) {
        painter.drawPoint(upperPoly[x].x(), upperPoly[x].y());
    }
    painter.setPen(QColor(0, 160, 220));
    for (int x = 0; x < lowerPoly.size(); ++x) {
        painter.drawPoint(lowerPoly[x].x(), lowerPoly[x].y());
    }

    painter.setPen(QColor(80, 80, 80));
    painter.drawLine(0, centerY, width(), centerY);
}

void WaveformWidget::drawSelection(QPainter& painter, const QRect& rect)
{
    if (!mHasSelection) return;

    int x1 = sampleToX(mSelectionStart);
    int x2 = sampleToX(mSelectionEnd);

    if (x1 > x2) qSwap(x1, x2);

    x1 = qMax(x1, rect.left());
    x2 = qMin(x2, rect.right());

    if (x1 >= x2) return;

    QColor selColor(50, 100, 220, 60);
    painter.fillRect(x1, rect.top(), x2 - x1, rect.height(), selColor);

    painter.setPen(QPen(QColor(80, 140, 255), 2));
    painter.drawLine(x1, rect.top(), x1, rect.bottom());
    painter.drawLine(x2, rect.top(), x2, rect.bottom());
}

void WaveformWidget::drawPlayhead(QPainter& painter, const QRect& rect)
{
    int x = sampleToX(mPlayheadPosition);
    if (x < rect.left() || x > rect.right()) return;

    painter.setPen(QPen(QColor(220, 40, 40), 2));
    painter.drawLine(x, rect.top(), x, rect.bottom());

    painter.setBrush(QColor(220, 40, 40));
    painter.setPen(Qt::NoPen);
    QPolygonF triangle;
    triangle << QPointF(x - 5, rect.top()) << QPointF(x + 5, rect.top()) << QPointF(x, rect.top() + 7);
    painter.drawPolygon(triangle);
}

void WaveformWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        mMiddlePanning = true;
        mDragStart = event->pos();
        mDragOffsetStart = mOffset;
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (event->button() == Qt::LeftButton && event->pos().y() > RulerHeight) {
        int sample = xToSample(static_cast<int>(event->pos().x()));
        if (event->modifiers() & Qt::ShiftModifier && mHasSelection) {
            mSelectionEnd = sample;
            if (mSelectionEnd < mSelectionStart) {
                qSwap(mSelectionStart, mSelectionEnd);
            }
            emit selectionChanged(mSelectionStart, mSelectionEnd);
            update();
        } else {
            mDragging = true;
            mDragStart = event->pos();
            mDragSelectionStart = mSelectionStart;
            mDragSelectionEnd = mSelectionEnd;
            mSelectionStart = sample;
            mSelectionEnd = sample;
            mHasSelection = false;
            update();
        }
    }

    if (event->button() == Qt::RightButton && event->pos().y() > RulerHeight) {
        int sample = xToSample(static_cast<int>(event->pos().x()));
        setPlayheadPosition(sample);
    }
}

void WaveformWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (mMiddlePanning) {
        int dx = static_cast<int>(event->pos().x() - mDragStart.x());
        int samplesPerPixel = qMax(1, static_cast<int>(mZoom));
        mOffset = mDragOffsetStart - dx * samplesPerPixel;
        if (mOffset < 0) mOffset = 0;
        rebuildDisplayData();
        update();
        return;
    }

    if (mDragging) {
        int sample = xToSample(static_cast<int>(event->pos().x()));
        mSelectionEnd = sample;
        if (mSelectionStart != mSelectionEnd) {
            mHasSelection = true;
            if (mSelectionEnd < mSelectionStart) {
                int tmp = mSelectionStart;
                mSelectionStart = mSelectionEnd;
                mSelectionEnd = tmp;
            }
            emit selectionChanged(mSelectionStart, mSelectionEnd);
        }
        update();
    }
}

void WaveformWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        mMiddlePanning = false;
        setCursor(Qt::ArrowCursor);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        mDragging = false;
        if (mHasSelection && mSelectionStart == mSelectionEnd) {
            mHasSelection = false;
        }
    }
}

void WaveformWidget::wheelEvent(QWheelEvent* event)
{
    QPoint pos = event->position().toPoint();
    int sampleAtCursor = xToSample(static_cast<int>(pos.x()));

    double oldZoom = mZoom;
    if (event->angleDelta().y() > 0) {
        mZoom /= 2.0;
    } else {
        mZoom *= 2.0;
    }
    mZoom = qBound(static_cast<double>(MinZoom), mZoom, static_cast<double>(MaxZoom));

    double ratio = (pos.x() > 0) ? static_cast<double>(pos.x()) / width() : 0.5;
    mOffset = sampleAtCursor - static_cast<int>(ratio * width() * mZoom);
    if (mOffset < 0) mOffset = 0;

    rebuildDisplayData();
    update();
}

void WaveformWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    rebuildDisplayData();
}

void WaveformWidget::trim(int startSample, int endSample)
{
    if (mSamples.isEmpty()) return;
    if (startSample < 0) startSample = 0;
    if (endSample > mSamples.size()) endSample = mSamples.size();
    if (startSample >= endSample) return;

    stop();
    QVector<float> trimmed(endSample - startSample);
    for (int i = 0; i < trimmed.size(); ++i) {
        trimmed[i] = mSamples[startSample + i];
    }
    mSamples = trimmed;
    mOffset = 0;
    mSelectionStart = 0;
    mSelectionEnd = 0;
    mHasSelection = false;
    mPlayheadPosition = 0;
    rebuildDisplayData();
    update();
}

void WaveformWidget::setVolume(float factor)
{
    if (mSamples.isEmpty()) return;
    stop();
    for (int i = 0; i < mSamples.size(); ++i) {
        mSamples[i] = qBound(-1.0f, mSamples[i] * factor, 1.0f);
    }
    rebuildDisplayData();
    update();
}

void WaveformWidget::fadeIn(int numSamples)
{
    if (mSamples.isEmpty() || numSamples <= 0) return;
    stop();
    int count = qMin(numSamples, mSamples.size());
    for (int i = 0; i < count; ++i) {
        float factor = static_cast<float>(i) / count;
        mSamples[i] *= factor;
    }
    rebuildDisplayData();
    update();
}

void WaveformWidget::fadeOut(int numSamples)
{
    if (mSamples.isEmpty() || numSamples <= 0) return;
    stop();
    int startIdx = qMax(0, mSamples.size() - numSamples);
    int count = mSamples.size() - startIdx;
    for (int i = 0; i < count; ++i) {
        float factor = 1.0f - static_cast<float>(i) / count;
        mSamples[startIdx + i] *= factor;
    }
    rebuildDisplayData();
    update();
}

bool WaveformWidget::saveAudio(const QString& filePath) const
{
    if (mSamples.isEmpty()) return false;

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly)) return false;

    quint16 channels = 1;
    quint32 sampleRate = static_cast<quint32>(mSampleRate);
    quint16 bitsPerSample = 16;
    quint16 blockAlign = channels * (bitsPerSample / 8);
    quint32 byteRate = sampleRate * blockAlign;
    quint32 dataSize = static_cast<quint32>(mSamples.size()) * blockAlign;

    QDataStream ds(&f);
    ds.setByteOrder(QDataStream::LittleEndian);

    ds.writeRawData("RIFF", 4);
    quint32 fileSize = 36 + dataSize;
    ds << fileSize;
    ds.writeRawData("WAVE", 4);

    ds.writeRawData("fmt ", 4);
    quint32 fmtSize = 16;
    ds << fmtSize;
    ds << static_cast<quint16>(1);
    ds << channels;
    ds << sampleRate;
    ds << byteRate;
    ds << blockAlign;
    ds << bitsPerSample;

    ds.writeRawData("data", 4);
    ds << dataSize;

    for (int i = 0; i < mSamples.size(); ++i) {
        float s = qBound(-1.0f, mSamples[i], 1.0f);
        qint16 val = static_cast<qint16>(s * 32767.0f);
        ds << val;
    }

    f.close();
    return true;
}

void WaveformWidget::play()
{
    if (mSamples.isEmpty()) return;
    if (mPlaying) stop();
    mPlaying = true;
    mPlayStartSample = mPlayheadPosition;
    mElapsedTimer.start();
    startPlaybackTimer();
    update();
}

void WaveformWidget::pause()
{
    if (!mPlaying) return;
    mPlaying = false;
    stopPlaybackTimer();
    double elapsed = mElapsedTimer.elapsed() / 1000.0;
    mPlayheadPosition = mPlayStartSample + static_cast<int>(elapsed * mSampleRate);
    if (mPlayheadPosition >= mSamples.size()) {
        mPlayheadPosition = mSamples.size() - 1;
    }
    update();
}

void WaveformWidget::stop()
{
    mPlaying = false;
    stopPlaybackTimer();
    update();
}

void WaveformWidget::startPlaybackTimer()
{
    mPlaybackTimer.start(16);
}

void WaveformWidget::stopPlaybackTimer()
{
    mPlaybackTimer.stop();
}
