#include "setkeyframepropertiescommand.hpp"
#include "../../libs/files/nifanim/nifanimationwriter.hpp"

SetKeyframePropertiesCommand::SetKeyframePropertiesCommand(
    NifAnimation* animation, const QString& clipName,
    const QString& boneName, float time,
    const AnimKeyframe& oldProperties,
    const AnimKeyframe& newProperties,
    const QString& description)
    : m_animation(animation),
      m_clipName(clipName),
      m_boneName(boneName),
      m_time(time),
      m_oldProperties(oldProperties),
      m_newProperties(newProperties)
{
    Q_UNUSED(description)
}

void SetKeyframePropertiesCommand::applyProperties(const AnimKeyframe& kf)
{
    if (!m_animation)
        return;

    for (int ci = 0; ci < m_animation->clips.size(); ++ci)
    {
        if (m_animation->clips[ci].name != m_clipName)
            continue;

        for (auto& ch : m_animation->clips[ci].channels)
        {
            if (ch.boneName != m_boneName)
                continue;

            NifAnimationWriter::channelSetKeyframeValue(ch, m_time, kf);
            return;
        }
    }
}

void SetKeyframePropertiesCommand::execute()
{
    applyProperties(m_newProperties);
}

void SetKeyframePropertiesCommand::undo()
{
    applyProperties(m_oldProperties);
}

QString SetKeyframePropertiesCommand::name() const
{
    return "Edit keyframe properties";
}
