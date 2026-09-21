#include "weaprecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tier2_components.hpp"
#include "../../components/tesfullname.hpp"

#include <QSet>

void WeaponRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFullName_Component>();
    components.add<tescomponents::TESModel_Component>();
    components.add<tescomponents::TESTexture_Component>();
    components.add<tescomponents::BGSPickupPutdownSounds_Component>();
}

void WeaponRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    loadOrder.clear();
    loadIsRaw.clear();
    hasEdid = false;
    hasFlags = false;
    flagsName = NAME('FNAM');
    hasData = false;
    hasEamt = false;
    hasMdob = false;
    hasEnam = false;
    eamtWidth = 4;
    mdobWidth = 4;
    enamWidth = 4;
    // Width-preserving scalar read (narrow-packed EAMT/MDOB/ENAM).
    auto readScalar = [&](quint32& out, quint8& width) {
        const qint64 left = esm.subLeft();
        if (left >= 4)
        {
            out = esm.readType<quint32>();
            width = 4;
        }
        else
        {
            out = 0;
            width = static_cast<quint8>(qMax<qint64>(left, 0));
            for (qint64 i = 0; i < left; ++i)
                out |= quint32(esm.readType<quint8>()) << (8 * i);
        }
        if (esm.subLeft() > 0)
            esm.skip(static_cast<int>(esm.subLeft()));
    };
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
        if (handled) { loadIsRaw.append(0); continue; }
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); hasEdid = true; loadIsRaw.append(0); break;
            case 'FNAM': case 'FLAG':
                flags = esm.readType<quint32>(); hasFlags = true; flagsName = sub;
                loadIsRaw.append(0); break;
            case 'DATA': {
                if (esm.subLeft() >= 24)
                {
                    hasData = true;
                    weaponType = esm.readType<quint32>();
                    speed = esm.readType<float>();
                    reach = esm.readType<float>();
                    flags = esm.readType<quint32>();
                    value = esm.readType<quint32>();
                    weight = esm.readType<float>();
                    loadIsRaw.append(0);
                }
                else
                {
                    RawSubRecord raw;
                    raw.name = sub;
                    esm.readRawSubData(raw.data);
                    rawSubRecords.push_back(raw);
                    loadIsRaw.append(1);
                }
                break;
            }
            case 'EAMT': readScalar(enchantment, eamtWidth); hasEamt = true; loadIsRaw.append(0); break;
            case 'MDOB': readScalar(magicSchool, mdobWidth); hasMdob = true; loadIsRaw.append(0); break;
            case 'ENAM': readScalar(enchantLimit, enamWidth); hasEnam = true; loadIsRaw.append(0); break;
            case 'DNAM':
            default:
            {
                RawSubRecord raw;
                raw.name = sub;
                esm.readRawSubData(raw.data);
                rawSubRecords.push_back(raw);
                loadIsRaw.append(1);
                break;
            }
        }
    }
    auto* nameComp = static_cast<tescomponents::TESFullName_Component*>(components.findByName(QStringLiteral("TESFullName")));
    if (nameComp) fullName = nameComp->fullName;
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(components.findByName(QStringLiteral("TESTexture")));
    if (tex) iconPath = tex->iconPath;
    auto* model = static_cast<tescomponents::TESModel_Component*>(components.findByName(QStringLiteral("TESModel")));
    if (model) modelPath = model->modelPath;
}

void WeaponRecord::save(ESMWriter& esm) const
{
    auto* nameComp = static_cast<tescomponents::TESFullName_Component*>(const_cast<WeaponRecord*>(this)->components.findByName(QStringLiteral("TESFullName")));
    if (nameComp) nameComp->fullName = fullName;
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(const_cast<WeaponRecord*>(this)->components.findByName(QStringLiteral("TESTexture")));
    if (tex) tex->iconPath = iconPath;
    auto* model = static_cast<tescomponents::TESModel_Component*>(const_cast<WeaponRecord*>(this)->components.findByName(QStringLiteral("TESModel")));
    if (model) model->modelPath = modelPath;

    const auto writeScalar = [&](NAME name, quint32 value, quint8 width) {
        quint8 w = width;
        if (w == 0 && value != 0)
            w = 4;
        esm.startSubRecord(name);
        for (quint8 i = 0; i < w; ++i)
            esm.writeType<quint8>(static_cast<quint8>((value >> (8 * i)) & 0xFF));
        esm.endSubRecord();
    };
    const auto writeData = [&] {
        esm.startSubRecord('DATA');
        esm.writeType<quint32>(weaponType);
        esm.writeType<float>(speed);
        esm.writeType<float>(reach);
        esm.writeType<quint32>(flags);
        esm.writeType<quint32>(value);
        esm.writeType<float>(weight);
        esm.endSubRecord();
    };

    if (loadOrder.isEmpty())
    {
        if (hasEdid || !editorId.isEmpty())
            esm.writeSubZString('EDID', editorId);
        components.saveAll(esm);
        if (hasFlags || flags != 0)
            esm.writeSubData<quint32>(flagsName, flags);
        if (hasData)
            writeData();
        if (hasEamt) writeScalar(NAME('EAMT'), enchantment, eamtWidth);
        if (hasMdob) writeScalar(NAME('MDOB'), magicSchool, mdobWidth);
        if (hasEnam) writeScalar(NAME('ENAM'), enchantLimit, enamWidth);
        for (const auto& raw : rawSubRecords)
            esm.writeRawSubRecord(raw);
        return;
    }

    QSet<NAME> seen;
    int rawCur = 0;
    for (int p = 0; p < loadOrder.size(); ++p)
    {
        const NAME sub = loadOrder[p];
        seen.insert(sub);
        const bool isRaw = p >= loadIsRaw.size() || loadIsRaw[p] != 0;
        if (isRaw)
        {
            if (rawCur < rawSubRecords.size())
                esm.writeRawSubRecord(rawSubRecords[rawCur++]);
            continue;
        }
        switch (sub)
        {
        case 'EDID': esm.writeSubZString('EDID', editorId); break;
        case 'FNAM': case 'FLAG': esm.writeSubData<quint32>(sub, flags); break;
        case 'DATA': if (hasData) writeData(); break;
        case 'EAMT': writeScalar(NAME('EAMT'), enchantment, eamtWidth); break;
        case 'MDOB': writeScalar(NAME('MDOB'), magicSchool, mdobWidth); break;
        case 'ENAM': writeScalar(NAME('ENAM'), enchantLimit, enamWidth); break;
        default:
        {
            bool done = false;
            for (auto& c : components.all())
            {
                if (c->canHandle(sub)) { done = c->writeSubrecord(sub, esm); break; }
            }
            if (!done && rawCur < rawSubRecords.size())
                esm.writeRawSubRecord(rawSubRecords[rawCur++]);
            break;
        }
        }
    }

    if (!seen.contains(NAME('FNAM')) && !seen.contains(NAME('FLAG')) && (hasFlags || flags != 0))
        esm.writeSubData<quint32>(flagsName, flags);
    if (!seen.contains(NAME('DATA')) && hasData)
        writeData();
    if (!seen.contains(NAME('EAMT')) && hasEamt) writeScalar(NAME('EAMT'), enchantment, eamtWidth);
    if (!seen.contains(NAME('MDOB')) && hasMdob) writeScalar(NAME('MDOB'), magicSchool, mdobWidth);
    if (!seen.contains(NAME('ENAM')) && hasEnam) writeScalar(NAME('ENAM'), enchantLimit, enamWidth);
    const auto leftover = [&](const QString& cls, const QVector<NAME>& names) {
        const Component* comp = components.findByName(cls);
        if (!comp) return;
        for (NAME name : names)
            if (!seen.contains(name))
                comp->writeSubrecord(name, esm);
    };
    leftover(QStringLiteral("TESModel"), { NAME('MODL'), NAME('MNAM') });
    leftover(QStringLiteral("TESTexture"), { NAME('ICON'), NAME('ICO2') });
    leftover(QStringLiteral("BGSPickupPutdownSounds"), { NAME('YNAM'), NAME('ZNAM') });
    while (rawCur < rawSubRecords.size())
        esm.writeRawSubRecord(rawSubRecords[rawCur++]);
}

void WeaponRecord::blank()
{
    editorId.clear();
    fullName.clear();
    formId = 0;
    flags = 0;
    weaponType = 0;
    damage = 0.0f;
    speed = 0.0f;
    reach = 0.0f;
    weight = 0.0f;
    value = 0;
    enchantment = 0;
    iconPath.clear();
    modelPath.clear();
    magicSchool = 0;
    enchantLimit = 0;
    loadOrder.clear();
    loadIsRaw.clear();
    hasEdid = false;
    hasFlags = false;
    flagsName = NAME('FNAM');
    hasData = false;
    hasEamt = false;
    hasMdob = false;
    hasEnam = false;
    eamtWidth = 4;
    mdobWidth = 4;
    enamWidth = 4;
    rawSubRecords.clear();
    verbatimBody.clear();
    verbatimFlags = 0;
    verbatimSnapshot.reset();
    initComponents();
}
