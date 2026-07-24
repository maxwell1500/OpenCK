#pragma once

#include <QColor>
#include <QString>
#include <QVector>

struct AnimKeyframe {
    float time = 0.0f;
    float tx = 0.0f, ty = 0.0f, tz = 0.0f;
    float rx = 0.0f, ry = 0.0f, rz = 0.0f;
    float sx = 1.0f, sy = 1.0f, sz = 1.0f;
};

struct AnimChannel {
    QString boneName;
    QString type;
    QVector<AnimKeyframe> keyframes;
    float duration = 0.0f;
};

struct AnimClip {
    QString name;
    QVector<AnimChannel> channels;
    float duration = 0.0f;
};

struct AnimMarker {
    float time = 0.0f;
    QString name;
    QColor color = QColor(255, 255, 0);
};

class NifAnimation {
public:
    QString name;
    QVector<AnimClip> clips;
    QVector<AnimMarker> markers;

    int totalKeyframeCount() const {
        int count = 0;
        for (const auto& clip : clips) {
            for (const auto& ch : clip.channels) {
                count += ch.keyframes.size();
            }
        }
        return count;
    }
    int clipCount() const { return clips.size(); }
};
