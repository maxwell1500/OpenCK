#include "Perkrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "subrecordreplay.hpp"
#include "../../components/tier1_components.hpp"

void PerkRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESTexture_Component>();
}

void PerkRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    loadOrder.clear();
    descRaw.clear();
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
            case 'DESC':
            {
                esm.readRawSubData(descRaw);
                const int nul = descRaw.indexOf('\0');
                description = QString::fromUtf8(descRaw.constData(),
                    nul >= 0 ? nul : descRaw.size());
                break;
            }
            case 'CTDA':
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
}

void PerkRecord::save(ESMWriter& esm) const
{
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(const_cast<PerkRecord*>(this)->components.findByName(QStringLiteral("TESTexture")));
    if (tex) tex->iconPath = iconPath;

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

    bool wroteEdid = false, wroteFlags = false, wroteDesc = false;
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
            case 'DESC':
                if (!wroteDesc && (!descRaw.isEmpty() || !description.isEmpty()))
                {
                    if (!descRaw.isEmpty())
                        esm.writeRawSubRecord(RawSubRecord{ NAME('DESC'), descRaw });
                    else
                        esm.writeSubZString('DESC', description);
                    wroteDesc = true;
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
    if (!wroteDesc && (!descRaw.isEmpty() || !description.isEmpty()))
    {
        if (!descRaw.isEmpty())
            esm.writeRawSubRecord(RawSubRecord{ NAME('DESC'), descRaw });
        else
            esm.writeSubZString('DESC', description);
    }

    replay.writeLeftover(esm);
}

void PerkRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    description.clear();
    requirements.clear();
    iconPath.clear();
    conditions.clear();
    rawSubRecords.clear();
    loadOrder.clear();
    hasFlags = false;
    flagsSpelling = NAME('FNAM');
    flagsWidth = 4;
    descRaw.clear();
    initComponents();
}
