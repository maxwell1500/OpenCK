#include "Enchrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "subrecordreplay.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tesfullname.hpp"

void EnchRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFullName_Component>();
}

void EnchRecord::load(ESMReader& esm, bool)
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
            case 'ENIT':
                hasEnit = true;
                if (esm.subLeft() >= 4) type = esm.readType<quint32>();
                if (esm.subLeft() >= 4) charges = esm.readType<quint32>();
                if (esm.subLeft() >= 4) costLimit = esm.readType<quint32>();
                if (esm.subLeft() > 0) esm.readRawSubData(enitExtra);
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
    auto* nameComp = static_cast<tescomponents::TESFullName_Component*>(components.findByName(QStringLiteral("TESFullName")));
    if (nameComp) name = nameComp->fullName;
}

void EnchRecord::save(ESMWriter& esm) const
{
    auto* nameComp = static_cast<tescomponents::TESFullName_Component*>(const_cast<EnchRecord*>(this)->components.findByName(QStringLiteral("TESFullName")));
    if (nameComp) nameComp->fullName = name;

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
    const auto writeEnit = [&]()
    {
        esm.startSubRecord('ENIT');
        esm.writeType<quint32>(type);
        esm.writeType<quint32>(charges);
        esm.writeType<quint32>(costLimit);
        if (!enitExtra.isEmpty())
            esm.writeRawData(enitExtra.constData(), enitExtra.size());
        esm.endSubRecord();
    };

    bool wroteEdid = false, wroteFlags = false, wroteEnit = false;
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
            case 'ENIT':
                if (!wroteEnit && (hasEnit || type != 0 || charges != 0 || costLimit != 0)) { writeEnit(); wroteEnit = true; }
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
    if (!wroteEnit && (hasEnit || type != 0 || charges != 0 || costLimit != 0))
        writeEnit();

    replay.writeLeftover(esm);
}

void EnchRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    name.clear();
    costLimit = 0;
    charges = 0;
    enchantmentData = 0;
    charge = 0.0f;
    duration = 0;
    magnitude = 0.0f;
    type = 0;
    soulGem = 0;
    rawSubRecords.clear();
    loadOrder.clear();
    hasFlags = false;
    flagsSpelling = NAME('FNAM');
    flagsWidth = 4;
    hasEnit = false;
    enitExtra.clear();
    initComponents();
}
