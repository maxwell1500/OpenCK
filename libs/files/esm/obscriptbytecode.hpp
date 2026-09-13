#pragma once

#include <QVector>
#include <QString>

// Simple stack-based bytecode for OBScript. The compiler emits these
// opcodes from the AST. A future VM (or interpreter) can execute them.

namespace ObScript
{

enum class Opcode : quint8
{
    // Control flow
    Nop,
    Halt,
    Jmp,          // u32 = target instruction index
    Jz,           // jump if top of stack is falsy
    Jnz,          // jump if top of stack is truthy

    // Arithmetic
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Neg,          // unary minus
    Not,          // logical not (!)
    BitNot,       // bitwise not (~)

    // Comparison
    Eq,
    Neq,
    Lt,
    Leq,
    Gt,
    Geq,

    // Bitwise
    BitAnd,
    BitOr,

    // Logical
    And,
    Or,

    // Stack operations
    PushInt,
    PushFloat,
    PushString,
    PushBool,
    PushNil,
    Pop,
    Dup,

    // Variables (u32 = index into symbolNames)
    LoadVar,
    StoreVar,

    // Functions
    Call,         // u32 = arg count
    Ret,
    RetVal,
    End
};

struct Instr
{
    Opcode op = Opcode::Nop;
    qint64 intValue = 0;      // PushInt / PushBool
    double floatValue = 0.0;  // PushFloat
    QString stringValue;      // PushString
    quint32 operand = 0;      // Jmp target / LoadVar index / Call arg count
};

struct BytecodeProgram
{
    QVector<Instr> code;
    QVector<QString> symbolNames;  // for LoadVar/StoreVar
    bool ok = false;
    QString error;
    int errorLine = 0;
};

} // namespace ObScript
