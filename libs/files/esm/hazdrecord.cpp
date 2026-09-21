#include "hazdrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tesfullname.hpp"

#include <QSet>

void HazdRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFullName_Component>();
    components.add<tescomponents::TESModel_Component>();
}

void HazdRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    loadOrder.clear();
    loadIsRaw.clear();
    hasEdid = false;
    hasData = false;
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;
        loadOrder.append(sub);
        bool handled = false;
        for (auto& c : components.all())
        {
            if (c->canHandle(sub))
            {
                c->handleSubrecord(sub, esm);
                handled = true;
                break;
            }
        }
        if (handled) { loadIsRaw.append(0); continue; }

        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); hasEdid = true; loadIsRaw.append(0); break;
            case 'DATA':
            {
                const quint32 size = static_cast<quint32>(esm.subLeft());
                if (size >= 18)
                {
                    limit = esm.readType<quint8>();
                    esm.skip(1);
                    radius = esm.readType<float>();
                    lifetime = esm.readType<float>();
                    imageSpace = esm.readType<quint32>();
                    target = esm.readType<quint8>();
                    esm.skip(1);
                    flags = esm.readType<quint8>();
                    esm.skip(1);
                    hasData = true;
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
    auto* model = static_cast<tescomponents::TESModel_Component*>(components.findByName(QStringLiteral("TESModel")));
    if (model) modelPath = model->modelPath;
}

void HazdRecord::save(ESMWriter& esm) const
{
    auto* model = static_cast<tescomponents::TESModel_Component*>(const_cast<HazdRecord*>(this)->components.findByName(QStringLiteral("TESModel")));
    if (model) model->modelPath = modelPath;

    const auto writeData = [&] {
        QByteArray data;
        data.append(static_cast<char>(limit));
        data.append('\0');
        data.append(reinterpret_cast<const char*>(&radius), 4);
        data.append(reinterpret_cast<const char*>(&lifetime), 4);
        data.append(reinterpret_cast<const char*>(&imageSpace), 4);
        data.append(static_cast<char>(target));
        data.append('\0');
        data.append(static_cast<char>(flags));
        data.append('\0');
        esm.startSubRecord('DATA');
        esm.writeRawData(data.constData(), data.size());
        esm.endSubRecord();
    };

    if (loadOrder.isEmpty())
    {
        if (hasEdid || !editorId.isEmpty())
            esm.writeSubZString('EDID', editorId);
        components.saveAll(esm);
        // Assembled records always carry DATA (it is mandatory for HAZD).
        writeData();
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
        if (p >= loadIsRaw.size() || loadIsRaw[p] != 0)
        {
            if (rawCur < rawSubRecords.size())
                esm.writeRawSubRecord(rawSubRecords[rawCur++]);
            continue;
        }
        switch (sub)
        {
        case 'EDID': esm.writeSubZString('EDID', editorId); break;
        case 'DATA': if (hasData) writeData(); break;
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

    if (!seen.contains(NAME('DATA')) && hasData)
        writeData();
    const Component* modelC = components.findByName(QStringLiteral("TESModel"));
    if (modelC && !seen.contains(NAME('MODL')))
        modelC->writeSubrecord(NAME('MODL'), esm);
    while (rawCur < rawSubRecords.size())
        esm.writeRawSubRecord(rawSubRecords[rawCur++]);
}

void HazdRecord::blank()
{
    editorId.clear();
    formId = 0;
    modelPath.clear();
    limit = 0;
    radius = 0.0f;
    lifetime = 0.0f;
    imageSpace = 0;
    target = 0;
    flags = 0;
    loadOrder.clear();
    loadIsRaw.clear();
    hasEdid = false;
    hasData = false;
    rawSubRecords.clear();
    verbatimBody.clear();
    verbatimFlags = 0;
    verbatimSnapshot.reset();
    initComponents();
}
