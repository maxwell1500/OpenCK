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
    pkdtRaws.clear();
    pldtRaws.clear();
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
                pkdtRaws.append(pkdtRaw);
                packageType = leU32(pkdtRaw, 0);
                hasPkdt = true;
                break;
            case 'PLDT':
                esm.readRawSubData(pldtRaw);
                pldtRaws.append(pldtRaw);
                targetType = leU32(pldtRaw, 0);
                hasPldt = true;
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
    int pkdtCur = 0, pldtCur = 0;
    // FNAM/FLAG repeat (package flags per entry); non-last occurrences replay
    // from the component's occurrence list, the last carries the live value.
    auto* flagsComp = static_cast<tescomponents::TESFlags_Component*>(
        const_cast<PackageRecord*>(this)->components.findByName(QStringLiteral("TESFlags")));
    int flagsCur = 0;
    for (NAME sub : loadOrder)
    {
        switch (sub)
        {
            case 'EDID':
                if (!wroteEdid) { esm.writeSubZString('EDID', editorId); wroteEdid = true; }
                break;
            case 'PKDT':
            {
                const int k = pkdtCur++;
                if (k >= 0 && k < pkdtRaws.size() - 1)
                    esm.writeRawSubRecord(RawSubRecord{ NAME('PKDT'), pkdtRaws[k] });
                else
                    writePkdt();
                wrotePkdt = true;
                break;
            }
            case 'PLDT':
            {
                const int k = pldtCur++;
                if (k >= 0 && k < pldtRaws.size() - 1)
                    esm.writeRawSubRecord(RawSubRecord{ NAME('PLDT'), pldtRaws[k] });
                else
                    writePldt();
                wrotePldt = true;
                break;
            }
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
                if (flagsComp && flagsCur >= 0 && flagsCur < flagsComp->flagsRaws.size() - 1)
                    esm.writeRawSubRecord(RawSubRecord{ sub, flagsComp->flagsRaws[flagsCur] });
                else
                    components.writeSubrecord(sub, esm);
                ++flagsCur;
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
    if (!wrotePkdt && (hasPkdt || packageType != 0 || loadOrder.isEmpty()))
        writePkdt();
    if (!wrotePldt && (hasPldt || targetType != 0 || loadOrder.isEmpty()))
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
    pkdtRaws.clear();
    pldtRaws.clear();
    hasPkdt = false;
    hasPldt = false;
    verbatimBody.clear();
    verbatimFlags = 0;
    verbatimSnapshot.reset();
    initComponents();
}