#ifndef MATERIALCOMPILER_HPP
#define MATERIALCOMPILER_HPP

#include <QString>
#include <QStringList>
#include <QMap>

struct MaterialRuleTemplate;

// The result of compiling a PBR material against a rule template.
struct MaterialCompileReport
{
    bool ok = false;
    QString templateName;
    QStringList appliedOperations;      // slot-affecting template rules applied
    QMap<QString, QString> resolvedSlots;   // slot -> absolute texture path (found on disk)
    QStringList finalSlots;             // texture slots after the template is applied
    QStringList missingSlots;           // required slots with no texture assigned
    QStringList unresolvedTextures;     // assigned slots whose file was not found
    QStringList warnings;

    // One-line human summary.
    QString summary() const;
};

// Compiles a texture-slot map against a rule template: applies the
// template's slot rules, then resolves each slot's texture against the
// game texture root (or absolute paths) and reports what a compiled
// .mat must satisfy.
class MaterialCompiler
{
public:
    static MaterialCompileReport compile(const MaterialRuleTemplate& tpl,
                                         const QMap<QString, QString>& slotTextures,
                                         const QString& textureRoot);
};

#endif // MATERIALCOMPILER_HPP
