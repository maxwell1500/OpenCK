#include "Inforecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier3_components.hpp"

#include <QHash>

void InfoRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFlags_Component>();
}

void InfoRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    loadOrder.clear();
    conditionOrder.clear();
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
        if (handled) continue;

        switch (sub)
        {
            case 'CNAM': responseText = esm.readZString(); hasCnam = true; break;
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
                break;
            }
            case 'TLOI':
            {
                hasTloi = true;
                if (esm.subLeft() >= 4)
                    targetId = esm.readType<quint32>();
                if (esm.subLeft() > 0)
                    esm.skip(static_cast<int>(esm.subLeft()));
                break;
            }
            case 'VMAP':
                voiceFile = esm.readZString();
                hasVmap = true;
                break;
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
    auto* flagsComp = static_cast<tescomponents::TESFlags_Component*>(components.findByName(QStringLiteral("TESFlags")));
    if (flagsComp) {
        flags = flagsComp->flags;
    }
}

void InfoRecord::save(ESMWriter& esm) const
{
    auto* flagsComp = static_cast<tescomponents::TESFlags_Component*>(const_cast<InfoRecord*>(this)->components.findByName(QStringLiteral("TESFlags")));
    if (flagsComp) {
        flagsComp->flags = flags;
    }

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
    QHash<NAME, QVector<int>> rawByName;
    for (int i = 0; i < rawSubRecords.size(); ++i)
        rawByName[rawSubRecords[i].name].append(i);
    QHash<NAME, int> rawCursor;
    const auto writeRawByName = [&](NAME name) -> bool
    {
        const QVector<int>& idx = rawByName[name];
        int& cur = rawCursor[name];
        if (cur < idx.size())
        {
            esm.writeRawSubRecord(rawSubRecords[idx[cur++]]);
            return true;
        }
        return false;
    };

    if (loadOrder.isEmpty())
    {
        // Assembled record (no on-disk order): emit the standard fields.
        if (flagsComp) flagsComp->save(esm);
        if (hasCnam || !responseText.isEmpty())
            esm.writeSubZString('CNAM', responseText);
        if (hasTloi || targetId != 0)
            esm.writeSubData<quint32>('TLOI', targetId);
        if (hasVmap || !voiceFile.isEmpty())
            esm.writeSubZString('VMAP', voiceFile);
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

    int parsedIdx = 0;
    int ctdaSeen = 0;
    for (NAME sub : loadOrder)
    {
        switch (sub)
        {
        case 'FNAM': case 'FLAG':
            if (flagsComp)
                flagsComp->save(esm);
            break;
        case 'CNAM':
            if (hasCnam || !responseText.isEmpty())
                esm.writeSubZString('CNAM', responseText);
            break;
        case 'TLOI':
            if (hasTloi || targetId != 0)
                esm.writeSubData<quint32>('TLOI', targetId);
            break;
        case 'VMAP':
            if (hasVmap || !voiceFile.isEmpty())
                esm.writeSubZString('VMAP', voiceFile);
            break;
        case 'CTDA':
            if (ctdaSeen < conditionOrder.size())
                writeCondition(conditionOrder[ctdaSeen], parsedIdx++);
            else
                writeCondition(-1, parsedIdx++);
            ++ctdaSeen;
            break;
        default:
            writeRawByName(sub);
            break;
        }
    }

    // Records built in memory may carry fields with no load order entry.
    if (!hasCnam && !responseText.isEmpty())
        esm.writeSubZString('CNAM', responseText);
    if (!hasTloi && targetId != 0)
        esm.writeSubData<quint32>('TLOI', targetId);
    if (!hasVmap && !voiceFile.isEmpty())
        esm.writeSubZString('VMAP', voiceFile);
    for (int i = 0; i < conditions.size(); ++i)
    {
        if (i < parsedIdx)
            continue;
        const QByteArray bytes = conditions[i].pack();
        esm.startSubRecord('CTDA');
        esm.writeRawData(bytes.constData(), bytes.size());
        esm.endSubRecord();
    }
}

void InfoRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    responseText.clear();
    voiceFile.clear();
    conditionIds.clear();
    conditions.clear();
    scriptFragment.clear();
    targetId = 0;
    scriptIds.clear();
    rawSubRecords.clear();
    loadOrder.clear();
    conditionOrder.clear();
    hasCnam = false;
    hasTloi = false;
    hasVmap = false;
    initComponents();
}
