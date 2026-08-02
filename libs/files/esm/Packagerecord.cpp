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
                quint32 count = esm.readType<quint32>();
                targetIds.resize(count);
                for (quint32 i = 0; i < count; i++)
                    targetIds[i] = esm.readType<quint32>();
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
    esm.startSubRecord('PTDT');
    esm.writeType<quint32>(targetIds.size());
    for (quint32 id : targetIds)
        esm.writeType<quint32>(id);
    esm.endSubRecord();

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
