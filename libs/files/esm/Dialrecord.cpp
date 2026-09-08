#include "Dialrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier3_components.hpp"
#include <cstring>

void DialRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFlags_Component>();
}

void DialRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
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
        if (handled) continue;

        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FULL': topicName = esm.readZString(); break;
            case 'INAM':
            {
                QByteArray data;
                esm.readRawSubData(data);
                hasInam = true;
                responseIds.clear();
                for (int i = 0; i + 4 <= data.size(); i += 4)
                {
                    quint32 id;
                    memcpy(&id, data.constData() + i, 4);
                    responseIds.append(id);
                }
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
    if (auto* f = static_cast<tescomponents::TESFlags_Component*>(components.findByName(QStringLiteral("TESFlags")))) { flags = f->flags; }
}

void DialRecord::save(ESMWriter& esm) const
{
    if (auto* f = static_cast<tescomponents::TESFlags_Component*>(const_cast<DialRecord*>(this)->components.findByName(QStringLiteral("TESFlags")))) { f->flags = flags; }

    esm.writeSubZString('EDID', editorId);
    if (!topicName.isEmpty())
        esm.writeSubZString('FULL', topicName);
    if (hasInam)
    {
        RawSubRecord raw;
        raw.name = 'INAM';
        raw.data.resize(responseIds.size() * 4);
        for (int i = 0; i < responseIds.size(); ++i)
        {
            memcpy(raw.data.data() + i * 4, &responseIds[i], 4);
        }
        esm.writeRawSubRecord(raw);
    }
    components.saveAll(esm);

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
    }
}

void DialRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    topicName = "";
    responseIds.clear();
    conditionIds.clear();
    animationIds.clear();
    emotionIds.clear();
    hasInam = false;
    rawSubRecords.clear();
    initComponents();
}
