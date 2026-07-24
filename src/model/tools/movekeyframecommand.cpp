#include "movekeyframecommand.hpp"

#include <cmath>

MoveKeyframeCommand::MoveKeyframeCommand(NifAnimation* animation, const QString& clipName,
                                         const QString& boneName, float oldTime, float newTime,
                                         const QString& description)
    : m_animation(animation),
      m_clipName(clipName),
      m_boneName(boneName),
      m_oldTime(oldTime),
      m_newTime(newTime),
      m_channelIndex(-1),
      m_keyframeIndex(-1)
{
    findKeyframe(m_oldTime);
}

bool MoveKeyframeCommand::findKeyframe(float time)
{
    for (int ci = 0; ci < m_animation->clips.size(); ++ci)
    {
        if (m_animation->clips[ci].name != m_clipName)
            continue;

        for (int chi = 0; chi < m_animation->clips[ci].channels.size(); ++chi)
        {
            if (m_animation->clips[ci].channels[chi].boneName != m_boneName)
                continue;

            m_channelIndex = chi;
            for (int ki = 0; ki < m_animation->clips[ci].channels[chi].keyframes.size(); ++ki)
            {
                if (std::abs(m_animation->clips[ci].channels[chi].keyframes[ki].time - time) < 0.001f)
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

void MoveKeyframeCommand::updateChannelDuration(int clipIndex)
{
    AnimChannel& channel = m_animation->clips[clipIndex].channels[m_channelIndex];
    if (channel.keyframes.isEmpty())
        channel.duration = 0.0f;
    else
        channel.duration = channel.keyframes.last().time;
}

void MoveKeyframeCommand::execute()
{
    if (m_channelIndex < 0 || m_keyframeIndex < 0)
        return;

    int clipIndex = -1;
    for (int ci = 0; ci < m_animation->clips.size(); ++ci)
    {
        if (m_animation->clips[ci].name == m_clipName)
        {
            clipIndex = ci;
            break;
        }
    }
    if (clipIndex < 0)
        return;

    AnimChannel& channel = m_animation->clips[clipIndex].channels[m_channelIndex];
    if (m_keyframeIndex >= channel.keyframes.size())
        return;

    // Remove from old position
    m_keyframe = channel.keyframes[m_keyframeIndex];
    channel.keyframes.remove(m_keyframeIndex);

    // Update time
    m_keyframe.time = m_newTime;

    // Re-insert at sorted position
    int insertPos = channel.keyframes.size();
    for (int i = 0; i < channel.keyframes.size(); ++i)
    {
        if (m_keyframe.time < channel.keyframes[i].time)
        {
            insertPos = i;
            break;
        }
    }
    channel.keyframes.insert(insertPos, m_keyframe);
    m_keyframeIndex = insertPos;

    updateChannelDuration(clipIndex);
}

void MoveKeyframeCommand::undo()
{
    if (m_channelIndex < 0 || m_keyframeIndex < 0)
        return;

    int clipIndex = -1;
    for (int ci = 0; ci < m_animation->clips.size(); ++ci)
    {
        if (m_animation->clips[ci].name == m_clipName)
        {
            clipIndex = ci;
            break;
        }
    }
    if (clipIndex < 0)
        return;

    AnimChannel& channel = m_animation->clips[clipIndex].channels[m_channelIndex];
    if (m_keyframeIndex >= channel.keyframes.size())
        return;

    // Remove from current position (at newTime)
    channel.keyframes.remove(m_keyframeIndex);

    // Restore old time
    m_keyframe.time = m_oldTime;

    // Re-insert at sorted position
    int insertPos = channel.keyframes.size();
    for (int i = 0; i < channel.keyframes.size(); ++i)
    {
        if (m_keyframe.time < channel.keyframes[i].time)
        {
            insertPos = i;
            break;
        }
    }
    channel.keyframes.insert(insertPos, m_keyframe);
    m_keyframeIndex = insertPos;

    updateChannelDuration(clipIndex);
}

QString MoveKeyframeCommand::name() const
{
    return "Move keyframe";
}
