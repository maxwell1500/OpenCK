#ifndef REMOVEKEYFRAMECOMMAND_HPP
#define REMOVEKEYFRAMECOMMAND_HPP

#include "command.hpp"
#include "../../libs/files/nifanim/nifanimation.hpp"

#include <QString>

class RemoveKeyframeCommand : public Command
{
public:
    RemoveKeyframeCommand(NifAnimation* animation, const QString& clipName,
                          const QString& boneName, float time,
                          const QString& description = QString());

    void execute() override;
    void undo() override;
    QString name() const override;

private:
    NifAnimation* m_animation;
    QString m_clipName;
    QString m_boneName;
    float m_time;
    AnimKeyframe m_keyframe;
    int m_clipIndex;
    int m_channelIndex;
    int m_keyframeIndex;

    bool findKeyframe();
    int findInsertPosition() const;
    void updateChannelDuration();
};

#endif // REMOVEKEYFRAMECOMMAND_HPP
