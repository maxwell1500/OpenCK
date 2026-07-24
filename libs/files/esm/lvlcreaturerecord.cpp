#include "lvlcreaturerecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void LvlcRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();

    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;

        bool handled = false;
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); handled = true; break;
            case 'LVLD': chanceNone = esm.readType<quint8>(); handled = true; break;
            case 'LVLF': listFlags = esm.readType<quint8>(); handled = true; break;
            case 'DATA': listFlags = esm.readType<quint8>(); handled = true; break;
            case 'LVLO':
            {
                LvloEntry entry;
                entry.level = esm.readType<qint16>();
                entry.formId = esm.readType<quint32>();
                entry.count = esm.readType<qint16>();
                entries.append(entry);
                handled = true;
                break;
            }
            default: break;
        }
        if (handled) continue;

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

        RawSubRecord raw;
        raw.name = sub;
        esm.readRawSubData(raw.data);
        rawSubRecords.push_back(raw);
    }
}

void LvlcRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint8>('LVLD', chanceNone);
    esm.writeSubData<quint8>('DATA', listFlags);

    for (const auto& entry : entries)
    {
        esm.startSubRecord('LVLO');
        esm.writeType<qint16>(entry.level);
        esm.writeType<quint32>(entry.formId);
        esm.writeType<qint16>(entry.count);
        esm.endSubRecord();
    }

    components.saveAll(esm);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void LvlcRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    chanceNone = 0;
    listFlags = 0;
    entries.clear();
    rawSubRecords.clear();
    components.clear();
}
