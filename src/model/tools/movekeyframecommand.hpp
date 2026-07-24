#ifndef MOVEKEYFRAMECOMMAND_HPP
#define MOVEKEYFRAMECOMMAND_HPP

#include "command.hpp"
#include "../../libs/files/nifanim/nifanimation.hpp"

#include <QString>

class MoveKeyframeCommand : public Command
{
public:
    MoveKeyframeCommand(NifAnimation* animation, const QString& clipName,
                        const QString& boneName, float oldTime, float newTime,
                        const QString& description = QString());

    void execute() override;
    void undo() override;
    QString name() const override;

private:
    NifAnimation* m_animation;
    QString m_clipName;
    QString m_boneName;
    float m_oldTime;
    float m_newTime;

    AnimKeyframe m_keyframe;
    int m_channelIndex;
    int m_keyframeIndex;

    bool findKeyframe(float time);
    void updateChannelDuration(int clipIndex);
};

#endif // MOVEKEYFRAMECOMMAND_HPP
