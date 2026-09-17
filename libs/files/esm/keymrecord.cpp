#include "keymrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "subrecordreplay.hpp"
#include "../../components/tier2_components.hpp"

void KeymRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();

    if (!components.findByName(QStringLiteral("TESFullName")))
        components.add<tescomponents::TESFullName_Component>();
    if (!components.findByName(QStringLiteral("TESModel")))
        components.add<tescomponents::TESModel_Component>();
    if (!components.findByName(QStringLiteral("TESTexture")))
        components.add<tescomponents::TESTexture_Component>();
    if (!components.findByName(QStringLiteral("BGSPickupPutdownSounds")))
        components.add<tescomponents::BGSPickupPutdownSounds_Component>();

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
                flags = esm.readSubU32(&flagsWidth);
                hasFlags = true;
                flagsSpelling = sub;
                handled = true;
                break;
            case 'DATA':
                hasData = true;
                if (esm.subLeft() >= 8)
                {
                    value = esm.readType<quint32>();
                    weight = esm.readType<float>();
                }
                else if (esm.subLeft() > 0)
                {
                    esm.skip(static_cast<int>(esm.subLeft()));
                }
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

    if (auto* n = static_cast<tescomponents::TESFullName_Component*>(
            components.findByName(QStringLiteral("TESFullName"))))
    {
        fullName = n->fullName;
    }
    if (auto* m = static_cast<tescomponents::TESModel_Component*>(
            components.findByName(QStringLiteral("TESModel"))))
    {
        modelPath = m->modelPath;
    }
    if (auto* t = static_cast<tescomponents::TESTexture_Component*>(
            components.findByName(QStringLiteral("TESTexture"))))
    {
        iconPath = t->iconPath;
    }
}

void KeymRecord::save(ESMWriter& esm) const
{
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
    const auto writeData = [&]()
    {
        esm.startSubRecord('DATA');
        esm.writeType<quint32>(value);
        esm.writeType<float>(weight);
        esm.endSubRecord();
    };

    bool wroteEdid = false, wroteFlags = false, wroteData = false;
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
                if (!wroteData && (hasData || value != 0 || weight != 0.0f)) { writeData(); wroteData = true; }
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
    if (!wroteData && (hasData || value != 0 || weight != 0.0f))
        writeData();

    replay.writeLeftover(esm);
}

void KeymRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    fullName = "";
    iconPath = "";
    modelPath = "";
    weight = 0.0f;
    value = 0;
    rawSubRecords.clear();
    loadOrder.clear();
    hasFlags = false;
    flagsSpelling = NAME('FNAM');
    flagsWidth = 4;
    hasData = false;
    components.clear();
}
