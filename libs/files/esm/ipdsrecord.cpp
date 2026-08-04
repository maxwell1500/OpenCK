#include "ipdsrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

void IpdsRecord::initComponents()
{
    components.clear();
}

void IpdsRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
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

        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'IPDS':
            {
                const qint64 n = (esm.subLeft() - 1) / 4;
                esm.skip(1);
                impactFormIds.clear();
                for (qint64 i = 0; i < n; ++i)
                    impactFormIds.push_back(esm.readType<quint32>());
                break;
            }
            default:
            {
                RawSubRecord raw;
                raw.name = sub;
                esm.readRawSubData(raw.data);
                rawSubRecords.push_back(raw);
                break;
            }
        }
    }
}

void IpdsRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.startSubRecord('IPDS');
    const quint8 count = static_cast<quint8>(impactFormIds.size());
    esm.writeRawData(reinterpret_cast<const char*>(&count), 1);
    for (quint32 id : impactFormIds)
        esm.writeRawData(reinterpret_cast<const char*>(&id), 4);
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void IpdsRecord::blank()
{
    editorId.clear();
    formId = 0;
    impactFormIds.clear();
    rawSubRecords.clear();
    initComponents();
}
