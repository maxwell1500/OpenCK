#pragma once

#include <QString>
#include <QVector>
#include <QHash>

#include "obscriptparser.hpp"
#include "obscriptbinder.hpp"
#include "obscriptbytecode.hpp"

namespace ObScript
{

/// Compiles a parsed OBScript AST into stack-based bytecode.
/// Returns a BytecodeProgram with code + symbol table, or an error.
BytecodeProgram compile(const ParseResult& program);

} // namespace ObScript
