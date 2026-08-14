#include "creaturerecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier3_components.hpp"

void CreatureRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();

    if (!components.findByName(QStringLiteral("TESFullName")))
        components.add<tescomponents::TESFullName_Component>();
    if (!components.findByName(QStringLiteral("TESAIForm")))
        components.add<tescomponents::TESAIForm_Component>();

    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;

        bool handled = false;
        switch (sub)
        {
        case 'EDID': editorId = esm.readZString(); handled = true; break;
        case 'DATA':
        {
            if (esm.subLeft() == static_cast<qint64>(sizeof(CreaData)))
            {
                creaData.type = esm.readType<quint32>();
                creaData.acrobatics = esm.readType<quint16>();
                creaData.armorer = esm.readType<quint16>();
                creaData.athletics = esm.readType<quint16>();
                creaData.blade = esm.readType<quint16>();
                creaData.block = esm.readType<quint16>();
                creaData.blunt = esm.readType<quint16>();
                creaData.handToHand = esm.readType<quint16>();
                creaData.heavyArmor = esm.readType<quint16>();
                creaData.alchemy = esm.readType<quint16>();
                creaData.alteration = esm.readType<quint16>();
                creaData.conjuration = esm.readType<quint16>();
                creaData.destruction = esm.readType<quint16>();
                creaData.illusion = esm.readType<quint16>();
                creaData.mysticism = esm.readType<quint16>();
                creaData.restoration = esm.readType<quint16>();
                creaData.lightArmor = esm.readType<quint16>();
                creaData.marksman = esm.readType<quint16>();
                creaData.mercantile = esm.readType<quint16>();
                creaData.security = esm.readType<quint16>();
                creaData.sneak = esm.readType<quint16>();
                creaData.speechcraft = esm.readType<quint16>();
                creaData.health = esm.readType<quint16>();
                creaData.magicka = esm.readType<quint16>();
                creaData.fatigue = esm.readType<quint16>();
                creaData.damage = esm.readType<quint16>();
                creaData.strength = esm.readType<quint16>();
                creaData.intelligence = esm.readType<quint16>();
                creaData.willpower = esm.readType<quint16>();
                creaData.agility = esm.readType<quint16>();
                creaData.speed = esm.readType<quint16>();
                creaData.endurance = esm.readType<quint16>();
                creaData.personality = esm.readType<quint16>();
                creaData.luck = esm.readType<quint16>();
                creaData.xp = esm.readType<quint16>();
                hasData = true;
            }
            else
            {
                RawSubRecord raw;
                raw.name = sub;
                esm.readRawSubData(raw.data);
                rawSubRecords.push_back(raw);
            }
            handled = true;
            break;
        }
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
}

void CreatureRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);

    if (hasData)
    {
        esm.startSubRecord('DATA');
        esm.writeType<quint32>(creaData.type);
        esm.writeType<quint16>(creaData.acrobatics);
        esm.writeType<quint16>(creaData.armorer);
        esm.writeType<quint16>(creaData.athletics);
        esm.writeType<quint16>(creaData.blade);
        esm.writeType<quint16>(creaData.block);
        esm.writeType<quint16>(creaData.blunt);
        esm.writeType<quint16>(creaData.handToHand);
        esm.writeType<quint16>(creaData.heavyArmor);
        esm.writeType<quint16>(creaData.alchemy);
        esm.writeType<quint16>(creaData.alteration);
        esm.writeType<quint16>(creaData.conjuration);
        esm.writeType<quint16>(creaData.destruction);
        esm.writeType<quint16>(creaData.illusion);
        esm.writeType<quint16>(creaData.mysticism);
        esm.writeType<quint16>(creaData.restoration);
        esm.writeType<quint16>(creaData.lightArmor);
        esm.writeType<quint16>(creaData.marksman);
        esm.writeType<quint16>(creaData.mercantile);
        esm.writeType<quint16>(creaData.security);
        esm.writeType<quint16>(creaData.sneak);
        esm.writeType<quint16>(creaData.speechcraft);
        esm.writeType<quint16>(creaData.health);
        esm.writeType<quint16>(creaData.magicka);
        esm.writeType<quint16>(creaData.fatigue);
        esm.writeType<quint16>(creaData.damage);
        esm.writeType<quint16>(creaData.strength);
        esm.writeType<quint16>(creaData.intelligence);
        esm.writeType<quint16>(creaData.willpower);
        esm.writeType<quint16>(creaData.agility);
        esm.writeType<quint16>(creaData.speed);
        esm.writeType<quint16>(creaData.endurance);
        esm.writeType<quint16>(creaData.personality);
        esm.writeType<quint16>(creaData.luck);
        esm.writeType<quint16>(creaData.xp);
        esm.endSubRecord();
    }

    components.saveAll(esm);

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
    }
}

void CreatureRecord::blank()
{
    editorId = ""; formId = 0; flags = 0;
    std::memset(&creaData, 0, sizeof(CreaData));
    hasData = false;
    fullName = "";
    rawSubRecords.clear();
    components.clear();
}
