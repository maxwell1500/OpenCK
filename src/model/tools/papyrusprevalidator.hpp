#ifndef PAPYRUSPREVALIDATOR_HPP
#define PAPYRUSPREVALIDATOR_HPP

#include <QString>
#include <QVector>
#include <QPair>

#include "papyrustypechecker.hpp"

struct PapyrusValidationError
{
    enum class Type {
        SyntaxError,
        StructureError,
        TypeMismatch,
        UndefinedVariable,
        UnusedVariable,
        InvalidStatement
    };
    
    Type type;
    int line;
    QString message;
    QString codeSnippet;
};

class PapyrusPreValidator
{
public:
    static QVector<PapyrusValidationError> validate(const QString& scriptContent);
    static PapyrusTypeChecker buildTypeChecker(const QString& scriptContent);
    
private:
    static QVector<PapyrusValidationError> checkSyntax(const QString& scriptContent);
    static QVector<PapyrusValidationError> checkStructure(const QString& scriptContent);
    static QVector<PapyrusValidationError> checkTypes(const QString& scriptContent);
    
    static bool isKeyword(const QString& word);
    static bool isType(const QString& word);
    static bool isStatementStart(const QString& word);
    static bool isStatementEnd(const QString& word);
};

#endif // PAPYRUSPREVALIDATOR_HPP
