#include "removekeyframecommand.hpp"

#include <cmath>

RemoveKeyframeCommand::RemoveKeyframeCommand(NifAnimation* animation, const QString& clipName,
                                             const QString& boneName, float time,
                                             const QString& description)
    : m_animation(animation),
      m_clipName(clipName),
      m_boneName(boneName),
      m_time(time),
      m_clipIndex(-1),
      m_channelIndex(-1),
      m_keyframeIndex(-1)
{
    findKeyframe();
}

bool RemoveKeyframeCommand::findKeyframe()
{
    m_clipIndex = -1;
    m_channelIndex = -1;
    m_keyframeIndex = -1;

    for (int ci = 0; ci < m_animation->clips.size(); ++ci)
    {
        if (m_animation->clips[ci].name != m_clipName)
            continue;

        m_clipIndex = ci;
        for (int chi = 0; chi < m_animation->clips[ci].channels.size(); ++chi)
        {
            if (m_animation->clips[ci].channels[chi].boneName != m_boneName)
                continue;

            m_channelIndex = chi;
            for (int ki = 0; ki < m_animation->clips[ci].channels[chi].keyframes.size(); ++ki)
            {
                if (std::abs(m_animation->clips[ci].channels[chi].keyframes[ki].time - m_time) < 0.001f)
                {
                    m_keyframeIndex = ki;
                    m_keyframe = m_animation->clips[ci].channels[chi].keyframes[ki];
                    return true;
                }
            }
            return false;
        }
    }
    return false;
}

int RemoveKeyframeCommand::findInsertPosition() const
{
    if (m_clipIndex < 0 || m_channelIndex < 0)
        return 0;

    const AnimChannel& channel = m_animation->clips[m_clipIndex].channels[m_channelIndex];
    for (int i = 0; i < channel.keyframes.size(); ++i)
    {
        if (m_keyframe.time < channel.keyframes[i].time)
            return i;
    }
    return channel.keyframes.size();
}

void RemoveKeyframeCommand::updateChannelDuration()
{
    if (m_clipIndex < 0 || m_channelIndex < 0)
        return;

    AnimChannel& channel = m_animation->clips[m_clipIndex].channels[m_channelIndex];
    if (channel.keyframes.isEmpty())
        channel.duration = 0.0f;
    else
        channel.duration = channel.keyframes.last().time;
}

void RemoveKeyframeCommand::execute()
{
    if (m_clipIndex < 0 || m_channelIndex < 0 || m_keyframeIndex < 0)
        return;

    AnimChannel& channel = m_animation->clips[m_clipIndex].channels[m_channelIndex];
    if (m_keyframeIndex >= channel.keyframes.size())
        return;

    m_keyframe = channel.keyframes[m_keyframeIndex];
    channel.keyframes.remove(m_keyframeIndex);
    updateChannelDuration();
}

void RemoveKeyframeCommand::undo()
{
    if (m_clipIndex < 0 || m_channelIndex < 0)
        return;

    AnimChannel& channel = m_animation->clips[m_clipIndex].channels[m_channelIndex];
    int insertPos = findInsertPosition();
    channel.keyframes.insert(insertPos, m_keyframe);
    m_keyframeIndex = insertPos;
    updateChannelDuration();
}

QString RemoveKeyframeCommand::name() const
{
    return "Remove keyframe";
}
