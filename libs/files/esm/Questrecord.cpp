#include "Questrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tesfullname.hpp"

void QuestRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFullName_Component>();
}

void QuestRecord::load(ESMReader& esm, bool)
{
    RecHeader rh = esm.readHeader();
    formId = esm.currentFormId();
    flags = rh.flags.val;
    initComponents();
    enum Group { Top, Stage, Objective, Alias };
    Group group = Top;
    auto rawPreserve = [&](NAME n, QVector<RawSubRecord>* dest) {
        RawSubRecord raw;
        raw.name = n;
        esm.readRawSubData(raw.data);
        dest->push_back(raw);
    };
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;
        bool handled = false;
        for (auto& c : components.all())
        {
            if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        }
        if (handled) continue;
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'DNAM': rawPreserve(sub, &rawSubRecords); break;
            case 'CTDA':
                if (group == Objective) rawPreserve(sub, &objectiveExtra.last());
                else if (group == Alias) rawPreserve(sub, &aliasExtra.last());
                else rawPreserve(sub, &rawSubRecords);
                break;
            case 'INDX':
                group = Stage;
                stageIds.append(esm.readType<quint32>());
                stageDescriptions.append(QString());
                stageFlags.append(0);
                stageExtra.append(QVector<RawSubRecord>());
                break;
            case 'QSDT':
                if (group == Stage) stageFlags.last() = esm.readType<quint8>();
                else rawPreserve(sub, &rawSubRecords);
                break;
            case 'CNAM':
                if (group == Stage) stageDescriptions.last() = esm.readZString();
                else if (group == Top) questDesc = esm.readZString();
                else if (group == Alias) rawPreserve(sub, &aliasExtra.last());
                else rawPreserve(sub, &objectiveExtra.last());
                break;
            case 'SCHR': case 'SCDA': case 'SCRO': case 'SCTX': case 'SLSD':
                if (group == Stage) rawPreserve(sub, &stageExtra.last());
                else rawPreserve(sub, &rawSubRecords);
                break;
            case 'QOBJ':
                group = Objective;
                if (esm.subLeft() >= 8)
                {
                    objectiveIds.append(esm.readType<quint32>());
                    objectiveFlags.append(esm.readType<quint32>());
                    objectiveExtra.append(QVector<RawSubRecord>());
                    if (esm.subLeft() > 0) esm.skip(static_cast<int>(esm.subLeft()));
                }
                else rawPreserve(sub, &rawSubRecords);
                break;
            case 'NAM1':
                if (group == Top) dialogueView = esm.readZString();
                else if (group == Objective) rawPreserve(sub, &objectiveExtra.last());
                else if (group == Alias) rawPreserve(sub, &aliasExtra.last());
                else rawPreserve(sub, &rawSubRecords);
                break;
            case 'NAM2':
                if (group == Objective) rawPreserve(sub, &objectiveExtra.last());
                else if (group == Alias) rawPreserve(sub, &aliasExtra.last());
                else rawPreserve(sub, &rawSubRecords);
                break;
            case 'ALST':
                group = Alias;
                if (esm.subLeft() >= 8)
                {
                    aliasIds.append(esm.readType<quint32>());
                    aliasFlags.append(esm.readType<quint32>());
                    aliasExtra.append(QVector<RawSubRecord>());
                    if (esm.subLeft() > 0) esm.skip(static_cast<int>(esm.subLeft()));
                }
                else rawPreserve(sub, &rawSubRecords);
                break;
            case 'ALID':
                if (group == Alias) rawPreserve(sub, &aliasExtra.last());
                else rawPreserve(sub, &rawSubRecords);
                break;
            default:
                if (group == Alias) rawPreserve(sub, &aliasExtra.last());
                else rawPreserve(sub, &rawSubRecords);
                break;
        }
    }
    auto* fn = static_cast<tescomponents::TESFullName_Component*>(components.findByName(QStringLiteral("TESFullName")));
    if (fn) questName = fn->fullName;
}

void QuestRecord::save(ESMWriter& esm) const
{
    auto* fn = const_cast<QuestRecord*>(this)->components.findByName(QStringLiteral("TESFullName"));
    if (fn) static_cast<tescomponents::TESFullName_Component*>(fn)->fullName = questName;

    esm.writeSubZString('EDID', editorId);
    components.saveAll(esm);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }

    for (int i = 0; i < stageIds.size(); ++i)
    {
        esm.writeSubData<quint32>('INDX', stageIds[i]);
        esm.writeSubData<quint8>('QSDT', i < stageFlags.size() ? stageFlags[i] : 0);
        if (i < stageDescriptions.size() && !stageDescriptions[i].isEmpty())
            esm.writeSubZString('CNAM', stageDescriptions[i]);
        if (i < stageExtra.size())
        {
            for (const auto& raw : stageExtra[i])
            {
                esm.startSubRecord(raw.name);
                esm.writeRawData(raw.data.data(), raw.data.size());
                esm.endSubRecord();
            }
        }
    }

    for (int i = 0; i < objectiveIds.size(); ++i)
    {
        esm.startSubRecord('QOBJ');
        esm.writeType<quint32>(objectiveIds[i]);
        esm.writeType<quint32>(i < objectiveFlags.size() ? objectiveFlags[i] : 0);
        esm.endSubRecord();
        if (i < objectiveExtra.size())
        {
            for (const auto& raw : objectiveExtra[i])
            {
                esm.startSubRecord(raw.name);
                esm.writeRawData(raw.data.data(), raw.data.size());
                esm.endSubRecord();
            }
        }
    }

    for (int i = 0; i < aliasIds.size(); ++i)
    {
        esm.startSubRecord('ALST');
        esm.writeType<quint32>(aliasIds[i]);
        esm.writeType<quint32>(i < aliasFlags.size() ? aliasFlags[i] : 0);
        esm.endSubRecord();
        if (i < aliasExtra.size())
        {
            for (const auto& raw : aliasExtra[i])
            {
                esm.startSubRecord(raw.name);
                esm.writeRawData(raw.data.data(), raw.data.size());
                esm.endSubRecord();
            }
        }
    }
}

void QuestRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    questName = "";
    questDesc = "";
    questType = 0;
    stageIds.clear();
    stageDescriptions.clear();
    objectiveIds.clear();
    aliasIds.clear();
    dialogueView = "";
    scriptIds.clear();
    stageFlags.clear();
    objectiveFlags.clear();
    aliasFlags.clear();
    stageExtra.clear();
    objectiveExtra.clear();
    aliasExtra.clear();
    rawSubRecords.clear();
    initComponents();
}
