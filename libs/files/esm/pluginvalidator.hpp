#ifndef PLUGINVALIDATOR_H
#define PLUGINVALIDATOR_H

#include <QString>
#include <QList>
#include <QVector>

class PluginValidator
{
public:
    enum class Severity { Info, Warning, Error };
    
    struct Issue
    {
        QString message;
        Severity severity;
        QString category; // "Masters", "FormIDs", "Records", "Structure"
    };
    
    struct PluginReport
    {
        QString pluginName;
        QVector<Issue> issues;
        int totalRecords;
        int modifiedRecords;
        int missingMasters;
    };
    
    static PluginReport validate(const QString& pluginPath, const QStringList& availablePlugins);
    static QVector<Issue> getMissingMasters(const QString& pluginPath, const QStringList& availablePlugins);
    static QVector<Issue> checkFormIdRanges(const QString& pluginPath);
    static QVector<Issue> checkDuplicateEditorIds(const QString& pluginPath);

private:
    static Severity issueToSeverity(int code);
    static QString severityToString(Severity severity);
};

#endif // PLUGINVALIDATOR_H
