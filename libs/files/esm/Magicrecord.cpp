#include "Magicrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "subrecordreplay.hpp"
#include "../../components/tier1_components.hpp"

void MagicRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESModel_Component>();
    components.add<tescomponents::TESTexture_Component>();
}

void MagicRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    loadOrder.clear();
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
            case 'MDOB':
                schools = esm.readSubU32();
                hasMdob = true;
                break;
            case 'SNAM':
                castingSound = esm.readSubU32();
                hasSnam = true;
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

void MagicRecord::save(ESMWriter& esm) const
{
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(const_cast<MagicRecord*>(this)->components.findByName(QStringLiteral("TESTexture")));
    if (tex) tex->iconPath = iconPath;
    auto* model = static_cast<tescomponents::TESModel_Component*>(const_cast<MagicRecord*>(this)->components.findByName(QStringLiteral("TESModel")));
    if (model) model->modelPath = modelPath;

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

    bool wroteEdid = false, wroteFlags = false, wroteMdob = false, wroteSnam = false;
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
            case 'MDOB':
                if (!wroteMdob && (hasMdob || schools != 0))
                {
                    esm.writeSubData<quint32>('MDOB', schools);
                    wroteMdob = true;
                }
                break;
            case 'SNAM':
                if (!wroteSnam && (hasSnam || castingSound != 0))
                {
                    esm.writeSubData<quint32>('SNAM', castingSound);
                    wroteSnam = true;
                }
                break;
            default:
                if (!components.writeSubrecord(sub, esm))
                    replay.write(sub, esm);
                break;
        }
    }

    if (!wroteEdid && !editorId.isEmpty())
        esm.writeSubZString('EDID', editorId);
    if (!wroteFlags && (hasFlags || flags != 0))
        writeFlags();
    if (!wroteMdob && (hasMdob || schools != 0))
        esm.writeSubData<quint32>('MDOB', schools);
    if (!wroteSnam && (hasSnam || castingSound != 0))
        esm.writeSubData<quint32>('SNAM', castingSound);

    replay.writeLeftover(esm);
}

void MagicRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    schools = 0;
    damageType = 0;
    castingSound = 0;
    iconPath.clear();
    modelPath.clear();
    effects.clear();
    rawSubRecords.clear();
    loadOrder.clear();
    hasFlags = false;
    flagsSpelling = NAME('FNAM');
    flagsWidth = 4;
    hasMdob = false;
    hasSnam = false;
    initComponents();
}
