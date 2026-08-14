#include "debrrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

#include <cstring>

void DebrRecord::initComponents()
{
    components.clear();
}

void DebrRecord::load(ESMReader& esm, bool)
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
            case 'DATA':
            {
                // u32 count, then per entry: char[256] model path + u32 count
                // + u16 scale + u16 flags.
                QByteArray data;
                esm.readRawSubData(data);
                const quint32 n = data.size() >= 4
                    ? *reinterpret_cast<const quint32*>(data.constData()) : 0;
                debris.clear();
                int off = 4;
                for (quint32 i = 0; i < n && off + 256 + 8 <= data.size(); ++i)
                {
                    DebrisEntry e;
                    const char* model = data.constData() + off;
                    e.modelPath = QString::fromLatin1(model, strnlen(model, 256));
                    off += 256;
                    e.count = *reinterpret_cast<const quint32*>(data.constData() + off); off += 4;
                    e.scale = *reinterpret_cast<const quint16*>(data.constData() + off); off += 2;
                    e.flags = *reinterpret_cast<const quint16*>(data.constData() + off); off += 2;
                    debris.push_back(e);
                }
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

void DebrRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    QByteArray data;
    const quint32 n = static_cast<quint32>(debris.size());
    data.append(reinterpret_cast<const char*>(&n), 4);
    for (const DebrisEntry& e : debris)
    {
        QByteArray path = e.modelPath.toLatin1();
        data.append(path.constData(), qMin<int>(256, path.size()));
        data.resize(data.size() + (256 - qMin<int>(256, path.size())));
        data.append(reinterpret_cast<const char*>(&e.count), 4);
        data.append(reinterpret_cast<const char*>(&e.scale), 2);
        data.append(reinterpret_cast<const char*>(&e.flags), 2);
    }
    esm.startSubRecord('DATA');
    esm.writeRawData(data.constData(), data.size());
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
    }
}

void DebrRecord::blank()
{
    editorId.clear();
    formId = 0;
    debris.clear();
    rawSubRecords.clear();
    initComponents();
}
