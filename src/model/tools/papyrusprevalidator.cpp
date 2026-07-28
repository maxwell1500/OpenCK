#include "papyrusprevalidator.hpp"
#include <QRegularExpression>
#include <QDebug>

static QString stripComment(const QString& line)
{
    int idx = line.indexOf("//");
    if (idx >= 0) return line.left(idx);
    return line;
}

PapyrusTypeChecker PapyrusPreValidator::buildTypeChecker(const QString& scriptContent)
{
    PapyrusTypeChecker tc;
    QStringList lines = scriptContent.split('\n');

    QRegularExpression scriptNameRe(R"(ScriptName\s+(\w+))");
    auto sm = scriptNameRe.match(scriptContent);
    if (sm.hasMatch()) tc.setScriptName(sm.captured(1));

    static const QStringList builtinTypes = {"Bool", "Int", "Float", "String", "Var"};

    for (int i = 0; i < lines.size(); ++i) {
        QString line = stripComment(lines[i]).trimmed();
        if (line.isEmpty()) continue;

        if (line.startsWith("Variable ") || line.startsWith("Variable\t")) {
            QRegularExpression vdRe(R"(^\s*Variable\s+(\w+)\s*:\s*(\w+(?:\[\])?))");
            auto m = vdRe.match(line);
            if (m.hasMatch()) {
                tc.declareVariable(m.captured(1),
                                   PapyrusTypeChecker::parseType(m.captured(2)), i + 1);
            }
            continue;
        }

        if (line.startsWith("Property ") || line.startsWith("Property\t")) {
            QRegularExpression propRe(R"(^\s*Property\s+(\w+)\s+(\w+)(?:\[\])?\s+.*)");
            auto m = propRe.match(line);
            if (m.hasMatch()) {
                PapyrusTypeChecker::TypeInfo ti{m.captured(1), false,
                                                !builtinTypes.contains(m.captured(1))};
                tc.declareVariable(m.captured(2), ti, i + 1);
            }
            continue;
        }

        if (line.contains("Function ") || line.contains("Event ")) {
            QRegularExpression fnRe(R"((?:Function|Event)\s+(\w+)\s*\(([^)]*)\))");
            auto m = fnRe.match(line);
            if (m.hasMatch()) {
                QString fname = m.captured(1);
                QString args = m.captured(2).trimmed();
                PapyrusTypeChecker::FunctionSignature sig;
                sig.name = fname;
                sig.line = i + 1;
                if (!args.isEmpty()) {
                    QStringList argList = args.split(',', Qt::SkipEmptyParts);
                    for (const QString& a : argList) {
                        QString at = a.trimmed();
                        QRegularExpression argRe(R"((\w+(?:\[\])?)\s+\w+)");
                        auto am = argRe.match(at);
                        if (am.hasMatch()) {
                            sig.paramTypes.append(PapyrusTypeChecker::parseType(am.captured(1)));
                        }
                    }
                }
                QRegularExpression retRe(R"((\w+(?:\[\])?)\s+Function\s+\w+)");
                auto rm = retRe.match(line);
                if (rm.hasMatch()) {
                    sig.returnType = PapyrusTypeChecker::parseType(rm.captured(1));
                }
                tc.declareFunction(sig);
            }
            continue;
        }

        QRegularExpression plainDeclRe(
            R"(^\s*(Bool|Int|Float|String|Var|\w+)\s+(\w+)\s*(?:=\s*(.*?))?$)");
        auto pm = plainDeclRe.match(line);
        if (pm.hasMatch()) {
            QString typeStr = pm.captured(1);
            QString name = pm.captured(2);
            if (typeStr != "If" && typeStr != "While" && typeStr != "For" &&
                typeStr != "Return" && typeStr != "Else" && typeStr != "ElseIf" &&
                !name.isEmpty() && !line.contains('(')) {
                PapyrusTypeChecker::TypeInfo ti = PapyrusTypeChecker::parseType(typeStr);
                if (ti.name != "EndIf" && ti.name != "EndWhile" && ti.name != "EndFor") {
                    tc.declareVariable(name, ti, i + 1);
                    if (!pm.captured(3).isEmpty()) {
                        tc.checkAssignment(name, tc.inferLiteralType(pm.captured(3)), i + 1);
                    }
                }
            }
        }
    }

    return tc;
}

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

    PapyrusTypeChecker tc = buildTypeChecker(scriptContent);
    QStringList lines = scriptContent.split('\n');

    static const QStringList controlKeywords = {
        "If", "While", "For", "Return", "Else", "ElseIf"
    };

    for (int i = 0; i < lines.size(); ++i) {
        QString raw = lines[i];
        QString line = stripComment(raw).trimmed();
        if (line.isEmpty()) continue;

        if (line.startsWith("If ", Qt::CaseInsensitive) ||
            line.startsWith("ElseIf ", Qt::CaseInsensitive) ||
            line.startsWith("While ", Qt::CaseInsensitive)) {
            QString keyword = line.startsWith("ElseIf", Qt::CaseInsensitive) ? "ElseIf"
                              : line.startsWith("While", Qt::CaseInsensitive) ? "While" : "If";
            int sp = line.indexOf(' ');
            QString cond = line.mid(sp + 1).trimmed();
            PapyrusTypeChecker::TypeInfo condType = tc.inferExpressionType(cond);
            if (condType.isValid()) {
                tc.checkConditional(condType, i + 1, keyword);
            }
        }

        QRegularExpression assignRe(R"(^(\w+)\s*=\s*(.+?)(?:\s*(?:;|$))?$)");
        auto am = assignRe.match(line);
        if (am.hasMatch()) {
            QString varName = am.captured(1);
            QString valueExpr = am.captured(2).trimmed();
            if (tc.hasVariable(varName)) {
                PapyrusTypeChecker::TypeInfo vt = tc.inferExpressionType(valueExpr);
                if (vt.isValid()) {
                    tc.checkAssignment(varName, vt, i + 1);
                }
            }
        }

        QRegularExpression callRe(R"((\w+)\s*\(([^)]*)\))");
        auto cmIter = callRe.globalMatch(line);
        while (cmIter.hasNext()) {
            auto cm = cmIter.next();
            QString fname = cm.captured(1);
            QString argsStr = cm.captured(2).trimmed();
            if (tc.hasFunction(fname) && !argsStr.isEmpty()) {
                QStringList args = argsStr.split(',', Qt::SkipEmptyParts);
                QVector<PapyrusTypeChecker::TypeInfo> argTypes;
                for (const QString& a : args) {
                    argTypes.append(tc.inferExpressionType(a.trimmed()));
                }
                tc.checkFunctionCall(fname, argTypes, i + 1);
            }
        }

        QRegularExpression indexRe(R"((\w+)\s*\[)");
        auto im = indexRe.match(line);
        if (im.hasMatch()) {
            QString varName = im.captured(1);
            if (tc.hasVariable(varName)) {
                tc.checkArrayIndex(tc.variableType(varName), i + 1);
            }
        }
    }

    const auto& symbols = tc.symbols();
    QSet<QString> usedVars;
    for (int i = 0; i < lines.size(); ++i) {
        QString line = stripComment(lines[i]);
        for (auto it = symbols.begin(); it != symbols.end(); ++it) {
            if (line.contains(QRegularExpression(
                    QString(R"(\b%1\b)").arg(QRegularExpression::escape(it.key()))))) {
                QRegularExpression declRe(
                    QString(R"(^\s*(?:Variable|Property|Const|Global)\s+%1\b)")
                        .arg(QRegularExpression::escape(it.key())));
                if (!declRe.match(line.trimmed()).hasMatch()) {
                    usedVars.insert(it.key());
                }
            }
        }
    }

    QVector<QString> unusedVars;
    for (auto it = symbols.begin(); it != symbols.end(); ++it) {
        if (!usedVars.contains(it.key())) {
            unusedVars.append(it.key());
        }
    }

    if (!unusedVars.isEmpty()) {
        errors.append({PapyrusValidationError::Type::UnusedVariable, 1,
                       QString("Unused variables: %1").arg(unusedVars.join(", ")),
                       scriptContent});
    }

    for (const auto& te : tc.typedErrors()) {
        PapyrusValidationError::Type t = PapyrusValidationError::Type::TypeMismatch;
        errors.append({t, te.line <= 0 ? 1 : te.line, te.message, lines.value(te.line - 1)});
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

