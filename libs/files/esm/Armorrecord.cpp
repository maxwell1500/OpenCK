#include "Armorrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tier2_components.hpp"
#include "../../components/tesfullname.hpp"

void ArmorRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFullName_Component>();
    components.add<tescomponents::TESModel_Component>();
    components.add<tescomponents::TESTexture_Component>();
    components.add<tescomponents::TESBipedModel_Component>();
    components.add<tescomponents::TESEnchantableForm_Component>();
    components.add<tescomponents::BGSPickupPutdownSounds_Component>();
}

void ArmorRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    loadOrder.clear();
    loadIsRaw.clear();
    hasEdid = false;
    hasFlag = false;
    flagWidth = 4;
    flagExtra.clear();
    hasDnam = false;
    dnamWidth = 4;
    dnamExtra.clear();
    hasData = false;
    dataExtra.clear();
    // Width-preserving scalar read (Starfield packs some flag/word
    // subrecords narrower than 4 bytes; a fixed u32 read would consume the
    // next subrecord). Extra tail bytes ride along for exact re-emit.
    auto readWidthU32 = [&](quint32& out, quint8& width, QByteArray& extra) {
        extra.clear();
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
            esm.readRawSubData(extra);
    };
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;
        loadOrder.append(sub);
        // ENAM/ANAM are u32 form IDs at 4 bytes; anything else (ARMO ANAM
        // carries a 47-byte path) stays record-level raw.
        if ((sub == NAME('ENAM') || sub == NAME('ANAM')) && esm.subLeft() != 4)
        {
            RawSubRecord raw;
            raw.name = sub;
            esm.readRawSubData(raw.data);
            rawSubRecords.push_back(raw);
            loadIsRaw.append(1);
            continue;
        }
        // INDT belongs to no editor; keep it record-level so it never lands
        // in the biped component's raw stash.
        if (sub == NAME('INDT'))
        {
            RawSubRecord raw;
            raw.name = sub;
            esm.readRawSubData(raw.data);
            rawSubRecords.push_back(raw);
            loadIsRaw.append(1);
            continue;
        }
        bool handled = false;
        for (auto& c : components.all())
        {
            if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        }
        if (handled) { loadIsRaw.append(0); continue; }
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); hasEdid = true; loadIsRaw.append(0); break;
            case 'FLAG': readWidthU32(flags, flagWidth, flagExtra); hasFlag = true; loadIsRaw.append(0); break;
            case 'DNAM': readWidthU32(armorRating, dnamWidth, dnamExtra); hasDnam = true; loadIsRaw.append(0); break;
            case 'DATA': {
                if (esm.subLeft() >= 8)
                {
                    hasData = true;
                    value = esm.readType<quint32>();
                    weight = esm.readType<float>();
                    if (esm.subLeft() > 0)
                        esm.readRawSubData(dataExtra);
                    else
                        dataExtra.clear();
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

void ArmorRecord::save(ESMWriter& esm) const
{
    auto* nameComp = static_cast<tescomponents::TESFullName_Component*>(const_cast<ArmorRecord*>(this)->components.findByName(QStringLiteral("TESFullName")));
    if (nameComp) nameComp->fullName = fullName;
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(const_cast<ArmorRecord*>(this)->components.findByName(QStringLiteral("TESTexture")));
    if (tex) tex->iconPath = iconPath;
    auto* model = static_cast<tescomponents::TESModel_Component*>(const_cast<ArmorRecord*>(this)->components.findByName(QStringLiteral("TESModel")));
    if (model) model->modelPath = modelPath;

    const auto writeWidthU32 = [&](NAME name, quint32 value, quint8 width, const QByteArray& extra) {
        quint8 w = width;
        if (w == 0 && value != 0)
            w = 4;
        esm.startSubRecord(name);
        for (quint8 i = 0; i < w; ++i)
            esm.writeType<quint8>(static_cast<quint8>((value >> (8 * i)) & 0xFF));
        if (!extra.isEmpty())
            esm.writeRawData(extra.constData(), extra.size());
        esm.endSubRecord();
    };

    if (loadOrder.isEmpty())
    {
        // Assembled record (no on-disk order): fixed field order, emitting
        // only what the record carries (save rule).
        if (hasEdid || !editorId.isEmpty())
            esm.writeSubZString('EDID', editorId);
        components.saveAll(esm);
        if (hasDnam || armorRating != 0)
            writeWidthU32(NAME('DNAM'), armorRating, dnamWidth, dnamExtra);
        if (hasData || value != 0 || weight != 0.0f)
        {
            esm.startSubRecord('DATA');
            esm.writeType<quint32>(value);
            esm.writeType<float>(weight);
            if (!dataExtra.isEmpty())
                esm.writeRawData(dataExtra.constData(), dataExtra.size());
            esm.endSubRecord();
        }

        for (const auto& raw : rawSubRecords)
        {
            esm.writeRawSubRecord(raw);
        }
        return;
    }

    QSet<NAME> seen;
    int rawCur = 0;
    int modlCur = 0;
    int mnamCur = 0;
    int indxCur = 0;
    int bmdtCur = 0;
    tescomponents::TESBipedModel_Component* biped =
        static_cast<tescomponents::TESBipedModel_Component*>(
            const_cast<ArmorRecord*>(this)->components.findByName(QStringLiteral("TESBipedModel")));
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
        case 'EDID':
            esm.writeSubZString('EDID', editorId);
            break;
        case 'FLAG':
            writeWidthU32(NAME('FLAG'), flags, flagWidth, flagExtra);
            break;
        case 'DNAM':
            writeWidthU32(NAME('DNAM'), armorRating, dnamWidth, dnamExtra);
            break;
        case 'DATA':
            if (hasData)
            {
                esm.startSubRecord('DATA');
                esm.writeType<quint32>(value);
                esm.writeType<float>(weight);
                if (!dataExtra.isEmpty())
                    esm.writeRawData(dataExtra.constData(), dataExtra.size());
                esm.endSubRecord();
            }
            break;
        default:
        {
            // INDX/BMDT repeat; non-last occurrences replay verbatim, the
            // last goes through the component so edits land there.
            if ((sub == NAME('INDX') || sub == NAME('BMDT')) && biped)
            {
                const bool isIndx = (sub == NAME('INDX'));
                const QVector<QByteArray>& raws =
                    isIndx ? biped->indxRaws : biped->bmdtRaws;
                int& cur = isIndx ? indxCur : bmdtCur;
                const int k = cur++;
                if (k >= 0 && k < raws.size() - 1)
                    esm.writeRawSubRecord(RawSubRecord{ sub, raws[k] });
                else
                    biped->writeSubrecord(sub, esm);
                break;
            }
            // MODL/MNAM repeat (ARMO biped slots); non-last occurrences
            // replay verbatim, the last goes through the component so edits
            // land there.
            if ((sub == NAME('MODL') || sub == NAME('MNAM')) && model)
            {
                const bool isModl = (sub == NAME('MODL'));
                const QVector<QByteArray>& raws =
                    isModl ? model->modlRaws : model->mnamRaws;
                int& cur = isModl ? modlCur : mnamCur;
                const int k = cur++;
                if (k >= 0 && k < raws.size() - 1)
                    esm.writeRawSubRecord(RawSubRecord{ sub, raws[k] });
                else
                    model->writeSubrecord(sub, esm);
                break;
            }
            bool done = false;
            for (auto& c : components.all())
            {
                if (c->canHandle(sub))
                {
                    done = c->writeSubrecord(sub, esm);
                    break;
                }
            }
            if (!done && rawCur < rawSubRecords.size())
                esm.writeRawSubRecord(rawSubRecords[rawCur++]);
            break;
        }
        }
    }

    // Fields carrying values but absent from the load order (fresh edits on
    // a loaded record) are appended; anything else stays unemitted.
    if (!seen.contains(NAME('FLAG')) && (hasFlag || flags != 0))
        writeWidthU32(NAME('FLAG'), flags, flagWidth, flagExtra);
    if (!seen.contains(NAME('DNAM')) && (hasDnam || armorRating != 0))
        writeWidthU32(NAME('DNAM'), armorRating, dnamWidth, dnamExtra);
    if (!seen.contains(NAME('DATA')) && (hasData || value != 0 || weight != 0.0f))
    {
        esm.startSubRecord('DATA');
        esm.writeType<quint32>(value);
        esm.writeType<float>(weight);
        if (!dataExtra.isEmpty())
            esm.writeRawData(dataExtra.constData(), dataExtra.size());
        esm.endSubRecord();
    }
    auto leftoverComponent = [&](const QString& className, const QVector<NAME>& names) {
        const Component* comp = components.findByName(className);
        if (!comp)
            return;
        for (NAME name : names)
        {
            if (!seen.contains(name))
                comp->writeSubrecord(name, esm);
        }
    };
    leftoverComponent(QStringLiteral("TESModel"),
        QVector<NAME>({ NAME('MODL'), NAME('MNAM') }));
    leftoverComponent(QStringLiteral("TESTexture"),
        QVector<NAME>({ NAME('ICON'), NAME('ICO2') }));
    leftoverComponent(QStringLiteral("TESBipedModel"),
        QVector<NAME>({ NAME('BNAM'), NAME('INDX'), NAME('FNAM') }));
    leftoverComponent(QStringLiteral("TESEnchantableForm"),
        QVector<NAME>({ NAME('ENAM') }));
    leftoverComponent(QStringLiteral("BGSPickupPutdownSounds"),
        QVector<NAME>({ NAME('YNAM'), NAME('ZNAM') }));

    while (rawCur < rawSubRecords.size())
        esm.writeRawSubRecord(rawSubRecords[rawCur++]);
}

void ArmorRecord::blank()
{
    editorId.clear();
    fullName.clear();
    formId = 0;
    flags = 0;
    armorRating = 0;
    weight = 0.0f;
    value = 0;
    iconPath.clear();
    modelPath.clear();
    health = 0.0f;
    loadOrder.clear();
    loadIsRaw.clear();
    hasEdid = false;
    hasFlag = false;
    flagWidth = 4;
    flagExtra.clear();
    hasDnam = false;
    dnamWidth = 4;
    dnamExtra.clear();
    hasData = false;
    dataExtra.clear();
    rawSubRecords.clear();
    verbatimBody.clear();
    verbatimFlags = 0;
    verbatimSnapshot.reset();
    initComponents();
}
