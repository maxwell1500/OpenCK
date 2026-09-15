#include "pndrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <QHash>

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
    // Only the FIRST FNAM is the u32 flags field; later FNAM occurrences
    // are different (longer) structs and must ride along as raw payloads.
    // Reading them all as flags clobbers `flags` with the last occurrence
    // and truncates the longer structs on save.
    bool seenFlags = false;
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;

        mOrder.append(sub);
        if (isTypedName(sub) && (sub != NAME('FNAM') || !seenFlags))
        {
            switch (sub)
            {
            case 'EDID': editorId = trimmedZString(esm.readZString()); break;
            case 'FNAM': flags = esm.readType<quint32>(); seenFlags = true; break;
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
    // Raw cursor by subrecord name: repeated FNAMs after the flags field
    // are stored as raw payloads and must replay at their own positions.
    QHash<NAME, QVector<int>> rawByName;
    for (int i = 0; i < rawSubRecords.size(); ++i)
        rawByName[rawSubRecords[i].name].append(i);
    QHash<NAME, int> rawCursor;
    bool wroteFlags = false;

    auto writeRaw = [&](NAME name) {
        const QVector<int>& idx = rawByName[name];
        int& cur = rawCursor[name];
        if (cur < idx.size())
            esm.writeRawSubRecord(rawSubRecords[idx[cur++]]);
    };

    for (quint32 sub : mOrder)
    {
        switch (sub)
        {
        case 'EDID': esm.writeSubZString('EDID', editorId); break;
        case 'FNAM':
            if (!wroteFlags)
            {
                esm.writeSubData<quint32>('FNAM', flags);
                wroteFlags = true;
            }
            else
                writeRaw(sub);
            break;
        case 'ANAM': esm.writeSubZString('ANAM', starSystem); break;
        case 'TEMP': esm.writeSubData<float>('TEMP', temperature); break;
        case 'DENS': esm.writeSubData<float>('DENS', density); break;
        case 'PHLA': esm.writeSubData<float>('PHLA', phase); break;
        case 'RSCS': esm.writeSubData<quint32>('RSCS', resources); break;
        default:
            writeRaw(sub);
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
