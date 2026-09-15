#include "Alchrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tier2_components.hpp"

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
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        bool handled = false;
        for (auto& c : components.all())
        {
            if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        }
        if (handled) continue;
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': case 'FLAG': flags = esm.readType<quint32>(); hasFlags = true; break;
            case 'DATA': {
                // Width-guarded: a short DATA must not over-read into the
                // following subrecord (that desyncs the stored raws and the
                // save then emits garbage).
                dataFields = 0;
                if (esm.subLeft() >= 4) { weight = esm.readType<float>(); ++dataFields; }
                if (esm.subLeft() >= 4) { value = esm.readType<quint32>(); ++dataFields; }
                hasData = true;
                break;
            }
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

void AlchRecord::save(ESMWriter& esm) const
{
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(const_cast<AlchRecord*>(this)->components.findByName(QStringLiteral("TESTexture")));
    if (tex) tex->iconPath = iconPath;
    auto* model = static_cast<tescomponents::TESModel_Component*>(const_cast<AlchRecord*>(this)->components.findByName(QStringLiteral("TESModel")));
    if (model) model->modelPath = modelPath;

    esm.writeSubZString('EDID', editorId);
    if (hasFlags || flags != 0)
        esm.writeSubData<quint32>('FNAM', flags);
    components.saveAll(esm);
    if (hasData || weight != 0.0f || value != 0)
    {
        esm.startSubRecord('DATA');
        if (dataFields > 0) esm.writeType<float>(weight);
        if (dataFields > 1) esm.writeType<quint32>(value);
        esm.endSubRecord();
    }

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
    }
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
    rawSubRecords.clear();
    initComponents();
}
