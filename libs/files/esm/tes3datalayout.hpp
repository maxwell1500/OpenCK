#ifndef TES3DATALAYOUT_H
#define TES3DATALAYOUT_H

// Type-specific DATA-subrecord layouts for Morrowind (TES3) records.
//
// Every layout below is grounded in a survey of the real Morrowind.esm
// (test_tes3data: per-type size histogram, per-lane value ranges, INFO/DIAL
// parent correlation, CELL FRMR pairing): the table keys (record code, DATA
// size) cover every fixed-size DATA group observed, and the conformance test
// asserts decode + re-encode round-trips all 348k DATA payloads byte-exactly.
// Groups without a proven layout (PGRD counts, variable *DT structs, ...)
// have no entry and stay on the generic hex path.

#include "common.hpp"

#include <QString>
#include <QByteArray>
#include <QVariant>

enum class Tes3DataFieldType
{
    U8,
    U16,
    U32,
    I32,
    Float,
    String
};

struct Tes3DataFieldDef
{
    const char* name;
    int offset;
    Tes3DataFieldType type;
    const char* tooltip;
};

struct Tes3DataLayout
{
    NAME code;
    int size; // exact DATA payload size, or -1 for variable-length strings
    const Tes3DataFieldDef* fields;
    int fieldCount;
    const char* note;
};

// Layout for this record code + DATA size, or nullptr when the group has no
// proven typed layout (caller keeps the hex fallback).
const Tes3DataLayout* tes3DataLayoutFor(NAME code, int size);

// Decodes one field from a DATA payload. Returns an invalid QVariant when
// the field runs past the payload end. Floats are read as u32 bits + memcpy.
QVariant tes3DataDecodeField(const Tes3DataFieldDef& field, const QByteArray& payload);

// Encodes one field value into a DATA payload at the field offset (little
// endian, floats via memcpy). Returns false when the value does not fit the
// field width or the field runs past the payload end.
bool tes3DataEncodeField(const Tes3DataFieldDef& field, const QVariant& value, QByteArray& payload);

#endif
