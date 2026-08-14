#include "lvlistrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void LvliRecord::load(ESMReader& esm, bool)
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
            case 'LVLF':
            {
                if (esm.subLeft() >= 4)
                {
                    levelFlags = static_cast<quint8>(esm.readType<quint32>());
                    levelFlagsSize = 4;
                    if (esm.subLeft() > 0)
                        esm.skip(static_cast<int>(esm.subLeft()));
                    handled = true;
                }
                else if (esm.subLeft() >= 1)
                {
                    levelFlags = esm.readType<quint8>();
                    levelFlagsSize = 1;
                    if (esm.subLeft() > 0)
                        esm.skip(static_cast<int>(esm.subLeft()));
                    handled = true;
                }
                break;
            }
            case 'LVLO':
            {
                if (esm.subLeft() >= 8)
                {
                    LvloEntry entry;
                    entry.level = esm.readType<qint16>();
                    entry.formId = esm.readType<quint32>();
                    entry.count = esm.readType<qint16>();
                    entries.append(entry);
                    if (esm.subLeft() > 0)
                        esm.skip(static_cast<int>(esm.subLeft()));
                    handled = true;
                }
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

void LvliRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint8>('LVLD', chanceNone);
    if (levelFlagsSize >= 4)
        esm.writeSubData<quint32>('LVLF', levelFlags);
    else
        esm.writeSubData<quint8>('LVLF', levelFlags);

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

void LvliRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    chanceNone = 0;
    levelFlags = 0;
    levelFlagsSize = 1;
    entries.clear();
    rawSubRecords.clear();
    components.clear();
}
