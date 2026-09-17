#include "lvlistrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "subrecordreplay.hpp"

namespace
{
quint32 leU32(const QByteArray& b, int offset)
{
    quint32 v = 0;
    const int n = qMin(4, b.size() - offset);
    for (int i = 0; i < n; ++i)
        v |= quint32(static_cast<quint8>(b.at(offset + i))) << (8 * i);
    return v;
}
}

void LvliRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    loadOrder.clear();
    lvldRaw.clear();
    lvlfRaw.clear();
    lvloRaws.clear();

    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;
        loadOrder.append(sub);

        bool handled = false;
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); handled = true; break;
            case 'LVLD':
                esm.readRawSubData(lvldRaw);
                chanceNone = lvldRaw.isEmpty() ? 0 : static_cast<quint8>(lvldRaw.at(0));
                hasLvld = true;
                handled = true;
                break;
            case 'LVLF':
                esm.readRawSubData(lvlfRaw);
                levelFlags = lvlfRaw.isEmpty() ? 0 : static_cast<quint8>(lvlfRaw.at(0));
                levelFlagsSize = static_cast<quint32>(lvlfRaw.size());
                hasLvlf = true;
                handled = true;
                break;
            case 'LVLO':
            {
                QByteArray bytes;
                esm.readRawSubData(bytes);
                lvloRaws.append(bytes);
                if (bytes.size() >= 8)
                {
                    LvloEntry entry;
                    entry.level = static_cast<qint16>(leU32(bytes, 0) & 0xFFFF);
                    entry.formId = leU32(bytes, 2);
                    entry.count = static_cast<qint16>(leU32(bytes, 6) & 0xFFFF);
                    entries.append(entry);
                }
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

void LvliRecord::save(ESMWriter& esm) const
{
    SubrecordReplay replay;
    replay.init(rawSubRecords);

    const auto writeLvlf = [&]()
    {
        if (levelFlagsSize >= 4)
            esm.writeSubData<quint32>('LVLF', levelFlags);
        else
            esm.writeSubData<quint8>('LVLF', levelFlags);
    };
    const auto writeLvlo = [&](const LvloEntry& entry)
    {
        esm.startSubRecord('LVLO');
        esm.writeType<qint16>(entry.level);
        esm.writeType<quint32>(entry.formId);
        esm.writeType<qint16>(entry.count);
        esm.endSubRecord();
    };

    bool wroteEdid = false, wroteLvld = false, wroteLvlf = false;
    int lvloIdx = 0;
    for (NAME sub : loadOrder)
    {
        switch (sub)
        {
            case 'EDID':
                if (!wroteEdid) { esm.writeSubZString('EDID', editorId); wroteEdid = true; }
                break;
            case 'LVLD':
                if (!wroteLvld && (hasLvld || chanceNone != 0))
                {
                    if (!lvldRaw.isEmpty())
                        esm.writeRawSubRecord(RawSubRecord{ NAME('LVLD'), lvldRaw });
                    else
                        esm.writeSubData<quint8>('LVLD', chanceNone);
                    wroteLvld = true;
                }
                break;
            case 'LVLF':
                if (!wroteLvlf && (hasLvlf || levelFlags != 0))
                {
                    if (!lvlfRaw.isEmpty())
                        esm.writeRawSubRecord(RawSubRecord{ NAME('LVLF'), lvlfRaw });
                    else
                        writeLvlf();
                    wroteLvlf = true;
                }
                break;
            case 'LVLO':
                if (lvloIdx < entries.size())
                {
                    if (lvloIdx < lvloRaws.size() && !lvloRaws[lvloIdx].isEmpty())
                        esm.writeRawSubRecord(RawSubRecord{ NAME('LVLO'), lvloRaws[lvloIdx] });
                    else
                        writeLvlo(entries[lvloIdx]);
                }
                ++lvloIdx;
                break;
            default:
                if (!components.writeSubrecord(sub, esm))
                    replay.write(sub, esm);
                break;
        }
    }

    if (!wroteEdid && !editorId.isEmpty())
        esm.writeSubZString('EDID', editorId);
    if (!wroteLvld && (hasLvld || chanceNone != 0))
    {
        if (!lvldRaw.isEmpty())
            esm.writeRawSubRecord(RawSubRecord{ NAME('LVLD'), lvldRaw });
        else
            esm.writeSubData<quint8>('LVLD', chanceNone);
    }
    if (!wroteLvlf && (hasLvlf || levelFlags != 0))
    {
        if (!lvlfRaw.isEmpty())
            esm.writeRawSubRecord(RawSubRecord{ NAME('LVLF'), lvlfRaw });
        else
            writeLvlf();
    }

    if (lvloIdx < entries.size())
    {
        // Synthetic records or dropped positions: emit the remaining entries.
        for (int i = lvloIdx; i < entries.size(); ++i)
            writeLvlo(entries[i]);
    }
    else if (lvloRaws.size() > static_cast<int>(entries.size()))
    {
        for (int i = entries.size(); i < lvloRaws.size(); ++i)
            esm.writeRawSubRecord(RawSubRecord{ NAME('LVLO'), lvloRaws[i] });
    }

    replay.writeLeftover(esm);
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
    loadOrder.clear();
    hasLvld = false;
    hasLvlf = false;
    lvldRaw.clear();
    lvlfRaw.clear();
    lvloRaws.clear();
    components.clear();
}
