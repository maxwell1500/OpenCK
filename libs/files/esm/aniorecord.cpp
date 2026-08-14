#include "aniorecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

void AnioRecord::initComponents()
{
    components.clear();
}

void AnioRecord::load(ESMReader& esm, bool)
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
            case 'DNAM':
            {
                // Fixed-size model path (0x100 bytes).
                QByteArray data;
                esm.readRawSubData(data);
                int len = 0;
                while (len < data.size() && data.at(len) != '\0') ++len;
                modelPath = QString::fromLatin1(data.constData(), len);
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

void AnioRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    QByteArray data = modelPath.toLatin1();
    data.resize(256);
    esm.startSubRecord('DNAM');
    esm.writeRawData(data.constData(), 256);
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
    }
}

void AnioRecord::blank()
{
    editorId.clear();
    formId = 0;
    modelPath.clear();
    rawSubRecords.clear();
    initComponents();
}
