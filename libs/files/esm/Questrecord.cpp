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
    loadOrder.clear();
    loadAction.clear();
    loadIndex.clear();
    stageHasQsdt.clear();
    stageHasCnam.clear();
    stageQsdtVals.clear();
    stageCnamVals.clear();
    stageCnamRaws.clear();
    questDescRaw.clear();
    dialogueViewRaw.clear();
    hasEdid = false;
    enum Group { Top, Stage, Objective, Alias };
    Group group = Top;
    auto pushAction = [&](NAME n, quint8 action, int index) {
        loadOrder.append(n);
        loadAction.append(action);
        loadIndex.append(index);
    };
    // Best-effort text mirror of a binary-or-LString payload (editors work
    // on text; replay compares against it to detect edits).
    auto parseQuestString = [](const QByteArray& bytes) {
        QString s = QString::fromUtf8(bytes.constData(), bytes.size());
        while (s.endsWith(QChar(0)))
            s.chop(1);
        return s;
    };
    auto rawPreserve = [&](NAME n, QVector<RawSubRecord>* dest) {
        RawSubRecord raw;
        raw.name = n;
        esm.readRawSubData(raw.data);
        dest->push_back(raw);
    };
    // Preserve a subrecord into the last entry of a per-stage/objective/alias
    // extra vector. If that group has no current entry (the subrecord arrived
    // before any INDX/QOBJ/ALST), fall back to the top-level raw list so we
    // never call QVector::last() on an empty vector. Returns true when the
    // subrecord went to the group extra, false on top fallback.
    auto rawPreserveGroup = [&](NAME n, QVector<QVector<RawSubRecord>>& outer) {
        if (!outer.isEmpty())
        {
            RawSubRecord raw;
            raw.name = n;
            esm.readRawSubData(raw.data);
            outer.last().push_back(raw);
            return true;
        }
        rawPreserve(n, &rawSubRecords);
        return false;
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
        if (handled) { pushAction(sub, QRA_Component, -1); continue; }
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); hasEdid = true; pushAction(sub, QRA_Edid, -1); break;
            case 'DNAM': rawPreserve(sub, &rawSubRecords); pushAction(sub, QRA_TopRaw, -1); break;
            case 'CTDA':
                if (group == Objective)
                {
                    if (rawPreserveGroup(sub, objectiveExtra))
                        pushAction(sub, QRA_ObjectiveExtra, objectiveExtra.size() - 1);
                    else
                        pushAction(sub, QRA_TopRaw, -1);
                }
                else if (group == Alias)
                {
                    if (rawPreserveGroup(sub, aliasExtra))
                        pushAction(sub, QRA_AliasExtra, aliasExtra.size() - 1);
                    else
                        pushAction(sub, QRA_TopRaw, -1);
                }
                else { rawPreserve(sub, &rawSubRecords); pushAction(sub, QRA_TopRaw, -1); }
                break;
            case 'INDX':
                group = Stage;
                stageIds.append(esm.readType<quint32>());
                stageDescriptions.append(QString());
                stageFlags.append(0);
                stageHasQsdt.append(0);
                stageHasCnam.append(0);
                stageQsdtVals.append(QVector<quint8>());
                stageCnamVals.append(QVector<QString>());
                stageCnamRaws.append(QVector<QByteArray>());
                stageExtra.append(QVector<RawSubRecord>());
                pushAction(sub, QRA_StageIndex, stageIds.size() - 1);
                break;
            case 'QSDT':
                if (group == Stage && !stageFlags.isEmpty())
                {
                    const quint8 v = esm.readType<quint8>();
                    stageFlags.last() = v;
                    stageQsdtVals.last().append(v);
                    stageHasQsdt.last() = 1;
                    pushAction(sub, QRA_StageQsdt, stageFlags.size() - 1);
                }
                else { rawPreserve(sub, &rawSubRecords); pushAction(sub, QRA_TopRaw, -1); }
                break;
            case 'CNAM':
                if (group == Stage && !stageDescriptions.isEmpty())
                {
                    QByteArray bytes;
                    esm.readRawSubData(bytes);
                    stageCnamRaws.last().append(bytes);
                    stageDescriptions.last() = parseQuestString(bytes);
                    stageCnamVals.last().append(stageDescriptions.last());
                    stageHasCnam.last() = 1;
                    pushAction(sub, QRA_StageCnam, stageDescriptions.size() - 1);
                }
                else if (group == Top)
                {
                    esm.readRawSubData(questDescRaw);
                    questDesc = parseQuestString(questDescRaw);
                    pushAction(sub, QRA_QuestDesc, -1);
                }
                else if (group == Alias)
                {
                    if (rawPreserveGroup(sub, aliasExtra))
                        pushAction(sub, QRA_AliasExtra, aliasExtra.size() - 1);
                    else
                        pushAction(sub, QRA_TopRaw, -1);
                }
                else
                {
                    if (rawPreserveGroup(sub, objectiveExtra))
                        pushAction(sub, QRA_ObjectiveExtra, objectiveExtra.size() - 1);
                    else
                        pushAction(sub, QRA_TopRaw, -1);
                }
                break;
            case 'SCHR': case 'SCDA': case 'SCRO': case 'SCTX': case 'SLSD':
                if (group == Stage)
                {
                    if (rawPreserveGroup(sub, stageExtra))
                        pushAction(sub, QRA_StageExtra, stageExtra.size() - 1);
                    else
                        pushAction(sub, QRA_TopRaw, -1);
                }
                else { rawPreserve(sub, &rawSubRecords); pushAction(sub, QRA_TopRaw, -1); }
                break;
            case 'QOBJ':
                group = Objective;
                if (esm.subLeft() >= 8)
                {
                    objectiveIds.append(esm.readType<quint32>());
                    objectiveFlags.append(esm.readType<quint32>());
                    objectiveExtra.append(QVector<RawSubRecord>());
                    if (esm.subLeft() > 0) esm.skip(static_cast<int>(esm.subLeft()));
                    pushAction(sub, QRA_Objective, objectiveIds.size() - 1);
                }
                else { rawPreserve(sub, &rawSubRecords); pushAction(sub, QRA_TopRaw, -1); }
                break;
            case 'NAM1':
                if (group == Top)
                {
                    esm.readRawSubData(dialogueViewRaw);
                    dialogueView = parseQuestString(dialogueViewRaw);
                    pushAction(sub, QRA_DialogueView, -1);
                }
                else if (group == Objective)
                {
                    if (rawPreserveGroup(sub, objectiveExtra))
                        pushAction(sub, QRA_ObjectiveExtra, objectiveExtra.size() - 1);
                    else
                        pushAction(sub, QRA_TopRaw, -1);
                }
                else if (group == Alias)
                {
                    if (rawPreserveGroup(sub, aliasExtra))
                        pushAction(sub, QRA_AliasExtra, aliasExtra.size() - 1);
                    else
                        pushAction(sub, QRA_TopRaw, -1);
                }
                else { rawPreserve(sub, &rawSubRecords); pushAction(sub, QRA_TopRaw, -1); }
                break;
            case 'NAM2':
                if (group == Objective)
                {
                    if (rawPreserveGroup(sub, objectiveExtra))
                        pushAction(sub, QRA_ObjectiveExtra, objectiveExtra.size() - 1);
                    else
                        pushAction(sub, QRA_TopRaw, -1);
                }
                else if (group == Alias)
                {
                    if (rawPreserveGroup(sub, aliasExtra))
                        pushAction(sub, QRA_AliasExtra, aliasExtra.size() - 1);
                    else
                        pushAction(sub, QRA_TopRaw, -1);
                }
                else { rawPreserve(sub, &rawSubRecords); pushAction(sub, QRA_TopRaw, -1); }
                break;
            case 'ALST':
                group = Alias;
                if (esm.subLeft() >= 8)
                {
                    aliasIds.append(esm.readType<quint32>());
                    aliasFlags.append(esm.readType<quint32>());
                    aliasExtra.append(QVector<RawSubRecord>());
                    if (esm.subLeft() > 0) esm.skip(static_cast<int>(esm.subLeft()));
                    pushAction(sub, QRA_Alias, aliasIds.size() - 1);
                }
                else { rawPreserve(sub, &rawSubRecords); pushAction(sub, QRA_TopRaw, -1); }
                break;
            case 'ALID':
                if (group == Alias)
                {
                    if (rawPreserveGroup(sub, aliasExtra))
                        pushAction(sub, QRA_AliasExtra, aliasExtra.size() - 1);
                    else
                        pushAction(sub, QRA_TopRaw, -1);
                }
                else { rawPreserve(sub, &rawSubRecords); pushAction(sub, QRA_TopRaw, -1); }
                break;
            default:
                if (group == Alias)
                {
                    if (rawPreserveGroup(sub, aliasExtra))
                        pushAction(sub, QRA_AliasExtra, aliasExtra.size() - 1);
                    else
                        pushAction(sub, QRA_TopRaw, -1);
                }
                else { rawPreserve(sub, &rawSubRecords); pushAction(sub, QRA_TopRaw, -1); }
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

    const auto parseQuestStringSave = [](const QByteArray& bytes) {
        QString s = QString::fromUtf8(bytes.constData(), bytes.size());
        while (s.endsWith(QChar(0)))
            s.chop(1);
        return s;
    };
    const auto writeStage = [&](int i) {
        esm.writeSubData<quint32>('INDX', stageIds[i]);
        esm.writeSubData<quint8>('QSDT', i < stageFlags.size() ? stageFlags[i] : 0);
        if (i < stageDescriptions.size() && !stageDescriptions[i].isEmpty())
            esm.writeSubZString('CNAM', stageDescriptions[i]);
    };
    const auto writeObjective = [&](int i) {
        esm.startSubRecord('QOBJ');
        esm.writeType<quint32>(objectiveIds[i]);
        esm.writeType<quint32>(i < objectiveFlags.size() ? objectiveFlags[i] : 0);
        esm.endSubRecord();
    };
    const auto writeAlias = [&](int i) {
        esm.startSubRecord('ALST');
        esm.writeType<quint32>(aliasIds[i]);
        esm.writeType<quint32>(i < aliasFlags.size() ? aliasFlags[i] : 0);
        esm.endSubRecord();
    };

    if (loadOrder.isEmpty())
    {
        // Assembled record (no on-disk order): fixed field order. Only emit
        // subrecords the record actually carries (save rule).
        if (hasEdid || !editorId.isEmpty())
            esm.writeSubZString('EDID', editorId);
        components.saveAll(esm);
        if (!questDesc.isEmpty())
            esm.writeSubZString('CNAM', questDesc);
        if (!dialogueView.isEmpty())
            esm.writeSubZString('NAM1', dialogueView);

        for (const auto& raw : rawSubRecords)
        {
            esm.writeRawSubRecord(raw);
        }

        for (int i = 0; i < stageIds.size(); ++i)
        {
            writeStage(i);
            if (i < stageExtra.size())
            {
                for (const auto& raw : stageExtra[i])
                {
                    esm.writeRawSubRecord(raw);
                }
            }
        }

        for (int i = 0; i < objectiveIds.size(); ++i)
        {
            writeObjective(i);
            if (i < objectiveExtra.size())
            {
                for (const auto& raw : objectiveExtra[i])
                {
                    esm.writeRawSubRecord(raw);
                }
            }
        }

        for (int i = 0; i < aliasIds.size(); ++i)
        {
            writeAlias(i);
            if (i < aliasExtra.size())
            {
                for (const auto& raw : aliasExtra[i])
                {
                    esm.writeRawSubRecord(raw);
                }
            }
        }
        return;
    }

    // Positional replay: each loadOrder entry re-emits exactly what the load
    // routed there. Stale indices (entries removed by an editor) are skipped;
    // entries appended after load are emitted by the leftover passes below.
    int topCur = 0;
    QVector<int> stageExtraCur(stageIds.size(), 0);
    QVector<int> objectiveExtraCur(objectiveIds.size(), 0);
    QVector<int> aliasExtraCur(aliasIds.size(), 0);
    QVector<int> stageQsdtCur(stageIds.size(), 0);
    QVector<int> stageCnamCur(stageIds.size(), 0);
    int seenStages = 0;
    int seenObjectives = 0;
    int seenAliases = 0;
    const auto validStage = [&](int i) { return i >= 0 && i < stageIds.size(); };
    const auto validObjective = [&](int i) { return i >= 0 && i < objectiveIds.size(); };
    const auto validAlias = [&](int i) { return i >= 0 && i < aliasIds.size(); };

    for (int p = 0; p < loadOrder.size(); ++p)
    {
        const int idx = p < loadIndex.size() ? loadIndex[p] : -1;
        switch (p < loadAction.size() ? loadAction[p] : QRA_TopRaw)
        {
        case QRA_Edid:
            esm.writeSubZString('EDID', editorId);
            break;
        case QRA_Component:
            components.saveAll(esm);
            break;
        case QRA_TopRaw:
            if (topCur < rawSubRecords.size())
                esm.writeRawSubRecord(rawSubRecords[topCur++]);
            break;
        case QRA_StageIndex:
            if (validStage(idx))
            {
                esm.writeSubData<quint32>('INDX', stageIds[idx]);
                seenStages = qMax(seenStages, idx + 1);
            }
            break;
        case QRA_StageQsdt:
            if (validStage(idx))
            {
                if (idx < stageHasQsdt.size() && stageHasQsdt[idx])
                {
                    // Stored occurrence each time; the editable mirror goes
                    // on the last occurrence (it equals the stored value
                    // unless an editor changed it).
                    const int k = idx < stageQsdtCur.size() ? stageQsdtCur[idx]++ : 0;
                    const int count = idx < stageQsdtVals.size() ? stageQsdtVals[idx].size() : 0;
                    quint8 v = idx < stageFlags.size() ? stageFlags[idx] : 0;
                    if (k != count - 1 && k >= 0 && k < count)
                        v = stageQsdtVals[idx][k];
                    esm.writeSubData<quint8>('QSDT', v);
                }
                seenStages = qMax(seenStages, idx + 1);
            }
            break;
        case QRA_StageCnam:
            if (validStage(idx))
            {
                if (idx < stageHasCnam.size() && stageHasCnam[idx] && idx < stageDescriptions.size())
                {
                    // Binary/LString payloads replay verbatim; a mirror edit
                    // lands on the last occurrence as text.
                    const int k = idx < stageCnamCur.size() ? stageCnamCur[idx]++ : 0;
                    const int count = idx < stageCnamVals.size() ? stageCnamVals[idx].size() : 0;
                    const int rawCount = idx < stageCnamRaws.size() ? stageCnamRaws[idx].size() : 0;
                    const bool isLast = (count <= 0) || (k == count - 1);
                    const bool unedited = isLast && k >= 0 && k < count
                        && stageDescriptions[idx] == stageCnamVals[idx][k];
                    if (k >= 0 && k < rawCount && (unedited || !isLast))
                    {
                        esm.startSubRecord('CNAM');
                        const QByteArray& raw = stageCnamRaws[idx][k];
                        esm.writeRawData(raw.constData(), raw.size());
                        esm.endSubRecord();
                    }
                    else
                    {
                        esm.writeSubZString('CNAM', stageDescriptions[idx]);
                    }
                }
                seenStages = qMax(seenStages, idx + 1);
            }
            break;
        case QRA_Objective:
            if (validObjective(idx))
            {
                writeObjective(idx);
                seenObjectives = qMax(seenObjectives, idx + 1);
            }
            break;
        case QRA_Alias:
            if (validAlias(idx))
            {
                writeAlias(idx);
                seenAliases = qMax(seenAliases, idx + 1);
            }
            break;
        case QRA_StageExtra:
            if (validStage(idx) && idx < stageExtra.size()
                && stageExtraCur[idx] < stageExtra[idx].size())
                esm.writeRawSubRecord(stageExtra[idx][stageExtraCur[idx]++]);
            break;
        case QRA_ObjectiveExtra:
            if (validObjective(idx) && idx < objectiveExtra.size()
                && objectiveExtraCur[idx] < objectiveExtra[idx].size())
                esm.writeRawSubRecord(objectiveExtra[idx][objectiveExtraCur[idx]++]);
            break;
        case QRA_AliasExtra:
            if (validAlias(idx) && idx < aliasExtra.size()
                && aliasExtraCur[idx] < aliasExtra[idx].size())
                esm.writeRawSubRecord(aliasExtra[idx][aliasExtraCur[idx]++]);
            break;
        case QRA_QuestDesc:
            if (questDesc == parseQuestStringSave(questDescRaw))
            {
                esm.startSubRecord('CNAM');
                esm.writeRawData(questDescRaw.constData(), questDescRaw.size());
                esm.endSubRecord();
            }
            else
            {
                esm.writeSubZString('CNAM', questDesc);
            }
            break;
        case QRA_DialogueView:
            if (dialogueView == parseQuestStringSave(dialogueViewRaw))
            {
                esm.startSubRecord('NAM1');
                esm.writeRawData(dialogueViewRaw.constData(), dialogueViewRaw.size());
                esm.endSubRecord();
            }
            else
            {
                esm.writeSubZString('NAM1', dialogueView);
            }
            break;
        default:
            break;
        }
    }

    // Entries appended after load have no replay positions: emit them whole.
    for (int i = seenStages; i < stageIds.size(); ++i)
    {
        writeStage(i);
        if (i < stageExtra.size())
        {
            for (const auto& raw : stageExtra[i])
            {
                esm.writeRawSubRecord(raw);
            }
        }
    }
    for (int i = seenObjectives; i < objectiveIds.size(); ++i)
    {
        writeObjective(i);
        if (i < objectiveExtra.size())
        {
            for (const auto& raw : objectiveExtra[i])
            {
                esm.writeRawSubRecord(raw);
            }
        }
    }
    for (int i = seenAliases; i < aliasIds.size(); ++i)
    {
        writeAlias(i);
        if (i < aliasExtra.size())
        {
            for (const auto& raw : aliasExtra[i])
            {
                esm.writeRawSubRecord(raw);
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
    loadOrder.clear();
    loadAction.clear();
    loadIndex.clear();
    stageHasQsdt.clear();
    stageHasCnam.clear();
    stageQsdtVals.clear();
    stageCnamVals.clear();
    stageCnamRaws.clear();
    questDescRaw.clear();
    dialogueViewRaw.clear();
    hasEdid = false;
    verbatimBody.clear();
    verbatimFlags = 0;
    verbatimSnapshot.reset();
    initComponents();
}
