#include "shourecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

void ShouRecord::initComponents()
{
    components.clear();
}

void ShouRecord::load(ESMReader& esm, bool)
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
            case 'FULL': fullName = esm.readZString(); break;
            case 'SNAM':
            {
                // Skyrim writes one SNAM per shout word (12 bytes each).
                // Append per subrecord; cleared in blank().
                const quint32 n = static_cast<quint32>(esm.subLeft()) / 12;
                for (quint32 i = 0; i < n; ++i)
                {
                    ShoutWord w;
                    w.wordFormId = esm.readType<quint32>();
                    w.spellFormId = esm.readType<quint32>();
                    w.recoveryTime = esm.readType<float>();
                    words.push_back(w);
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

void ShouRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    if (!fullName.isEmpty())
        esm.writeSubZString('FULL', fullName);
    for (const ShoutWord& w : words)
    {
        esm.startSubRecord('SNAM');
        esm.writeType<quint32>(w.wordFormId);
        esm.writeType<quint32>(w.spellFormId);
        esm.writeType<float>(w.recoveryTime);
        esm.endSubRecord();
    }

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void ShouRecord::blank()
{
    editorId.clear();
    formId = 0;
    fullName.clear();
    words.clear();
    rawSubRecords.clear();
    initComponents();
}
