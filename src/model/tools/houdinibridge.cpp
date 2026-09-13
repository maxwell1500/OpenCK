#include "houdinibridge.hpp"

QStringList HoudiniBridge::interactiveCommand() const
{
    QStringList args;
    args << houdiniExecutable;
    if (!hipFile.trimmed().isEmpty())
        args << hipFile;
    args << extraArgs;
    return args;
}

QStringList HoudiniBridge::batchCommand(const QString& script,
                                        const QStringList& scriptArgs) const
{
    QStringList args;
    args << houdiniExecutable;
    args << script;
    args << extraArgs;
    args << scriptArgs;
    return args;
}

QString HoudiniBridge::defaultBatchExecutable()
{
    // hython is Houdini's Python interpreter used for headless batch work.
    return QStringLiteral("hython.exe");
}
