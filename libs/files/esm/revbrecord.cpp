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
                // Keep the whole reverb payload for a byte-exact round-trip
                // and surface the leading flags field for the editor.
                esm.readRawSubData(data);
                if (data.size() >= 4)
                {
                    flags = static_cast<quint8>(data[0])
                        | (static_cast<quint8>(data[1]) << 8)
                        | (static_cast<quint8>(data[2]) << 16)
                        | (static_cast<quint8>(data[3]) << 24);
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

    if (!data.isEmpty())
    {
        esm.startSubRecord(static_cast<NAME>('DATA'));
        esm.writeRawData(data.constData(), data.size());
        esm.endSubRecord();
    }
    else if (flags != 0)
    {
        // Freshly created records persist their flags in a 40-byte DATA
        // subrecord matching the real layout.
        esm.startSubRecord(static_cast<NAME>('DATA'));
        esm.writeType<quint32>(flags);
        for (int i = 0; i < 9; ++i)
            esm.writeType<quint32>(0);
        esm.endSubRecord();
    }

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
    data.clear();
    rawSubRecords.clear();
    initComponents();
}
