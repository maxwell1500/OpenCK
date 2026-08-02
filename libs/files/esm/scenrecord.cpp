#include "scenrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "conditionrecord.hpp"

void ScenRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();

    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;

        bool handled = false;
        switch (sub)
        {
        case 'EDID': editorId = esm.readZString(); handled = true; break;
        case 'CTDA':
        {
            QByteArray bytes;
            esm.readRawSubData(bytes);
            CtdaCondition condition;
            if (CtdaCondition::unpack(bytes, condition))
            {
                conditions.append(condition);
            }
            else
            {
                const QVector<CtdaCondition> parsed = CtdaCondition::unpackList(bytes);
                if (!parsed.isEmpty())
                    conditions.append(parsed);
                else
                {
                    RawSubRecord raw;
                    raw.name = sub;
                    raw.data = bytes;
                    rawSubRecords.push_back(raw);
                }
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
}

void ScenRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    components.saveAll(esm);

    for (const CtdaCondition& condition : conditions)
    {
        const QByteArray bytes = condition.pack();
        esm.startSubRecord('CTDA');
        esm.writeRawData(bytes.constData(), bytes.size());
        esm.endSubRecord();
    }

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void ScenRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    conditions.clear();
    rawSubRecords.clear();
    components.clear();
}
