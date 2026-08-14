#include "Packagerecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier3_components.hpp"
#include "conditionrecord.hpp"

void PackageRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFlags_Component>();
}

void PackageRecord::load(ESMReader& esm, bool)
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
            case 'PKDT': packageType = esm.readType<quint32>(); break;
            case 'PLDT': targetType = esm.readType<quint32>(); break;
            case 'PTDT':
            {
                // Fixed 12-byte target data struct: target mode (u32),
                // target id (u32), count (s32). One subrecord per target.
                if (esm.subLeft() >= 12)
                {
                    esm.readType<quint32>();
                    targetIds.append(esm.readType<quint32>());
                    esm.readType<qint32>();
                }
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

void PackageRecord::save(ESMWriter& esm) const
{
    if (auto* f = static_cast<tescomponents::TESFlags_Component*>(const_cast<PackageRecord*>(this)->components.findByName(QStringLiteral("TESFlags")))) { f->flags = flags; }

    esm.writeSubZString('EDID', editorId);
    components.saveAll(esm);
    esm.writeSubData<quint32>('PKDT', packageType);
    esm.writeSubData<quint32>('PLDT', targetType);
    for (quint32 id : targetIds)
    {
        esm.startSubRecord('PTDT');
        esm.writeType<quint32>(0);
        esm.writeType<quint32>(id);
        esm.writeType<qint32>(1);
        esm.endSubRecord();
    }

    for (const CtdaCondition& condition : conditions)
    {
        const QByteArray bytes = condition.pack();
        esm.startSubRecord('CTDA');
        esm.writeRawData(bytes.constData(), bytes.size());
        esm.endSubRecord();
    }

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
    }
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
    initComponents();
}
