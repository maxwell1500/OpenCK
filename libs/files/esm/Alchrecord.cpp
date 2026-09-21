#include "Alchrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tier2_components.hpp"

#include <QSet>

void AlchRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESModel_Component>();
    components.add<tescomponents::TESTexture_Component>();
    components.add<tescomponents::BGSPickupPutdownSounds_Component>();
}

void AlchRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    loadOrder.clear();
    loadIsRaw.clear();
    hasEdid = false;
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
            case 'FNAM': case 'FLAG': flags = esm.readType<quint32>(); hasFlags = true; loadIsRaw.append(0); break;
            case 'DATA': {
                // Width-guarded: a short DATA must not over-read into the
                // following subrecord (that desyncs the stored raws and the
                // save then emits garbage).
                dataFields = 0;
                if (esm.subLeft() >= 4) { weight = esm.readType<float>(); ++dataFields; }
                if (esm.subLeft() >= 4) { value = esm.readType<quint32>(); ++dataFields; }
                hasData = true;
                loadIsRaw.append(0);
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
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(components.findByName(QStringLiteral("TESTexture")));
    if (tex) iconPath = tex->iconPath;
    auto* model = static_cast<tescomponents::TESModel_Component*>(components.findByName(QStringLiteral("TESModel")));
    if (model) modelPath = model->modelPath;
}

void AlchRecord::save(ESMWriter& esm) const
{
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(const_cast<AlchRecord*>(this)->components.findByName(QStringLiteral("TESTexture")));
    if (tex) tex->iconPath = iconPath;
    auto* model = static_cast<tescomponents::TESModel_Component*>(const_cast<AlchRecord*>(this)->components.findByName(QStringLiteral("TESModel")));
    if (model) model->modelPath = modelPath;

    const auto writeData = [&] {
        esm.startSubRecord('DATA');
        if (dataFields > 0) esm.writeType<float>(weight);
        if (dataFields > 1) esm.writeType<quint32>(value);
        esm.endSubRecord();
    };

    if (loadOrder.isEmpty())
    {
        if (hasEdid || !editorId.isEmpty())
            esm.writeSubZString('EDID', editorId);
        if (hasFlags || flags != 0)
            esm.writeSubData<quint32>('FNAM', flags);
        components.saveAll(esm);
        if (hasData || weight != 0.0f || value != 0)
            writeData();
        for (const auto& raw : rawSubRecords)
            esm.writeRawSubRecord(raw);
        return;
    }

    QSet<NAME> seen;
    int rawCur = 0;
    int mnamCur = 0;
    int ynamCur = 0;
    int znamCur = 0;
    tescomponents::TESModel_Component* modelComp =
        static_cast<tescomponents::TESModel_Component*>(
            const_cast<AlchRecord*>(this)->components.findByName(QStringLiteral("TESModel")));
    tescomponents::BGSPickupPutdownSounds_Component* soundComp =
        static_cast<tescomponents::BGSPickupPutdownSounds_Component*>(
            const_cast<AlchRecord*>(this)->components.findByName(QStringLiteral("BGSPickupPutdownSounds")));
    const auto writeOccurrence = [&](NAME sub, const QVector<QByteArray>& raws, int& cur,
                                     Component* comp) {
        const int k = cur++;
        if (k >= 0 && k < raws.size() - 1)
            esm.writeRawSubRecord(RawSubRecord{ sub, raws[k] });
        else if (comp)
            comp->writeSubrecord(sub, esm);
    };
    for (int p = 0; p < loadOrder.size(); ++p)
    {
        const NAME sub = loadOrder[p];
        seen.insert(sub);
        if (p >= loadIsRaw.size() || loadIsRaw[p] != 0)
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
        case 'MNAM':
            if (modelComp)
                writeOccurrence(sub, modelComp->mnamRaws, mnamCur, modelComp);
            else
                esm.writeRawSubRecord(rawSubRecords[rawCur++]);
            break;
        case 'ZNAM':
            if (soundComp)
                writeOccurrence(sub, soundComp->znamRaws, znamCur, soundComp);
            else
                esm.writeRawSubRecord(rawSubRecords[rawCur++]);
            break;
        case 'YNAM': case 'PICK':
            if (soundComp)
                writeOccurrence(sub, soundComp->ynamRaws, ynamCur, soundComp);
            else
                esm.writeRawSubRecord(rawSubRecords[rawCur++]);
            break;
        default:
        {
            bool done = false;
            for (auto& c : components.all())
                if (c->canHandle(sub)) { done = c->writeSubrecord(sub, esm); break; }
            if (!done && rawCur < rawSubRecords.size())
                esm.writeRawSubRecord(rawSubRecords[rawCur++]);
            break;
        }
        }
    }

    if (!seen.contains(NAME('FNAM')) && !seen.contains(NAME('FLAG')) && (hasFlags || flags != 0))
        esm.writeSubData<quint32>('FNAM', flags);
    if (!seen.contains(NAME('DATA')) && (hasData || weight != 0.0f || value != 0))
        writeData();
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

void AlchRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    iconPath.clear();
    modelPath.clear();
    weight = 0.0f;
    value = 0;
    dataFields = 2;
    hasData = false;
    hasFlags = false;
    loadOrder.clear();
    loadIsRaw.clear();
    hasEdid = false;
    rawSubRecords.clear();
    verbatimBody.clear();
    verbatimFlags = 0;
    verbatimSnapshot.reset();
    initComponents();
}
