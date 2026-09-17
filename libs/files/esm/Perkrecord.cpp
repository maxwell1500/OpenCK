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
    descRaws.clear();
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
                QByteArray bytes;
                esm.readRawSubData(bytes);
                descRaws.append(bytes);
                if (descRaws.size() == 1)
                {
                    const int nul = bytes.indexOf('\0');
                    description = QString::fromUtf8(bytes.constData(),
                        nul >= 0 ? nul : bytes.size());
                }
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

    bool wroteEdid = false, wroteFlags = false;
    int descIdx = 0;
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
                if (descIdx < descRaws.size())
                    esm.writeRawSubRecord(RawSubRecord{ NAME('DESC'), descRaws[descIdx] });
                else if (descIdx == descRaws.size() && !description.isEmpty())
                    esm.writeSubZString('DESC', description);
                ++descIdx;
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
    while (descIdx < descRaws.size())
    {
        esm.writeRawSubRecord(RawSubRecord{ NAME('DESC'), descRaws[descIdx] });
        ++descIdx;
    }
    if (descRaws.isEmpty() && !description.isEmpty())
        esm.writeSubZString('DESC', description);

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
    descRaws.clear();
    initComponents();
}
