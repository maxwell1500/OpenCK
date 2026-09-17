#include "msttrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "subrecordreplay.hpp"

void MsttRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();

    if (!components.findByName(QStringLiteral("TESModel")))
        components.add<tescomponents::TESModel_Component>();

    loadOrder.clear();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;
        loadOrder.append(sub);

        bool handled = false;
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); handled = true; break;
            case 'FNAM': case 'FLAG':
                flags = esm.readSubU32(&fnamWidth);
                hasFnam = true;
                handled = true;
                break;
            case 'DATA':
                msttFlags = esm.readSubU32(&dataWidth);
                hasData = true;
                handled = true;
                break;
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
    SubrecordReplay replay;
    replay.init(rawSubRecords);

    const auto writeWidth = [&](NAME sub, quint32 value, quint8 width)
    {
        esm.startSubRecord(sub);
        const quint8 w = width == 0 ? 1 : width;
        for (quint8 i = 0; i < w; ++i)
            esm.writeType<quint8>(static_cast<quint8>((value >> (8 * i)) & 0xFF));
        esm.endSubRecord();
    };

    bool wroteEdid = false, wroteFnam = false, wroteData = false;
    for (NAME sub : loadOrder)
    {
        switch (sub)
        {
            case 'EDID':
                if (!wroteEdid) { esm.writeSubZString('EDID', editorId); wroteEdid = true; }
                break;
            case 'FNAM': case 'FLAG':
                if (!wroteFnam && (hasFnam || flags != 0)) { writeWidth(sub, flags, fnamWidth); wroteFnam = true; }
                break;
            case 'DATA':
                if (!wroteData && (hasData || msttFlags != 0)) { writeWidth(sub, msttFlags, dataWidth); wroteData = true; }
                break;
            default:
                if (!components.writeSubrecord(sub, esm))
                    replay.write(sub, esm);
                break;
        }
    }

    if (!wroteEdid && !editorId.isEmpty())
        esm.writeSubZString('EDID', editorId);
    if (!wroteFnam && (hasFnam || flags != 0))
        writeWidth(NAME('FNAM'), flags, fnamWidth);
    if (!wroteData && (hasData || msttFlags != 0))
        writeWidth(NAME('DATA'), msttFlags, dataWidth);

    replay.writeLeftover(esm);
}

void MsttRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    modelPath = "";
    msttFlags = 0;
    fnamWidth = 4;
    dataWidth = 4;
    hasFnam = false;
    hasData = false;
    rawSubRecords.clear();
    loadOrder.clear();
    components.clear();
}
