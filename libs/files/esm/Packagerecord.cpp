#include "Packagerecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "subrecordreplay.hpp"
#include "../../components/tier3_components.hpp"
#include "conditionrecord.hpp"

#include <QHash>

namespace
{
quint32 leU32(const QByteArray& b, int offset)
{
    quint32 v = 0;
    const int n = qMin(4, b.size() - offset);
    for (int i = 0; i < n; ++i)
        v |= quint32(static_cast<quint8>(b.at(offset + i))) << (8 * i);
    return v;
}
}

void PackageRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFlags_Component>();
}

void PackageRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    loadOrder.clear();
    conditionOrder.clear();
    pkdtRaw.clear();
    pldtRaw.clear();
    ptdtRaws.clear();
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
            case 'EDID': editorId = esm.readZString(); break;
            case 'PKDT':
                esm.readRawSubData(pkdtRaw);
                packageType = leU32(pkdtRaw, 0);
                break;
            case 'PLDT':
                esm.readRawSubData(pldtRaw);
                targetType = leU32(pldtRaw, 0);
                break;
            case 'PTDT':
            {
                QByteArray bytes;
                esm.readRawSubData(bytes);
                ptdtRaws.append(bytes);
                if (bytes.size() >= 8)
                    targetIds.append(leU32(bytes, 4));
                else if (bytes.size() >= 4)
                    targetIds.append(leU32(bytes, 0));
                break;
            }
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
    if (auto* f = static_cast<tescomponents::TESFlags_Component*>(components.findByName(QStringLiteral("TESFlags"))))
        flags = f->flags;
}

void PackageRecord::save(ESMWriter& esm) const
{
    SubrecordReplay replay;
    replay.init(rawSubRecords);

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
    const auto writePkdt = [&]()
    {
        if (!pkdtRaw.isEmpty() && leU32(pkdtRaw, 0) == packageType)
            esm.writeRawSubRecord(RawSubRecord{ NAME('PKDT'), pkdtRaw });
        else
            esm.writeSubData<quint32>('PKDT', packageType);
    };
    const auto writePldt = [&]()
    {
        if (!pldtRaw.isEmpty() && leU32(pldtRaw, 0) == targetType)
            esm.writeRawSubRecord(RawSubRecord{ NAME('PLDT'), pldtRaw });
        else
            esm.writeSubData<quint32>('PLDT', targetType);
    };

    bool wroteEdid = false, wrotePkdt = false, wrotePldt = false;
    int parsedIdx = 0, ctdaSeen = 0, ptdtSeen = 0;
    for (NAME sub : loadOrder)
    {
        switch (sub)
        {
            case 'EDID':
                if (!wroteEdid) { esm.writeSubZString('EDID', editorId); wroteEdid = true; }
                break;
            case 'PKDT':
                if (!wrotePkdt) { writePkdt(); wrotePkdt = true; }
                break;
            case 'PLDT':
                if (!wrotePldt) { writePldt(); wrotePldt = true; }
                break;
            case 'PTDT':
                if (ptdtSeen < ptdtRaws.size()
                    && ptdtSeen < targetIds.size()
                    && leU32(ptdtRaws[ptdtSeen], 4) == targetIds[ptdtSeen])
                {
                    esm.writeRawSubRecord(RawSubRecord{ NAME('PTDT'), ptdtRaws[ptdtSeen] });
                }
                else if (ptdtSeen < targetIds.size())
                {
                    esm.startSubRecord('PTDT');
                    esm.writeType<quint32>(0);
                    esm.writeType<quint32>(targetIds[ptdtSeen]);
                    esm.writeType<qint32>(1);
                    esm.endSubRecord();
                }
                ++ptdtSeen;
                break;
            case 'FNAM': case 'FLAG':
                components.writeSubrecord(sub, esm);
                break;
            case 'CTDA':
                if (ctdaSeen < conditionOrder.size())
                    writeCondition(conditionOrder[ctdaSeen], parsedIdx++);
                else
                    writeCondition(-1, parsedIdx++);
                ++ctdaSeen;
                break;
            default:
                if (!components.writeSubrecord(sub, esm))
                    replay.write(sub, esm);
                break;
        }
    }

    if (!wroteEdid && !editorId.isEmpty())
        esm.writeSubZString('EDID', editorId);
    if (!wrotePkdt)
        writePkdt();
    if (!wrotePldt)
        writePldt();
    for (int i = ptdtSeen; i < targetIds.size(); ++i)
    {
        esm.startSubRecord('PTDT');
        esm.writeType<quint32>(0);
        esm.writeType<quint32>(targetIds[i]);
        esm.writeType<qint32>(1);
        esm.endSubRecord();
    }
    for (int i = parsedIdx; i < conditions.size(); ++i)
    {
        const QByteArray bytes = conditions[i].pack();
        esm.startSubRecord('CTDA');
        esm.writeRawData(bytes.constData(), bytes.size());
        esm.endSubRecord();
    }

    replay.writeLeftover(esm);
}

void PackageRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    packageType = 0;
    targetType = 0;
    targetIds.clear();
    parameters.clear();
    conditions.clear();
    rawSubRecords.clear();
    loadOrder.clear();
    conditionOrder.clear();
    pkdtRaw.clear();
    pldtRaw.clear();
    ptdtRaws.clear();
    initComponents();
}