#include "revbrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

void RevbRecord::initComponents()
{
    components.clear();
}

void RevbRecord::load(ESMReader& esm, bool)
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
                if (esm.subLeft() >= 16)
                {
                    esm.skip(static_cast<int>(esm.recLeft()) - 4);
                    flags = esm.readType<quint32>();
                }
                else
                {
                    RawSubRecord raw;
                    raw.name = sub;
                    esm.readRawSubData(raw.data);
                    rawSubRecords.push_back(raw);
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

void RevbRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.startSubRecord('DATA');
    // 12 float reverb params + flags.
    static const float zero[12] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    esm.writeRawData(reinterpret_cast<const char*>(zero), sizeof(zero));
    esm.writeRawData(reinterpret_cast<const char*>(&flags), 4);
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void RevbRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    rawSubRecords.clear();
    initComponents();
}
