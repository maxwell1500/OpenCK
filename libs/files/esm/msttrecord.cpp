#include "msttrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace
{
// Starfield writes some TTSM flag subrecords shorter than 32 bits (observed:
// 1-byte DATA/FNAM). Read only what is declared, little-endian, so the
// stream never advances past the record end (which desynced every following
// record in the file).
quint32 readFlagsLE(ESMReader& esm, NAME sub, quint8& width)
{
    qint64 n = esm.subLeft();
    if (n <= 0)
    {
        width = 0;
        return 0;
    }
    if (n > 4)
        n = 4;
    width = static_cast<quint8>(n);
    quint32 v = 0;
    for (qint64 i = 0; i < n; ++i)
        v |= quint32(esm.readType<quint8>()) << (8 * i);
    return v;
}

void writeFlagsWidth(ESMWriter& esm, NAME sub, quint32 value, quint8 width)
{
    esm.startSubRecord(sub);
    for (int i = 0; i < width; ++i)
        esm.writeType<quint8>(static_cast<quint8>((value >> (8 * i)) & 0xFF));
    esm.endSubRecord();
}
}

void MsttRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();

    if (!components.findByName(QStringLiteral("TESModel")))
        components.add<tescomponents::TESModel_Component>();

    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;

        bool handled = false;
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); handled = true; break;
            case 'FNAM': case 'FLAG': flags = readFlagsLE(esm, sub, fnamWidth); handled = true; break;
            case 'DATA': msttFlags = readFlagsLE(esm, sub, dataWidth); handled = true; break;
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

    if (auto* m = static_cast<tescomponents::TESModel_Component*>(
            components.findByName(QStringLiteral("TESModel"))))
    {
        modelPath = m->modelPath;
    }
}

void MsttRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    writeFlagsWidth(esm, NAME('FNAM'), flags, fnamWidth);
    writeFlagsWidth(esm, NAME('DATA'), msttFlags, dataWidth);

    components.saveAll(esm);

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
    }
}

void MsttRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    modelPath = "";
    msttFlags = 0;
    rawSubRecords.clear();
    components.clear();
}
