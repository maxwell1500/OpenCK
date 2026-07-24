#include "addkeyframecommand.hpp"

AddKeyframeCommand::AddKeyframeCommand(NifAnimation* animation, const QString& clipName,
                                       const QString& boneName, const AnimKeyframe& keyframe,
                                       const QString& description)
    : m_animation(animation),
      m_clipName(clipName),
      m_boneName(boneName),
      m_keyframe(keyframe),
      m_clipIndex(-1),
      m_channelIndex(-1),
      m_insertIndex(-1),
      m_channelCreated(false)
{
    findOrCreateChannel();
    m_insertIndex = findInsertPosition();
}

bool AddKeyframeCommand::findOrCreateChannel()
{
    m_clipIndex = -1;
    for (int ci = 0; ci < m_animation->clips.size(); ++ci)
    {
        if (m_animation->clips[ci].name == m_clipName)
        {
            m_clipIndex = ci;
            break;
        }
    }
    if (m_clipIndex < 0)
        return false;

    AnimClip& clip = m_animation->clips[m_clipIndex];
    m_channelIndex = -1;
    for (int chi = 0; chi < clip.channels.size(); ++chi)
    {
        if (clip.channels[chi].boneName == m_boneName)
        {
            m_channelIndex = chi;
            break;
        }
    }

    if (m_channelIndex < 0)
    {
        AnimChannel newChannel;
        newChannel.boneName = m_boneName;
        newChannel.type = "Transform";
        newChannel.duration = 0.0f;
        clip.channels.append(newChannel);
        m_channelIndex = clip.channels.size() - 1;
        m_channelCreated = true;
    }

    return true;
}

int AddKeyframeCommand::findInsertPosition() const
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

void AddKeyframeCommand::updateChannelDuration()
{
    if (m_clipIndex < 0 || m_channelIndex < 0)
        return;

    AnimChannel& channel = m_animation->clips[m_clipIndex].channels[m_channelIndex];
    if (channel.keyframes.isEmpty())
        channel.duration = 0.0f;
    else
        channel.duration = channel.keyframes.last().time;
}

void AddKeyframeCommand::execute()
{
    if (m_clipIndex < 0 || m_channelIndex < 0)
        return;

    AnimChannel& channel = m_animation->clips[m_clipIndex].channels[m_channelIndex];
    if (m_insertIndex < 0 || m_insertIndex > channel.keyframes.size())
        return;

    channel.keyframes.insert(m_insertIndex, m_keyframe);
    updateChannelDuration();
}

void AddKeyframeCommand::undo()
{
    if (m_clipIndex < 0 || m_channelIndex < 0)
        return;

    AnimChannel& channel = m_animation->clips[m_clipIndex].channels[m_channelIndex];
    if (m_insertIndex >= channel.keyframes.size())
        return;

    channel.keyframes.remove(m_insertIndex);
    updateChannelDuration();

    if (m_channelCreated && channel.keyframes.isEmpty()) {
        m_animation->clips[m_clipIndex].channels.remove(m_channelIndex);
        m_channelIndex = -1;
    }
}

QString AddKeyframeCommand::name() const
{
    return "Add keyframe";
}
