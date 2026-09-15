#include "baseformcomponents.hpp"

#include <cstring>

quint32 baseFormU32(const QByteArray& data, int offset)
{
    if (offset < 0 || offset + 4 > data.size())
        return 0;
    const uchar* p = reinterpret_cast<const uchar*>(data.constData()) + offset;
    return static_cast<quint32>(p[0])
        | (static_cast<quint32>(p[1]) << 8)
        | (static_cast<quint32>(p[2]) << 16)
        | (static_cast<quint32>(p[3]) << 24);
}

float baseFormF32(const QByteArray& data, int offset)
{
    const quint32 bits = baseFormU32(data, offset);
    float f = 0.0f;
    static_assert(sizeof(float) == 4, "float must be 32-bit");
    std::memcpy(&f, &bits, sizeof(f));
    return f;
}

namespace {

QString trimmedZString(const QByteArray& data)
{
    QString t = QString::fromLatin1(data);
    while (!t.isEmpty() && t.at(t.size() - 1) < QChar(0x20))
        t.chop(1);
    return t;
}

} // namespace

bool BaseFormComponent::hasSubrecord(NAME name) const
{
    return findSubrecord(name) != nullptr;
}

const RawSubRecord* BaseFormComponent::findSubrecord(NAME name) const
{
    for (const RawSubRecord& r : subrecords)
        if (r.name == name)
            return &r;
    return nullptr;
}

quint32 BaseFormComponent::firstU32(NAME name) const
{
    const RawSubRecord* r = findSubrecord(name);
    return r ? baseFormU32(r->data, 0) : 0;
}

QVector<quint32> BaseFormComponent::u32List(NAME name) const
{
    // Collect across every occurrence: some lists are one subrecord holding N
    // values, others appear once per entry. Both flatten to the same list.
    QVector<quint32> out;
    for (const RawSubRecord& r : subrecords)
    {
        if (r.name != name)
            continue;
        for (int off = 0; off + 4 <= r.data.size(); off += 4)
            out.append(baseFormU32(r.data, off));
    }
    return out;
}

float BaseFormComponent::firstFloat(NAME name) const
{
    const RawSubRecord* r = findSubrecord(name);
    return r ? baseFormF32(r->data, 0) : 0.0f;
}

QVector<BaseFormComponent> splitBaseFormComponents(
    const QVector<RawSubRecord>& subrecords,
    QVector<RawSubRecord>* leading,
    QVector<RawSubRecord>* trailing)
{
    QVector<BaseFormComponent> pieces;
    if (leading)
        leading->clear();
    if (trailing)
        trailing->clear();

    BaseFormComponent current;
    bool inComponent = false;
    bool seenComponent = false;

    for (const RawSubRecord& raw : subrecords)
    {
        if (raw.name == NAME('BFCB'))
        {
            if (inComponent)
                pieces.append(current);
            current = BaseFormComponent();
            current.typeName = trimmedZString(raw.data);
            inComponent = true;
            seenComponent = true;
        }
        else if (raw.name == NAME('BFCE'))
        {
            if (inComponent)
            {
                pieces.append(current);
                current = BaseFormComponent();
                inComponent = false;
            }
        }
        else if (inComponent)
        {
            current.subrecords.append(raw);
        }
        else if (!seenComponent && leading)
        {
            leading->append(raw);
        }
        else if (seenComponent && trailing)
        {
            trailing->append(raw);
        }
    }

    if (inComponent)
        pieces.append(current);

    return pieces;
}

const BaseFormComponent* findBaseFormComponent(
    const QVector<BaseFormComponent>& components, const QString& typeName)
{
    for (const BaseFormComponent& c : components)
        if (c.typeName == typeName)
            return &c;
    return nullptr;
}

bool parseVolumePayload(const QByteArray& data, QVector<VolumeEntry>& out)
{
    out.clear();
    if (data.size() < 4)
        return false;

    int offset = 0;
    const quint32 count = baseFormU32(data, offset);
    offset += 4;

    out.reserve(static_cast<int>(count));
    for (quint32 i = 0; i < count; ++i)
    {
        VolumeEntry entry;
        entry.type = baseFormU32(data, offset);
        offset += 4;

        // type + 16 matrix floats + 3 floats, then the type-specific tail.
        int extraCount = 0;
        if (entry.type == 1)
            extraCount = 1;
        else if (entry.type == 3)
            extraCount = 2;
        else if (entry.type == 5)
            extraCount = 3;
        const int entrySize = 4 + VolumeEntry::kMatrixFloats * 4 + 3 * 4
            + extraCount * 4;

        if (offset + entrySize - 4 > data.size())
            return false;   // truncated: reject rather than emit a partial entry

        for (int f = 0; f < VolumeEntry::kMatrixFloats; ++f)
        {
            entry.matrix[f] = baseFormF32(data, offset);
            offset += 4;
        }
        entry.a = baseFormF32(data, offset); offset += 4;
        entry.b = baseFormF32(data, offset); offset += 4;
        entry.c = baseFormF32(data, offset); offset += 4;
        for (int e = 0; e < extraCount; ++e)
        {
            entry.extra.append(baseFormF32(data, offset));
            offset += 4;
        }
        out.append(entry);
    }

    // Every shipped VLMS consumes exactly its subrecord; a leftover tail means
    // the layout assumption is wrong.
    return offset == data.size();
}
