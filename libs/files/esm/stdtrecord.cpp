#include "stdtrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

namespace {

QString lenString(const QByteArray& data, int& offset)
{
    const quint32 len = baseFormU32(data, offset);
    offset += 4;
    if (offset + static_cast<int>(len) > data.size())
    {
        offset = data.size();
        return QString();
    }
    const QString s = QString::fromLatin1(
        data.constData() + offset, static_cast<int>(len));
    offset += static_cast<int>(len);
    return s;
}

QString trimmed(QString s)
{
    while (!s.isEmpty() && s.at(s.size() - 1) < QChar(0x20))
        s.chop(1);
    return s;
}

} // namespace

void StdtRecord::initComponents()
{
    components.clear();
}

void StdtRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;

        bool handled = false;
        for (auto& c : components.all())
        {
            if (c->canHandle(sub))
            {
                c->handleSubrecord(sub, esm);
                handled = true;
                break;
            }
        }
        if (handled) continue;

        if (sub == NAME('EDID'))
        {
            editorId = esm.readZString();
            continue;
        }

        // Everything else — including the BFCB/BFCE component markers — is
        // preserved verbatim in order so the record round-trips byte-exactly.
        RawSubRecord raw;
        raw.name = sub;
        esm.readRawSubData(raw.data);
        rawSubRecords.push_back(raw);
    }
}

void StdtRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);

    for (const auto& raw : rawSubRecords)
        esm.writeRawSubRecord(raw);
}

void StdtRecord::blank()
{
    editorId.clear();
    formId = 0;
    rawSubRecords.clear();
    initComponents();
}

const RawSubRecord* StdtRecord::findSubrecord(NAME name) const
{
    for (const RawSubRecord& raw : rawSubRecords)
        if (raw.name == name)
            return &raw;
    return nullptr;
}

QString StdtRecord::starName() const
{
    const RawSubRecord* r = findSubrecord(NAME('ANAM'));
    return r ? trimmed(QString::fromLatin1(r->data)) : QString();
}

bool StdtRecord::hasParsecLocation() const
{
    const RawSubRecord* r = findSubrecord(NAME('BNAM'));
    return r && r->data.size() >= 12;
}

void StdtRecord::parsecLocation(float& x, float& y, float& z) const
{
    x = y = z = 0.0f;
    const RawSubRecord* r = findSubrecord(NAME('BNAM'));
    if (!r || r->data.size() < 12)
        return;
    x = baseFormF32(r->data, 0);
    y = baseFormF32(r->data, 4);
    z = baseFormF32(r->data, 8);
}

quint32 StdtRecord::systemId() const
{
    const RawSubRecord* r = findSubrecord(NAME('DNAM'));
    return r ? baseFormU32(r->data, 0) : 0;
}

QByteArray StdtRecord::color() const
{
    const RawSubRecord* r = findSubrecord(NAME('ENAM'));
    return r ? r->data : QByteArray();
}

quint32 StdtRecord::binaryStar() const
{
    const RawSubRecord* r = findSubrecord(NAME('SNAM'));
    return r ? baseFormU32(r->data, 0) : 0;
}

quint32 StdtRecord::sunPreset() const
{
    const RawSubRecord* r = findSubrecord(NAME('PNAM'));
    return r ? baseFormU32(r->data, 0) : 0;
}

StarDataComponent StdtRecord::starData() const
{
    StarDataComponent data;

    const QVector<BaseFormComponent> comps = splitBaseFormComponents(rawSubRecords);
    const BaseFormComponent* c =
        findBaseFormComponent(comps, QStringLiteral("BGSStarDataComponent_Component"));
    if (!c)
        return data;
    const RawSubRecord* r = c->findSubrecord(NAME('DATA'));
    if (!r)
        return data;

    int offset = 0;
    data.catalogueId = lenString(r->data, offset);
    data.spectralClass = lenString(r->data, offset);
    data.magnitude = baseFormF32(r->data, offset); offset += 4;
    data.massSolarMasses = baseFormF32(r->data, offset); offset += 4;
    data.innerHabitableZone = baseFormF32(r->data, offset); offset += 4;
    data.outerHabitableZone = baseFormF32(r->data, offset); offset += 4;
    data.hip = baseFormU32(r->data, offset); offset += 4;
    data.radius = baseFormU32(r->data, offset); offset += 4;
    data.temperatureK = baseFormU32(r->data, offset); offset += 4;

    data.valid = offset <= r->data.size();
    return data;
}
