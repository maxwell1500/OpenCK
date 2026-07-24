#ifndef NIFANIMATIONSTATE_HPP
#define NIFANIMATIONSTATE_HPP

#include <QObject>
#include <QTimer>
#include <QVector>
#include <QString>
#include <cmath>

#include "../../libs/files/nifanim/nifanimation.hpp"

inline void eulerToQuat(float rx, float ry, float rz, float& qw, float& qx, float& qy, float& qz) {
    float cr = std::cos(rx * 0.5f), sr = std::sin(rx * 0.5f);
    float cp = std::cos(ry * 0.5f), sp = std::sin(ry * 0.5f);
    float cy = std::cos(rz * 0.5f), sy = std::sin(rz * 0.5f);
    qw = cr * cp * cy + sr * sp * sy;
    qx = sr * cp * cy - cr * sp * sy;
    qy = cr * sp * cy + sr * cp * sy;
    qz = cr * cp * sy - sr * sp * cy;
}
inline void quatToEuler(float qw, float qx, float qy, float qz, float& rx, float& ry, float& rz) {
    rx = std::atan2(2.0f * (qw * qx + qy * qz), 1.0f - 2.0f * (qx * qx + qy * qy));
    ry = std::asin(qMax(-1.0f, qMin(1.0f, 2.0f * (qw * qy - qz * qx))));
    rz = std::atan2(2.0f * (qw * qz + qx * qy), 1.0f - 2.0f * (qy * qy + qz * qz));
}

struct TransformKeyframe {
    QString nodeName;
    float tx = 0.0f, ty = 0.0f, tz = 0.0f;
    float rx = 0.0f, ry = 0.0f, rz = 0.0f;
    float sx = 1.0f, sy = 1.0f, sz = 1.0f;
};

class NifAnimationState : public QObject {
    Q_OBJECT
public:
    enum PlayState { Stopped, Playing, Paused };

    explicit NifAnimationState(QObject* parent = nullptr)
        : QObject(parent)
        , m_timer(new QTimer(this))
        , m_state(Stopped)
    {
        connect(m_timer, &QTimer::timeout, this, &NifAnimationState::onTimerTick);
    }

    void setAnimation(NifAnimation* animation) {
        stop();
        m_animation = animation;
        m_clips.clear();
        if (m_animation) {
            for (const auto& clip : m_animation->clips) {
                m_clips.append(clip.name);
            }
            if (!m_clips.isEmpty()) {
                setClip(m_clips.first());
            }
        }
    }

    void setClip(const QString& clipName) {
        if (m_currentClip == clipName && m_state != Stopped)
            return;

        stop();
        m_currentClip = clipName;

        if (!m_animation)
            return;

        for (const auto& clip : m_animation->clips) {
            if (clip.name == clipName) {
                m_duration = clip.duration;
                m_currentTime = 0.0f;
                m_currentChannels = clip.channels;
                emit timeChanged(m_currentTime);
                return;
            }
        }
    }

    void play() {
        if (m_state == Playing)
            return;
        if (!m_animation || m_currentClip.isEmpty())
            return;

        m_state = Playing;
        m_timer->start(33);
        emit stateChanged(Playing);
    }

    void pause() {
        if (m_state != Playing)
            return;
        m_timer->stop();
        m_state = Paused;
        emit stateChanged(Paused);
    }

    void stop() {
        m_timer->stop();
        bool wasPlaying = (m_state != Stopped);
        m_state = Stopped;
        m_currentTime = 0.0f;
        if (wasPlaying) {
            emit timeChanged(m_currentTime);
            emit stateChanged(Stopped);
        }
    }

    void setSpeed(float speed) {
        if (speed <= 0.0f)
            return;
        m_speed = speed;
    }

    void setCurrentTime(float time) {
        if (time < 0.0f)
            time = 0.0f;
        if (time > m_duration)
            time = m_duration;
        m_currentTime = time;
        emit timeChanged(m_currentTime);
    }

    void setLooping(bool loop) {
        m_looping = loop;
    }

    void setBlendAnimation(NifAnimation* anim) { m_blendAnimation = anim; }

    void setBlendClip(const QString& clipName) {
        m_blendChannels.clear();
        if (!m_blendAnimation) return;
        for (const auto& clip : m_blendAnimation->clips) {
            if (clip.name == clipName) {
                m_blendChannels = clip.channels;
                break;
            }
        }
    }

    void setBlendWeight(float weight) { m_blendWeight = qBound(0.0f, weight, 1.0f); }

    void resetCrossfade() {
        m_isCrossfading = false;
        m_crossfadeElapsed = 0.0f;
        m_crossfadeDuration = 0.0f;
        m_blendWeight = 0.0f;
    }

    float blendWeight() const { return m_blendWeight; }

    void startCrossfade(float duration) {
        m_crossfadeDuration = duration;
        m_crossfadeElapsed = 0.0f;
        m_isCrossfading = (duration > 0.0f);
        m_blendWeight = 0.0f;
    }

    PlayState state() const { return m_state; }
    float currentTime() const { return m_currentTime; }
    float duration() const { return m_duration; }
    float speed() const { return m_speed; }
    bool isLooping() const { return m_looping; }
    QString currentClip() const { return m_currentClip; }
    QStringList availableClips() const { return m_clips; }

    QVector<TransformKeyframe> getCurrentFrame() const {
        QVector<TransformKeyframe> result;
        if (!m_animation || m_currentClip.isEmpty())
            return result;

        for (const auto& channel : m_currentChannels) {
            if (channel.keyframes.isEmpty())
                continue;

            TransformKeyframe frame = interpolateChannel(channel, m_currentTime);

            if (m_blendWeight > 0.0f && !m_blendChannels.isEmpty()) {
                for (const auto& blendCh : m_blendChannels) {
                    if (blendCh.boneName == channel.boneName && !blendCh.keyframes.isEmpty()) {
                        TransformKeyframe blendFrame = interpolateChannel(blendCh, m_currentTime);
                        float w = m_blendWeight;
                        frame.tx = frame.tx * (1 - w) + blendFrame.tx * w;
                        frame.ty = frame.ty * (1 - w) + blendFrame.ty * w;
                        frame.tz = frame.tz * (1 - w) + blendFrame.tz * w;
                        frame.sx = frame.sx * (1 - w) + blendFrame.sx * w;
                        frame.sy = frame.sy * (1 - w) + blendFrame.sy * w;
                        frame.sz = frame.sz * (1 - w) + blendFrame.sz * w;
                        float qw1, qx1, qy1, qz1;
                        float qw2, qx2, qy2, qz2;
                        eulerToQuat(frame.rx, frame.ry, frame.rz, qw1, qx1, qy1, qz1);
                        eulerToQuat(blendFrame.rx, blendFrame.ry, blendFrame.rz, qw2, qx2, qy2, qz2);
                        float dot = qw1*qw2 + qx1*qx2 + qy1*qy2 + qz1*qz2;
                        if (dot < 0.0f) { qw2 = -qw2; qx2 = -qx2; qy2 = -qy2; qz2 = -qz2; dot = -dot; }
                        float angle = std::acos(qMin(1.0f, dot));
                        float sinA = std::sin(angle);
                        float wa, wb;
                        if (sinA > 1e-6f) { wa = std::sin((1 - w) * angle) / sinA; wb = std::sin(w * angle) / sinA; }
                        else { wa = 1.0f - w; wb = w; }
                        float rqw = wa*qw1 + wb*qw2;
                        float rqx = wa*qx1 + wb*qx2;
                        float rqy = wa*qy1 + wb*qy2;
                        float rqz = wa*qz1 + wb*qz2;
                        quatToEuler(rqw, rqx, rqy, rqz, frame.rx, frame.ry, frame.rz);
                        break;
                    }
                }
            }

            result.append(frame);
        }

        return result;
    }

signals:
    void timeChanged(float time);
    void stateChanged(NifAnimationState::PlayState state);
    void animationFinished();

private slots:
    void onTimerTick() {
        if (m_state != Playing)
            return;

        float elapsed = m_speed * 0.033f;
        m_currentTime += elapsed;

        if (m_isCrossfading) {
            m_crossfadeElapsed += elapsed;
            if (m_crossfadeDuration > 0) {
                m_blendWeight = qMin(1.0f, m_crossfadeElapsed / m_crossfadeDuration);
            }
            if (m_crossfadeElapsed >= m_crossfadeDuration) {
                m_isCrossfading = false;
                m_blendWeight = 1.0f;
            }
        }

        if (m_currentTime >= m_duration) {
            if (m_looping) {
                if (m_duration > 0.0f) {
                    m_currentTime = std::fmod(m_currentTime, m_duration);
                }
                if (m_currentTime < 0.0f)
                    m_currentTime = 0.0f;
            } else {
                m_currentTime = m_duration;
                m_timer->stop();
                m_state = Stopped;
                emit timeChanged(m_currentTime);
                emit stateChanged(Stopped);
                emit animationFinished();
                return;
            }
        }

        emit timeChanged(m_currentTime);
    }

private:
    TransformKeyframe interpolateChannel(const AnimChannel& channel, float time) const {
        TransformKeyframe frame;
        frame.nodeName = channel.boneName;

        if (channel.keyframes.isEmpty())
            return frame;

        if (channel.keyframes.size() == 1) {
            frame.tx = channel.keyframes[0].tx;
            frame.ty = channel.keyframes[0].ty;
            frame.tz = channel.keyframes[0].tz;
            frame.rx = channel.keyframes[0].rx;
            frame.ry = channel.keyframes[0].ry;
            frame.rz = channel.keyframes[0].rz;
            frame.sx = channel.keyframes[0].sx;
            frame.sy = channel.keyframes[0].sy;
            frame.sz = channel.keyframes[0].sz;
            return frame;
        }

        const auto& kfs = channel.keyframes;

        if (time <= kfs.first().time) {
            frame.tx = kfs.first().tx;
            frame.ty = kfs.first().ty;
            frame.tz = kfs.first().tz;
            frame.rx = kfs.first().rx;
            frame.ry = kfs.first().ry;
            frame.rz = kfs.first().rz;
            frame.sx = kfs.first().sx;
            frame.sy = kfs.first().sy;
            frame.sz = kfs.first().sz;
            return frame;
        }

        if (time >= kfs.last().time) {
            frame.tx = kfs.last().tx;
            frame.ty = kfs.last().ty;
            frame.tz = kfs.last().tz;
            frame.rx = kfs.last().rx;
            frame.ry = kfs.last().ry;
            frame.rz = kfs.last().rz;
            frame.sx = kfs.last().sx;
            frame.sy = kfs.last().sy;
            frame.sz = kfs.last().sz;
            return frame;
        }

        for (int i = 0; i < kfs.size() - 1; ++i) {
            if (time >= kfs[i].time && time <= kfs[i + 1].time) {
                float span = kfs[i + 1].time - kfs[i].time;
                float alpha = (span > 0.0f) ? (time - kfs[i].time) / span : 0.0f;
                frame.tx = kfs[i].tx + (kfs[i + 1].tx - kfs[i].tx) * alpha;
                frame.ty = kfs[i].ty + (kfs[i + 1].ty - kfs[i].ty) * alpha;
                frame.tz = kfs[i].tz + (kfs[i + 1].tz - kfs[i].tz) * alpha;
                frame.rx = kfs[i].rx + (kfs[i + 1].rx - kfs[i].rx) * alpha;
                frame.ry = kfs[i].ry + (kfs[i + 1].ry - kfs[i].ry) * alpha;
                frame.rz = kfs[i].rz + (kfs[i + 1].rz - kfs[i].rz) * alpha;
                frame.sx = kfs[i].sx + (kfs[i + 1].sx - kfs[i].sx) * alpha;
                frame.sy = kfs[i].sy + (kfs[i + 1].sy - kfs[i].sy) * alpha;
                frame.sz = kfs[i].sz + (kfs[i + 1].sz - kfs[i].sz) * alpha;
                break;
            }
        }

        return frame;
    }

    QTimer* m_timer;
    PlayState m_state;
    float m_currentTime = 0.0f;
    float m_duration = 0.0f;
    float m_speed = 1.0f;
    bool m_looping = true;
    QString m_currentClip;
    QStringList m_clips;
    NifAnimation* m_animation = nullptr;
    QVector<AnimChannel> m_currentChannels;

    NifAnimation* m_blendAnimation = nullptr;
    QVector<AnimChannel> m_blendChannels;
    float m_blendWeight = 0.0f;
    float m_crossfadeDuration = 0.0f;
    float m_crossfadeElapsed = 0.0f;
    bool m_isCrossfading = false;
};

#endif // NIFANIMATIONSTATE_HPP
