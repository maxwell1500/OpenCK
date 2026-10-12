#ifndef SETKEYFRAMEPROPERTIESCOMMAND_HPP
#define SETKEYFRAMEPROPERTIESCOMMAND_HPP

#include "command.hpp"
#include "../../libs/files/nifanim/nifanimation.hpp"

#include <QString>

class SetKeyframePropertiesCommand : public Command
{
public:
    SetKeyframePropertiesCommand(NifAnimation* animation, const QString& clipName,
                                 const QString& boneName, float time,
                                 const AnimKeyframe& oldProperties,
                                 const AnimKeyframe& newProperties,
                                 const QString& description = QString());

    void execute() override;
    void undo() override;
    QString name() const override;

private:
    NifAnimation* m_animation;
    QString m_clipName;
    QString m_boneName;
    float m_time;
    AnimKeyframe m_oldProperties;
    AnimKeyframe m_newProperties;

    void applyProperties(const AnimKeyframe& kf);
};

#endif // SETKEYFRAMEPROPERTIESCOMMAND_HPP
