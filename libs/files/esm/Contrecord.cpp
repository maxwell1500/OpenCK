#include "Contrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "subrecordreplay.hpp"
#include "../../components/tier1_components.hpp"

void ContRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESModel_Component>();
    components.add<tescomponents::TESTexture_Component>();
    components.add<tescomponents::TESContainer_Component>();
}

void ContRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    loadOrder.clear();
    dataRaw.clear();
    coctRaw.clear();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;
        loadOrder.append(sub);
        bool handled = false;
        for (auto& c : components.all())
        {
            if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        }
        if (handled) continue;
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': case 'FLAG':
                flags = esm.readSubU32(&flagsWidth);
                hasFlags = true;
                flagsSpelling = sub;
                break;
            case 'DATA':
                esm.readRawSubData(dataRaw);
                flags = dataRaw.isEmpty() ? 0 : static_cast<quint32>(static_cast<quint8>(dataRaw.at(0)));
                break;
            case 'COCT':
                esm.readRawSubData(coctRaw);
                inventoryControl = 0;
                for (int i = 0; i < qMin(4, coctRaw.size()); ++i)
                    inventoryControl |= quint32(static_cast<quint8>(coctRaw.at(i))) << (8 * i);
                break;
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
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(components.findByName(QStringLiteral("TESTexture")));
    if (tex) iconPath = tex->iconPath;
    auto* model = static_cast<tescomponents::TESModel_Component*>(components.findByName(QStringLiteral("TESModel")));
    if (model) modelPath = model->modelPath;
}

void ContRecord::save(ESMWriter& esm) const
{
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(const_cast<ContRecord*>(this)->components.findByName(QStringLiteral("TESTexture")));
    if (tex) tex->iconPath = iconPath;
    auto* model = static_cast<tescomponents::TESModel_Component*>(const_cast<ContRecord*>(this)->components.findByName(QStringLiteral("TESModel")));
    if (model) model->modelPath = modelPath;
    auto* cont = static_cast<tescomponents::TESContainer_Component*>(const_cast<ContRecord*>(this)->components.findByName(QStringLiteral("TESContainer")));

    SubrecordReplay replay;
    replay.init(rawSubRecords);

    const auto writeFlags = [&]()
    {
        esm.startSubRecord(flagsSpelling);
        const quint8 w = flagsWidth == 0 ? 1 : flagsWidth;
        for (quint8 i = 0; i < w; ++i)
            esm.writeType<quint8>(static_cast<quint8>((flags >> (8 * i)) & 0xFF));
        esm.endSubRecord();
    };

    bool wroteEdid = false, wroteFlags = false, wroteData = false, wroteCoct = false;
    int cntoIdx = 0;
    for (NAME sub : loadOrder)
    {
        switch (sub)
        {
            case 'EDID':
                if (!wroteEdid) { esm.writeSubZString('EDID', editorId); wroteEdid = true; }
                break;
            case 'FNAM': case 'FLAG':
                if (!wroteFlags && (hasFlags || flags != 0)) { writeFlags(); wroteFlags = true; }
                break;
            case 'DATA':
                if (!wroteData)
                {
                    if (!dataRaw.isEmpty())
                        esm.writeRawSubRecord(RawSubRecord{ NAME('DATA'), dataRaw });
                    else
                        esm.writeSubData<quint32>('DATA', flags);
                    wroteData = true;
                }
                break;
            case 'COCT':
                if (!wroteCoct)
                {
                    if (!coctRaw.isEmpty())
                        esm.writeRawSubRecord(RawSubRecord{ NAME('COCT'), coctRaw });
                    else
                        esm.writeSubData<quint32>('COCT', inventoryControl);
                    wroteCoct = true;
                }
                break;
            case 'CNTO':
                if (cont && cntoIdx < cont->items.size())
                {
                    const auto& entry = cont->items[cntoIdx];
                    esm.startSubRecord('CNTO');
                    esm.writeType<quint32>(entry.formId);
                    esm.writeType<qint32>(entry.count);
                    esm.endSubRecord();
                }
                ++cntoIdx;
                break;
            default:
                if (!components.writeSubrecord(sub, esm))
                    replay.write(sub, esm);
                break;
        }
    }

    if (!wroteEdid && !editorId.isEmpty())
        esm.writeSubZString('EDID', editorId);
    if (!wroteFlags && (hasFlags || loadOrder.isEmpty()))
        writeFlags();
    if (!wroteData && (!dataRaw.isEmpty() || loadOrder.isEmpty()))
    {
        if (!dataRaw.isEmpty())
            esm.writeRawSubRecord(RawSubRecord{ NAME('DATA'), dataRaw });
        else
            esm.writeSubData<quint32>('DATA', flags);
    }
    if (!wroteCoct && (!coctRaw.isEmpty() || loadOrder.isEmpty()))
    {
        if (!coctRaw.isEmpty())
            esm.writeRawSubRecord(RawSubRecord{ NAME('COCT'), coctRaw });
        else
            esm.writeSubData<quint32>('COCT', inventoryControl);
    }
    if (cont)
    {
        for (int i = cntoIdx; i < cont->items.size(); ++i)
        {
            const auto& entry = cont->items[i];
            esm.startSubRecord('CNTO');
            esm.writeType<quint32>(entry.formId);
            esm.writeType<qint32>(entry.count);
            esm.endSubRecord();
        }
    }

    replay.writeLeftover(esm);
}

void ContRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    iconPath.clear();
    modelPath.clear();
    contents = 0;
    inventoryControl = 0;
    weight = 0.0f;
    value = 0;
    rawSubRecords.clear();
    loadOrder.clear();
    hasFlags = false;
    flagsSpelling = NAME('FNAM');
    flagsWidth = 4;
    dataRaw.clear();
    coctRaw.clear();
    initComponents();
}
