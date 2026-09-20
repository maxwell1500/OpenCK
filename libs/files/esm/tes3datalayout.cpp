#include "tes3datalayout.hpp"

#include <cstring>

namespace
{
const Tes3DataFieldDef kDialFields[] = {
    { "dialogType", 0, Tes3DataFieldType::U8, "Dialog type; matches the INFO parent rule. Observed 0-4." },
};

const Tes3DataFieldDef kInfoFields[] = {
    { "dialogType", 0, Tes3DataFieldType::U8, "Echoes the parent DIAL type byte (proven 1:1 over 23,693 INFOs)." },
    { "disposition", 4, Tes3DataFieldType::U8, "Required disposition 0-100; journal entries store the journal index here." },
    { "flags", 5, Tes3DataFieldType::U8, "Observed 0/1 only." },
    { "rank", 8, Tes3DataFieldType::U8, "Required faction rank 0-9; 255 means none." },
    { "gender", 9, Tes3DataFieldType::U8, "Observed 0, 1, 255." },
    { "pcRank", 10, Tes3DataFieldType::U8, "Required PC faction rank 0-9; 255 means none." },
};

const Tes3DataFieldDef kCellHeaderFields[] = {
    { "cellFlags", 0, Tes3DataFieldType::U32, "Cell flags bitmask." },
    { "gridX", 4, Tes3DataFieldType::I32, "Exterior grid X (-1 for interiors)." },
    { "gridY", 8, Tes3DataFieldType::I32, "Exterior grid Y (-1 for interiors)." },
};

// CELL-embedded placed reference: position + rotation in radians. Proven by
// exact FRMR pairing (2,538/2,538 cells) and float value ranges.
const Tes3DataFieldDef kCellRefFields[] = {
    { "posX", 0, Tes3DataFieldType::Float, "Reference position X." },
    { "posY", 4, Tes3DataFieldType::Float, "Reference position Y." },
    { "posZ", 8, Tes3DataFieldType::Float, "Reference position Z." },
    { "rotX", 12, Tes3DataFieldType::Float, "Reference rotation X (radians)." },
    { "rotY", 16, Tes3DataFieldType::Float, "Reference rotation Y (radians)." },
    { "rotZ", 20, Tes3DataFieldType::Float, "Reference rotation Z (radians)." },
};

const Tes3DataFieldDef kFlags32Fields[] = {
    { "flags", 0, Tes3DataFieldType::U32, "Observed small values only." },
};

const Tes3DataFieldDef kSoundGenFields[] = {
    { "type", 0, Tes3DataFieldType::U32, "Observed 0-7." },
};

const Tes3DataFieldDef kSoundFields[] = {
    { "volume", 0, Tes3DataFieldType::U8, "Volume 0-255 (observed 51-255)." },
    { "minRange", 1, Tes3DataFieldType::U8, "Minimum audible range." },
    { "maxRange", 2, Tes3DataFieldType::U8, "Maximum audible range." },
};

const Tes3DataFieldDef kTexturePathFields[] = {
    { "texturePath", 0, Tes3DataFieldType::String, "NUL-terminated texture file path." },
};

const Tes3DataLayout kLayouts[] = {
    { NAME('DIAL'), 1, kDialFields, 1, "DIAL dialog type byte." },
    { NAME('INFO'), 12, kInfoFields, 6, "INFO response requirements." },
    { NAME('CELL'), 12, kCellHeaderFields, 3, "CELL flags + exterior grid." },
    { NAME('CELL'), 24, kCellRefFields, 6, "CELL-embedded placed reference transform." },
    { NAME('LAND'), 4, kFlags32Fields, 1, "LAND flags (observed 1-7)." },
    { NAME('LEVC'), 4, kFlags32Fields, 1, "LEVC flags (observed 0/1)." },
    { NAME('LEVI'), 4, kFlags32Fields, 1, "LEVI flags (observed 0-3)." },
    { NAME('SNDG'), 4, kSoundGenFields, 1, "SNDG sound-generator type." },
    { NAME('SOUN'), 3, kSoundFields, 3, "SOUN volume + range." },
    { NAME('LTEX'), -1, kTexturePathFields, 1, "LTEX texture path string." },
};
}

const Tes3DataLayout* tes3DataLayoutFor(NAME code, int size)
{
    for (const Tes3DataLayout& layout : kLayouts)
    {
        if (layout.code != code)
            continue;
        if (layout.size == size || layout.size < 0)
            return &layout;
    }
    return nullptr;
}

namespace
{
int fieldWidth(Tes3DataFieldType type)
{
    switch (type)
    {
    case Tes3DataFieldType::U8: return 1;
    case Tes3DataFieldType::U16: return 2;
    case Tes3DataFieldType::U32: return 4;
    case Tes3DataFieldType::I32: return 4;
    case Tes3DataFieldType::Float: return 4;
    case Tes3DataFieldType::String: return 0;
    }
    return 0;
}
}

QVariant tes3DataDecodeField(const Tes3DataFieldDef& field, const QByteArray& payload)
{
    if (field.offset < 0 || field.offset > payload.size())
        return QVariant();
    const quint8* b = reinterpret_cast<const quint8*>(payload.constData()) + field.offset;
    const int left = payload.size() - field.offset;
    switch (field.type)
    {
    case Tes3DataFieldType::U8:
        if (left < 1)
            return QVariant();
        return QVariant(static_cast<quint32>(b[0]));
    case Tes3DataFieldType::U16:
    {
        if (left < 2)
            return QVariant();
        quint16 v;
        memcpy(&v, b, 2);
        return QVariant(static_cast<quint32>(v));
    }
    case Tes3DataFieldType::U32:
    {
        if (left < 4)
            return QVariant();
        quint32 v;
        memcpy(&v, b, 4);
        return QVariant(v);
    }
    case Tes3DataFieldType::I32:
    {
        if (left < 4)
            return QVariant();
        qint32 v;
        memcpy(&v, b, 4);
        return QVariant(v);
    }
    case Tes3DataFieldType::Float:
    {
        if (left < 4)
            return QVariant();
        quint32 bits;
        memcpy(&bits, b, 4);
        float v;
        memcpy(&v, &bits, 4);
        return QVariant(v);
    }
    case Tes3DataFieldType::String:
    {
        QString s = QString::fromUtf8(payload.constData() + field.offset, left);
        while (s.endsWith(QChar(0)))
            s.chop(1);
        return QVariant(s);
    }
    }
    return QVariant();
}

bool tes3DataEncodeField(const Tes3DataFieldDef& field, const QVariant& value, QByteArray& payload)
{
    if (field.offset < 0 || field.offset > payload.size())
        return false;
    quint8* b = reinterpret_cast<quint8*>(payload.data()) + field.offset;
    const int left = payload.size() - field.offset;
    const int width = fieldWidth(field.type);
    if (field.type != Tes3DataFieldType::String && left < width)
        return false;
    bool ok = false;
    switch (field.type)
    {
    case Tes3DataFieldType::U8:
    {
        const quint64 v = value.toULongLong(&ok);
        if (!ok || v > 0xFFULL)
            return false;
        b[0] = static_cast<quint8>(v);
        return true;
    }
    case Tes3DataFieldType::U16:
    {
        const quint64 v = value.toULongLong(&ok);
        if (!ok || v > 0xFFFFULL)
            return false;
        const quint16 w = static_cast<quint16>(v);
        memcpy(b, &w, 2);
        return true;
    }
    case Tes3DataFieldType::U32:
    {
        const quint64 v = value.toULongLong(&ok);
        if (!ok || v > 0xFFFFFFFFULL)
            return false;
        const quint32 w = static_cast<quint32>(v);
        memcpy(b, &w, 4);
        return true;
    }
    case Tes3DataFieldType::I32:
    {
        const qint64 v = value.toLongLong(&ok);
        if (!ok || v < -2147483648LL || v > 2147483647LL)
            return false;
        const qint32 w = static_cast<qint32>(v);
        memcpy(b, &w, 4);
        return true;
    }
    case Tes3DataFieldType::Float:
    {
        const double v = value.toDouble(&ok);
        if (!ok)
            return false;
        const float w = static_cast<float>(v);
        quint32 bits;
        memcpy(&bits, &w, 4);
        memcpy(b, &bits, 4);
        return true;
    }
    case Tes3DataFieldType::String:
        return false; // strings are whole-payload; see Tes3Data_Component
    }
    return false;
}
