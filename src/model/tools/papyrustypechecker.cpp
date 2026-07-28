#include "papyrustypechecker.hpp"

#include <QRegularExpression>
#include <QSet>

bool PapyrusTypeChecker::TypeInfo::isBuiltin() const
{
    return name == "Int" || name == "Float" || name == "Bool" || name == "String";
}

PapyrusTypeChecker::PapyrusTypeChecker()
{
    registerScriptType("ObjectReference");
    registerScriptType("Actor");
    registerScriptType("Form");
    registerScriptType("Quest");
    registerScriptType("ActorBase");
    registerScriptType("FormList");
    registerScriptType("Weapon");
    registerScriptType("Armor");
    registerScriptType("Spell");
    registerScriptType("EffectShader");
    registerScriptType("Keyword");
    registerScriptType("AlchemyItem");
    registerScriptType("Ingredient");
    registerScriptType("MiscItem");
    registerScriptType("Book");
    registerScriptType("Note");
    registerScriptType("Apparatus");
    registerScriptType("Ammo");
    registerScriptType("Food");
    registerScriptType("Key");
    registerScriptType("Tool");
    registerScriptType("SoulGem");
    registerScriptType("ScrollItem");
    registerScriptType("Enchantment");
    registerScriptType("MagicEffect");
    registerScriptType("Activator");
    registerScriptType("Tree");
    registerScriptType("Flora");
    registerScriptType("Projectile");
    registerScriptType("Cell");
    registerScriptType("WorldSpace");
    registerScriptType("Package");
}

void PapyrusTypeChecker::setScriptName(const QString& name)
{
    m_scriptName = name;
    registerScriptType(name);
}

void PapyrusTypeChecker::declareVariable(const QString& name, const TypeInfo& type, int line)
{
    if (name.isEmpty()) return;
    if (m_symbols.contains(name)) {
        addError(line, QString("Variable '%1' is already declared").arg(name));
        return;
    }
    m_symbols.insert(name, type);
}

void PapyrusTypeChecker::declareFunction(const FunctionSignature& sig)
{
    m_functions.insert(sig.name, sig);
}

void PapyrusTypeChecker::registerScriptType(const QString& name)
{
    m_scriptTypes.insert(name);
}

bool PapyrusTypeChecker::hasVariable(const QString& name) const
{
    return m_symbols.contains(name);
}

PapyrusTypeChecker::TypeInfo PapyrusTypeChecker::variableType(const QString& name) const
{
    return m_symbols.value(name);
}

bool PapyrusTypeChecker::hasFunction(const QString& name) const
{
    return m_functions.contains(name);
}

bool PapyrusTypeChecker::isAssignableFrom(const TypeInfo& target, const TypeInfo& source) const
{
    if (!target.isValid() || !source.isValid()) return true;
    if (target == source) return true;
    if (target.name == "Float" && source.name == "Int") return true;
    if (target.isArray != source.isArray) return false;
    if (target.isArray && target.name != source.name) return false;
    if (target.name == "Form" && m_scriptTypes.contains(source.name)) return true;
    if (m_scriptTypes.contains(target.name) && source.name == "None") return true;
    if (target.name == "ObjectReference" && source.name == "Actor") return true;
    if (source.name == "None" && (target.isBuiltin() == false)) return true;
    return false;
}

bool PapyrusTypeChecker::compatible(const TypeInfo& target, const TypeInfo& source) const
{
    return isAssignableFrom(target, source);
}

bool PapyrusTypeChecker::checkAssignment(const QString& varName, const TypeInfo& valueType, int line)
{
    if (!hasVariable(varName)) {
        addError(line, QString("Assignment to undeclared variable '%1'").arg(varName));
        return false;
    }
    TypeInfo varType = variableType(varName);
    if (!isAssignableFrom(varType, valueType)) {
        addError(line, QString("Cannot assign %1%2 to variable '%3' of type %4%5")
                     .arg(valueType.isArray ? "Array of " : "")
                     .arg(valueType.name)
                     .arg(varName)
                     .arg(varType.isArray ? "Array of " : "")
                     .arg(varType.name));
        return false;
    }
    return true;
}

bool PapyrusTypeChecker::checkFunctionCall(const QString& funcName, const QVector<TypeInfo>& argTypes, int line)
{
    if (!hasFunction(funcName)) {
        return true;
    }
    const FunctionSignature& sig = m_functions[funcName];
    if (sig.paramTypes.size() != argTypes.size()) {
        addError(line, QString("Function '%1' expects %2 argument(s) but got %3")
                      .arg(funcName)
                      .arg(sig.paramTypes.size())
                      .arg(argTypes.size()));
        return false;
    }
    bool ok = true;
    for (int i = 0; i < argTypes.size(); ++i) {
        if (!isAssignableFrom(sig.paramTypes[i], argTypes[i])) {
            addError(line, QString("Argument %1 of '%2': cannot pass %3%4 as %5%6")
                          .arg(i + 1)
                          .arg(funcName)
                          .arg(argTypes[i].isArray ? "Array of " : "")
                          .arg(argTypes[i].name)
                          .arg(sig.paramTypes[i].isArray ? "Array of " : "")
                          .arg(sig.paramTypes[i].name));
            ok = false;
        }
    }
    return ok;
}

bool PapyrusTypeChecker::checkConditional(const TypeInfo& exprType, int line, const QString& keyword)
{
    if (!exprType.isValid()) return true;
    if (exprType.isArray) {
        addError(line, QString("%1 condition requires Bool, got Array of %2").arg(keyword).arg(exprType.name));
        return false;
    }
    if (!exprType.isBool()) {
        addError(line, QString("%1 condition requires Bool, got %2").arg(keyword).arg(exprType.name));
        return false;
    }
    return true;
}

bool PapyrusTypeChecker::checkArrayIndex(const TypeInfo& arrayType, int line)
{
    if (!arrayType.isArray) {
        addError(line, QString("Cannot index into non-array type %1").arg(arrayType.name));
        return false;
    }
    return true;
}

bool PapyrusTypeChecker::checkArrayElement(const TypeInfo& arrayType, const TypeInfo& elementType, int line)
{
    if (!arrayType.isArray) {
        addError(line, QString("Cannot append to non-array type %1").arg(arrayType.name));
        return false;
    }
    TypeInfo inner{arrayType.name, false, arrayType.isScriptType};
    if (!isAssignableFrom(inner, elementType)) {
        addError(line, QString("Cannot add %1%2 to array of %3")
                      .arg(elementType.isArray ? "Array of " : "")
                      .arg(elementType.name)
                      .arg(arrayType.name));
        return false;
    }
    return true;
}

PapyrusTypeChecker::TypeInfo PapyrusTypeChecker::elementTypeOf(const TypeInfo& arrayType) const
{
    if (!arrayType.isArray) return TypeInfo{};
    return TypeInfo{arrayType.name, false, arrayType.isScriptType};
}

PapyrusTypeChecker::TypeInfo PapyrusTypeChecker::inferLiteralType(const QString& literal) const
{
    QString t = literal.trimmed();
    if (t.isEmpty()) return TypeInfo{};
    if (t == "True" || t == "False") return TypeInfo{"Bool", false, false};
    if (t == "None") return TypeInfo{"None", false, false};
    if (t.startsWith("\"") || t.startsWith("'")) return TypeInfo{"String", false, false};
    static QRegularExpression intRe(R"(^-?\d+$)");
    if (intRe.match(t).hasMatch()) return TypeInfo{"Int", false, false};
    static QRegularExpression floatRe(R"(^-?\d+\.\d+f?$)");
    if (floatRe.match(t).hasMatch()) return TypeInfo{"Float", false, false};
    if (t.startsWith("[") || t.endsWith("]")) {
        return TypeInfo{"Unknown", true, false};
    }
    if (hasVariable(t)) return variableType(t);
    return TypeInfo{};
}

PapyrusTypeChecker::TypeInfo PapyrusTypeChecker::inferExpressionType(const QString& expr) const
{
    QString t = expr.trimmed();
    if (t.isEmpty()) return TypeInfo{};

    static QRegularExpression newCallRe(R"(^new\s+(\w+)\s*\[)");
    auto m = newCallRe.match(t);
    if (m.hasMatch()) {
        return TypeInfo{m.captured(1), true, !isBuiltinType(m.captured(1))};
    }

    static QRegularExpression callRe(R"(^(\w+)\s*\()");
    m = callRe.match(t);
    if (m.hasMatch() && hasFunction(m.captured(1))) {
        return m_functions[m.captured(1)].returnType;
    }

    static QRegularExpression memberRe(R"(^(\w+)\.(\w+)\s*\()");
    m = memberRe.match(t);
    if (m.hasMatch()) return TypeInfo{};

    static QRegularExpression indexRe(R"(^(\w+)\[)");
    m = indexRe.match(t);
    if (m.hasMatch() && hasVariable(m.captured(1))) {
        return elementTypeOf(variableType(m.captured(1)));
    }

    if (t.contains("+") || t.contains("-") || t.contains("*") || t.contains("/")) {
        bool anyFloat = false;
        bool anyString = false;
        QStringList parts = t.split(QRegularExpression(R"([+\-*/])"), Qt::SkipEmptyParts);
        for (const QString& p : parts) {
            TypeInfo ti = inferLiteralType(p);
            if (ti.name == "Float") anyFloat = true;
            if (ti.name == "String") anyString = true;
        }
        if (anyString) return TypeInfo{"String", false, false};
        return TypeInfo{anyFloat ? "Float" : "Int", false, false};
    }

    if (t.contains("==") || t.contains("!=") || t.contains("<=") || t.contains(">=") ||
        t.contains("<") || t.contains(">") || t.contains("&&") || t.contains("||") ||
        t.contains("!")) {
        return TypeInfo{"Bool", false, false};
    }

    return inferLiteralType(t);
}

QVector<QString> PapyrusTypeChecker::getErrors() const
{
    QVector<QString> out;
    for (const auto& e : m_errors) out.append(e.message);
    return out;
}

void PapyrusTypeChecker::addError(int line, const QString& message)
{
    m_errors.append({line, message});
}

void PapyrusTypeChecker::clear()
{
    m_symbols.clear();
    m_functions.clear();
    m_errors.clear();
    m_scriptName.clear();
}

bool PapyrusTypeChecker::isBuiltinType(const QString& name)
{
    return name == "Int" || name == "Float" || name == "Bool" || name == "String" ||
           name == "Var";
}

bool PapyrusTypeChecker::isBuiltinArrayType(const QString& name)
{
    return name == "Int[]" || name == "Float[]" || name == "Bool[]" || name == "String[]";
}

PapyrusTypeChecker::TypeInfo PapyrusTypeChecker::parseType(const QString& typeText)
{
    QString t = typeText.trimmed();
    bool isArray = false;
    if (t.endsWith("[]")) {
        isArray = true;
        t.chop(2);
        t = t.trimmed();
    }
    bool isScript = !isBuiltinType(t);
    return TypeInfo{t, isArray, isScript};
}