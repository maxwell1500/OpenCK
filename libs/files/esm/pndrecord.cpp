#include "pndrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace {
bool isTypedName(NAME sub)
{
    switch (sub)
    {
    case 'EDID': case 'FNAM': case 'ANAM': case 'TEMP':
    case 'DENS': case 'PHLA': case 'RSCS':
        return true;
    default:
        return false;
    }
}

QString trimmedZString(const QString& s)
{
    QString t = s;
    while (!t.isEmpty() && t.at(t.size() - 1) < QChar(0x20))
        t.chop(1);
    return t;
}
}

void PndRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;

        mOrder.append(sub);
        if (isTypedName(sub))
        {
            switch (sub)
            {
            case 'EDID': editorId = trimmedZString(esm.readZString()); break;
            case 'FNAM': flags = esm.readType<quint32>(); break;
            case 'ANAM': starSystem = trimmedZString(esm.readZString()); break;
            case 'TEMP': temperature = esm.readType<float>(); break;
            case 'DENS': density = esm.readType<float>(); break;
            case 'PHLA': phase = esm.readType<float>(); break;
            case 'RSCS': resources = esm.readType<quint32>(); break;
            }
        }
        else
        {
            RawSubRecord raw;
            raw.name = sub;
            esm.readRawSubData(raw.data);
            rawSubRecords.push_back(raw);
        }
    }
}

void PndRecord::save(ESMWriter& esm) const
{
    int rawIdx = 0;
    for (quint32 sub : mOrder)
    {
        switch (sub)
        {
        case 'EDID': esm.writeSubZString('EDID', editorId); break;
        case 'FNAM': esm.writeSubData<quint32>('FNAM', flags); break;
        case 'ANAM': esm.writeSubZString('ANAM', starSystem); break;
        case 'TEMP': esm.writeSubData<float>('TEMP', temperature); break;
        case 'DENS': esm.writeSubData<float>('DENS', density); break;
        case 'PHLA': esm.writeSubData<float>('PHLA', phase); break;
        case 'RSCS': esm.writeSubData<quint32>('RSCS', resources); break;
        default:
            if (rawIdx < rawSubRecords.size())
            {
                const RawSubRecord& raw = rawSubRecords[rawIdx++];
                esm.startSubRecord(raw.name);
                esm.writeRawData(raw.data.data(), raw.data.size());
                esm.endSubRecord();
            }
            break;
        }
    }
}

void PndRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    starSystem.clear();
    temperature = 0.0f;
    density = 1.0f;
    phase = 1.0f;
    resources = 0;
    rawSubRecords.clear();
    mOrder.clear();
    components.clear();
}
