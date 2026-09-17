#include "scenrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "conditionrecord.hpp"

#include <QHash>

void ScenRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    loadOrder.clear();
    conditionOrder.clear();

    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;
        loadOrder.append(sub);

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
                conditionOrder.append(-1);
            }
            else
            {
                const QVector<CtdaCondition> parsed = CtdaCondition::unpackList(bytes);
                if (!parsed.isEmpty())
                {
                    for (const CtdaCondition& c : parsed)
                    {
                        conditions.append(c);
                        conditionOrder.append(-1);
                    }
                }
                else
                {
                    RawSubRecord raw;
                    raw.name = sub;
                    raw.data = bytes;
                    rawSubRecords.push_back(raw);
                    conditionOrder.append(rawSubRecords.size() - 1);
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
    const auto writeCondition = [&](int conditionRawIndex, int parsedIndex) -> bool
    {
        if (conditionRawIndex >= 0)
        {
            if (conditionRawIndex >= rawSubRecords.size())
                return false;
            esm.writeRawSubRecord(rawSubRecords[conditionRawIndex]);
            return true;
        }
        if (parsedIndex >= conditions.size())
            return false;
        const QByteArray bytes = conditions[parsedIndex].pack();
        esm.startSubRecord('CTDA');
        esm.writeRawData(bytes.constData(), bytes.size());
        esm.endSubRecord();
        return true;
    };

    if (loadOrder.isEmpty())
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
            esm.writeRawSubRecord(raw);
        return;
    }

    QHash<NAME, QVector<int>> rawByName;
    for (int i = 0; i < rawSubRecords.size(); ++i)
        rawByName[rawSubRecords[i].name].append(i);
    QHash<NAME, int> rawCursor;

    int parsedIdx = 0;
    int ctdaSeen = 0;
    for (NAME sub : loadOrder)
    {
        switch (sub)
        {
        case 'EDID':
            esm.writeSubZString('EDID', editorId);
            break;
        case 'CTDA':
            if (ctdaSeen < conditionOrder.size())
                writeCondition(conditionOrder[ctdaSeen], parsedIdx++);
            else
                writeCondition(-1, parsedIdx++);
            ++ctdaSeen;
            break;
        default:
        {
            const QVector<int>& idx = rawByName[sub];
            int& cur = rawCursor[sub];
            if (cur < idx.size())
                esm.writeRawSubRecord(rawSubRecords[idx[cur++]]);
            break;
        }
        }
    }
}

void ScenRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    conditions.clear();
    rawSubRecords.clear();
    loadOrder.clear();
    conditionOrder.clear();
    components.clear();
}
