#ifndef ADDKEYFRAMECOMMAND_HPP
#define ADDKEYFRAMECOMMAND_HPP

#include "command.hpp"
#include "../../libs/files/nifanim/nifanimation.hpp"

#include <QString>

class AddKeyframeCommand : public Command
{
public:
    AddKeyframeCommand(NifAnimation* animation, const QString& clipName,
                       const QString& boneName, const AnimKeyframe& keyframe,
                       const QString& description = QString());

    void execute() override;
    void undo() override;
    QString name() const override;

private:
    NifAnimation* m_animation;
    QString m_clipName;
    QString m_boneName;
    AnimKeyframe m_keyframe;
    int m_clipIndex;
    int m_channelIndex;
    int m_insertIndex;
    bool m_channelCreated;

    bool findOrCreateChannel();
    int findInsertPosition() const;
    void updateChannelDuration();
};

#endif // ADDKEYFRAMECOMMAND_HPP
