#include "papyrusprevalidator.hpp"
#include <QRegularExpression>
#include <QDebug>

QVector<PapyrusValidationError> PapyrusPreValidator::validate(const QString& scriptContent)
{
    QVector<PapyrusValidationError> errors;
    
    errors.append(checkSyntax(scriptContent));
    errors.append(checkStructure(scriptContent));
    errors.append(checkTypes(scriptContent));
    
    return errors;
}

QVector<PapyrusValidationError> PapyrusPreValidator::checkSyntax(const QString& scriptContent)
{
    QVector<PapyrusValidationError> errors;
    
    int line = 1;
    int column = 1;
    
    for (int i = 0; i < scriptContent.length(); ++i) {
        QChar c = scriptContent[i];
        
        if (c == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        
        if (c == '{' || c == '}') {
            errors.append({
                PapyrusValidationError::Type::SyntaxError,
                line, 
                QString("Unmatched brace: %1").arg(c),
                scriptContent.mid(0, i + 1)
            });
        }
        
        if (c == '(' || c == ')') {
            errors.append({
                PapyrusValidationError::Type::SyntaxError,
                line,
                QString("Unmatched parenthesis: %1").arg(c),
                scriptContent.mid(0, i + 1)
            });
        }
        
        if (c == '"' || c == '\'') {
            errors.append({
                PapyrusValidationError::Type::SyntaxError,
                line,
                QString("Unmatched quote: %1").arg(c),
                scriptContent.mid(0, i + 1)
            });
        }
    }
    
    return errors;
}

QVector<PapyrusValidationError> PapyrusPreValidator::checkStructure(const QString& scriptContent)
{
    QVector<PapyrusValidationError> errors;
    
    QRegularExpression ifRegex(R"(^[\s]*If\s+)", QRegularExpression::MultilineOption);
    QRegularExpression elseRegex(R"(^[\s]*Else(?:If)?\s+)", QRegularExpression::MultilineOption);
    QRegularExpression whileRegex(R"(^[\s]*While\s+)", QRegularExpression::MultilineOption);
    QRegularExpression forRegex(R"(^[\s]*For\s+)", QRegularExpression::MultilineOption);
    
    QVector<QString> ifBlocks;
    QVector<QString> elseBlocks;
    QVector<QString> whileBlocks;
    QVector<QString> forBlocks;
    
    QStringList lines = scriptContent.split('\n');
    
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i];
        
        if (ifRegex.match(line).hasMatch()) {
            ifBlocks.append(QString("If"));
        }
        
        if (elseRegex.match(line).hasMatch() && !line.contains("ElseIf")) {
            elseBlocks.append(QString("Else"));
        }

        if (whileRegex.match(line).hasMatch() || forRegex.match(line).hasMatch()) {
            QString blockType = whileRegex.match(line).hasMatch() ? "While" : "For";
            whileBlocks.append(blockType);
            forBlocks.append(blockType);
        }
    }
    
    if (ifBlocks.size() != elseBlocks.size()) {
        errors.append({
            PapyrusValidationError::Type::StructureError,
            1,
            QString("Mismatched If/Else blocks: %1 Ifs, %1 Elses").arg(ifBlocks.size()),
            scriptContent
        });
    }
    
    if (whileBlocks.size() != forBlocks.size()) {
        errors.append({
            PapyrusValidationError::Type::StructureError,
            1,
            QString("Mismatched While/For blocks: %1 Whiles, %1 Fors").arg(whileBlocks.size()),
            scriptContent
        });
    }
    
    return errors;
}

QVector<PapyrusValidationError> PapyrusPreValidator::checkTypes(const QString& scriptContent)
{
    QVector<PapyrusValidationError> errors;
    
    QStringList lines = scriptContent.split('\n');
    
    QRegularExpression varRegex(R"(^[\s]*Variable\s+(\w+)\s+:\s+(\w+))", QRegularExpression::MultilineOption);
    QRegularExpression constRegex(R"(^[\s]*Const\s+(\w+)\s+:\s+(\w+))", QRegularExpression::MultilineOption);
    
    QVector<QString> declaredVars;
    QVector<QString> usedVars;
    
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i];
        
        QRegularExpressionMatch match = varRegex.match(line);
        if (match.hasMatch()) {
            QString varName = match.captured(1);
            QString varType = match.captured(2);
            declaredVars.append(varName);
        }
        
        match = constRegex.match(line);
        if (match.hasMatch()) {
            QString varName = match.captured(1);
            QString varType = match.captured(2);
            declaredVars.append(varName);
        }
        
        for (const QString& var : declaredVars) {
            if (line.contains(QString("%1 ").arg(var)) && !var.startsWith("m_")) {
                usedVars.append(var);
            }
        }
    }
    
    QVector<QString> unusedVars;
    for (const QString& var : declaredVars) {
        if (!usedVars.contains(var)) {
            unusedVars.append(var);
        }
    }
    
    if (!unusedVars.isEmpty()) {
        errors.append({
            PapyrusValidationError::Type::UnusedVariable,
            1,
            QString("Unused variables: %1").arg(unusedVars.join(", ")),
            scriptContent
        });
    }
    
    return errors;
}

bool PapyrusPreValidator::isKeyword(const QString& word)
{
    static const QStringList keywords = {
        "ScriptName", "EndScript", "Function", "EndFunction",
        "Event", "EndEvent", "Property", "EndProperty", "Variable", "EndVariable",
        "Bool", "Int", "Float", "String", "Form",
        "If", "Else", "ElseIf", "EndIf",
        "While", "EndWhile", "For", "EndFor",
        "Return", "True", "False", "None"
    };
    
    return keywords.contains(word);
}

bool PapyrusPreValidator::isType(const QString& word)
{
    static const QStringList types = {
        "Form", "FormList", "ObjectReference", "Actor", "Quest", "Package",
        "Spell", "EffectShader", "Keyword", "ActorBase", "Weapon", "Armor",
        "AlchemyItem", "Ingredient", "MiscItem", "Book", "Note", "Apparatus",
        "Ammo", "Food", "Key", "Tool", "SoulGem", "ScrollItem",
        "Enchantment", "MagicEffect", "Activator", "Tree", "Flora", "Projectile",
        "Shader", "Texture", "Sound", "Music", "AmbientSound", "Water", "Light",
        "Cell", "WorldSpace", "InteriorCell", "ExteriorCell", "NavMesh"
    };
    
    return types.contains(word);
}

bool PapyrusPreValidator::isStatementStart(const QString& word)
{
    static const QStringList statements = {
        "If", "While", "For", "Return", "Variable", "Const", "Property"
    };
    
    return statements.contains(word);
}

bool PapyrusPreValidator::isStatementEnd(const QString& word)
{
    static const QStringList ends = {
        "EndIf", "EndWhile", "EndFor", "EndFunction", "EndEvent",
        "EndProperty", "EndVariable", "EndScript"
    };
    
    return ends.contains(word);
}

