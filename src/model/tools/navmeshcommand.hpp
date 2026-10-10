#ifndef NAVMESHCOMMAND_HPP
#define NAVMESHCOMMAND_HPP

#include "command.hpp"
#include "../../view/window/navmesheditordialog.hpp"
#include <QString>

class NavmeshEditCommand : public Command
{
public:
    NavmeshEditCommand(NavMeshData* target, const NavMeshData& before, const NavMeshData& after, const QString& name = QStringLiteral("Edit Navmesh"))
        : mTarget(target), mBefore(before), mAfter(after), mName(name)
    {
    }

    void execute() override
    {
        if (mTarget) {
            *mTarget = mAfter;
        }
    }

    void undo() override
    {
        if (mTarget) {
            *mTarget = mBefore;
        }
    }

    QString name() const override
    {
        return mName;
    }

private:
    NavMeshData* mTarget;
    NavMeshData mBefore;
    NavMeshData mAfter;
    QString mName;
};

#endif // NAVMESHCOMMAND_HPP
