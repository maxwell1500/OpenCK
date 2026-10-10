#include "materialcompiler.hpp"
#include "materialruletemplate.hpp"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include "../../files/log/logger.hpp"

MaterialCompileReport MaterialCompiler::compile(const MaterialRuleTemplate& tpl,
                                                const QMap<QString, QString>& slotTextures,
                                                const QString& textureRoot)
{
    MaterialCompileReport report;
    report.templateName = tpl.name;

    const MaterialRuleTemplate::ApplyMapResult applied = tpl.applyToSlotMap(slotTextures);
    report.appliedOperations = applied.operations;
    report.finalSlots = applied.slotPaths.keys();

    const QStringList required = tpl.requiredSlots();
    for (const QString& slot : required)
    {
        if (!applied.slotPaths.contains(slot))
            report.missingSlots.append(slot);
    }

    const QString root = textureRoot;
    for (const QString& slot : applied.slotPaths.keys())
    {
        const QString path = applied.slotPaths.value(slot);
        if (path.isEmpty())
        {
            report.unresolvedTextures.append(slot);
            continue;
        }

        QFileInfo fi(path);
        QString abs;
        if (fi.isAbsolute())
        {
            abs = fi.absoluteFilePath();
        }
        else if (!root.isEmpty())
        {
            abs = QDir(root).absoluteFilePath(path);
        }
        else
        {
            abs = path;
        }

        if (QFile::exists(abs))
            report.resolvedSlots.insert(slot, abs);
        else
        {
            report.unresolvedTextures.append(slot);
            report.warnings << QStringLiteral("Texture not found for slot '%1': %2")
                .arg(slot, abs);
        }
    }

    report.ok = report.missingSlots.isEmpty() && report.unresolvedTextures.isEmpty();
    if (!report.ok)
        LOG_WARNING(QString("MaterialCompiler: %1").arg(report.summary()));
    else
        LOG_DEBUG(QString("MaterialCompiler: compiled '%1' with %2 slots (template %3)")
            .arg(tpl.name).arg(report.finalSlots.size()).arg(tpl.name));
    return report;
}

QString MaterialCompileReport::summary() const
{
    if (ok)
    {
        return QStringLiteral("Compiled with template '%1': %2 texture slots resolved")
            .arg(templateName, QString::number(resolvedSlots.size()));
    }

    QStringList parts;
    if (!missingSlots.isEmpty())
        parts << QStringLiteral("missing slots: %1").arg(missingSlots.join(QStringLiteral(", ")));
    if (!unresolvedTextures.isEmpty())
        parts << QStringLiteral("unresolved textures: %1").arg(unresolvedTextures.join(QStringLiteral(", ")));
    return QStringLiteral("Compile failed (%1)").arg(parts.join(QStringLiteral("; ")));
}
